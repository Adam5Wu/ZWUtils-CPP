/*
Copyright (c) 2005 - 2016, Zhenyu Wu; 2012 - 2016, NEC Labs America Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of ZWUtils-VCPP nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// [Threading] Generic Worker Thread

#include "WorkerThread.h"

#include "Debug/Logging.h"
#include "Debug/SysError.h"

#ifdef WINDOWS

class TThreadRecord {
protected:
	TWorkerThread* const WorkerThread;
	typedef DWORD(TWorkerThread::*WTThreadMain)(void);
	WTThreadMain const ThreadMain;
	bool const SelfFree;

public:
	TThreadRecord(TWorkerThread* const &xWorkerThread, WTThreadMain const &xThreadMain, bool const &xSelfFree) :
		WorkerThread(xWorkerThread), ThreadMain(xThreadMain), SelfFree(xSelfFree) {}
	~TThreadRecord(void) { if (SelfFree) DefaultObjAllocator<TWorkerThread>().Destroy(WorkerThread); }

	DWORD operator()(void)
	{ return (WorkerThread->*ThreadMain)(); }
};
typedef ManagedRef<TThreadRecord> MRThreadRecord;

static DWORD WINAPI _ThreadProc(LPVOID PThreadRecord) {
	MRThreadRecord ThreadRecord((TThreadRecord*)PThreadRecord, CONSTRUCTION::HANDOFF);
	return (*ThreadRecord)();
}

#endif

#define WTLogTag _T("WThread '%s'")
#define WTLogHeader _T("{") WTLogTag _T("} ")

TSyncObj<TWorkerThread::TSubscriberList> TWorkerThread::Subscribers[(unsigned int)State::__MAX_STATES];

void TWorkerThread::__StateNotify(State const &rState) {
	auto SubscriberList(Subscribers[(unsigned int)rState].Pickup());
	for (size_t i = 0; i < SubscriberList->size(); i++) {
		auto & entry = SubscriberList->at(i);
		LOGVV(WTLogHeader _T("Notifying [%s] event '%s'..."), Name.c_str(), STR_State(rState), entry.first.c_str());
		entry.second(*this, rState);
	}
}

TWorkerThread::TNotificationStub TWorkerThread::StateNotify(TString const &Name, State const &rState, TStateNotice const &Func) {
	auto SubscriberList(Subscribers[(unsigned int)rState].Pickup());
	for (auto &entry : *SubscriberList) {
		if (entry.first.compare(Name) == 0)
			FAIL(_T("WorkerThread [%s] event '%s' already registered"), STR_State(rState), Name.c_str());
	}
	SubscriberList->emplace_back(Name, Func);

	return TNotificationStub(Name, [&, rState](TString const &EvtName) {
		auto UnsubscriberList(Subscribers[(unsigned int)rState].Pickup());
		auto Iter = UnsubscriberList->begin();
		while (Iter != UnsubscriberList->end()) {
			if (Iter->first.compare(EvtName) == 0) {
				UnsubscriberList->erase(Iter);
				return;
			}
		}
		LOG(_T("WARNING: Failed to unregister WorkerThread [%s] event '%s'"), STR_State(rState), EvtName.c_str());
	});
}

//! @ingroup Threading
//! Raise an exception within a worker thread with formatted string message
#define WTFAIL(...) {																\
	SOURCEMARK																		\
	throw TWorkerThreadException::Create(*this, std::move(__SrcMark), __VA_ARGS__);	\
}

//! @ingroup Threading
//! Raise a self-destruct exception within a worker thread
#define WTDESTROY {														\
	SOURCEMARK															\
	throw TWorkThreadSelfDestruct::Create(*this, std::move(__SrcMark));	\
}

//! Perform logging within a worker thread
#define WTLOG(s, ...) LOG(WTLogHeader s, Name.c_str(), __VA_ARGS__)
#define WTLOGV(s, ...) LOGV(WTLogHeader s, Name.c_str(), __VA_ARGS__)
#define WTLOGVV(s, ...) LOGVV(WTLogHeader s, Name.c_str(), __VA_ARGS__)

PCTCHAR TWorkerThread::STR_State(State const &xState) {
	static PCTCHAR _STR_State[] = {
		_T("Constructed"),
		_T("Initialzing"),
		_T("Running"),
		_T("Terminating"),
		_T("Terminated"),
	};
	return _STR_State[(int)xState];
}

#ifdef WINDOWS

HANDLE TWorkerThread::__CreateThread(size_t StackSize, bool xSelfFree) {
	MRThreadRecord Foward(CONSTRUCTION::EMPLACE, this, &TWorkerThread::__CallForwarder, xSelfFree);
	HANDLE rThread = CreateThread(nullptr, StackSize, &_ThreadProc, &Foward, CREATE_SUSPENDED, &_ThreadID);
	if (rThread == nullptr) SYSFAIL(_T("Failed to create thread for worker '%s'"), Name.c_str());
	return Foward.Drop(), rThread;
}

void TWorkerThread::__Pre_Destroy(void) {
	switch (CurrentState()) {
		case State::Constructed:
		case State::Terminated:
			break;
		default:
			if (GetCurrentThreadId() == _ThreadID) {
				WTLOG(_T("WARNING: A worker thread should not delete its own instance in the thread body!"));
				WTLOG(_T("HINT: Self-freeing worker thread can be achieved by passing a construction parameter."));
				WTLOG(_T("      Please read the documentation and understand the constraints of using this feature."));
				WTDESTROY;
			}
	}
}

void TWorkerThread::__DestroyThread(TString const &Name) {
	if (GetCurrentThreadId() != _ThreadID) {
		State PrevState = SignalTerminate();
		if ((PrevState == State::Running) || (PrevState == State::Terminating)) {
			WTLOGV(_T("WARNING: Waiting for worker termination..."));
			WaitFor();
		}
	} else {
		WTLOGV(_T("Self destruction in progress..."));
	}
}

DWORD TWorkerThread::__CallForwarder(void) {
	DWORD Ret = 0;
	auto iCurState = _State.CompareAndSwap(State::Initialzing, State::Running);
	if (iCurState == State::Initialzing) {
		__StateNotify(State::Running);
		WTLOGV(_T("Running"));
		try {
			rReturnData = rRunnable->Run(*this, rInputData);
		} catch (Exception *e) {
			rException.Assign(e, CONSTRUCTION::HANDOFF);
			DEBUGV_DO(if (dynamic_cast<TWorkThreadSelfDestruct*>(e) == nullptr) {
				WTLOG(_T("WARNING: Abnormal termination due to unhanded ZWUtils Exception - %s"), rException->Why().c_str());
			});
		}
		WTLOGV(_T("Terminated"));
	}
	__StateNotify(_State = State::Terminated);
	return Ret;
}

void TWorkerThread::Start(TBuffer &&xInputData) {
	auto iCurState = (State)_State.CompareAndSwap(State::Constructed, State::Initialzing);
	switch (iCurState) {
		case State::Constructed:
			__StateNotify(State::Initialzing);
			rInputData = std::move(xInputData);
			ResumeThread(Refer());
			break;
		default:
			WTFAIL(_T("Unable to start, current state [%s]"), STR_State(iCurState));
	}
}

__ARC_UINT TWorkerThread::ThreadID(void) {
	return Refer(), _ThreadID;
}

TWorkerThread::State TWorkerThread::SignalTerminate(void) {
	while (true) {
		auto iCurState = _State.CompareAndSwap(State::Running, State::Terminating);
		switch (iCurState) {
			case State::Constructed:
				iCurState = _State.CompareAndSwap(State::Constructed, State::Terminating);
				if (iCurState == State::Constructed)
					__StateNotify(State::Terminating), ResumeThread(Refer());
				break;
			case State::Initialzing:
				SwitchToThread();
				continue;
			case State::Running:
				__StateNotify(State::Terminating), rRunnable->StopNotify();
				break;
		}
		return iCurState;
	}
}

#endif

void* TWorkerThread::ReturnData(void) {
	State iCurState = CurrentState();
	if (iCurState != State::Terminated)
		WTFAIL(_T("Could not get return data while in state '%s'"), STR_State(iCurState));
	return *rReturnData;
}

Exception* TWorkerThread::FatalException(void) {
	State iCurState = CurrentState();
	if (iCurState != State::Terminated)
		WTFAIL(_T("Could not get fatal exception while in state '%s'"), STR_State(iCurState));
	return &rException;
}

WaitResult TWorkerThread::WaitFor(WAITTIME Timeout) {
	while (true) {
		if (!_ResValid) {
			switch (CurrentState()) {
				case State::Constructed: break;
				case State::Initialzing: continue;
				case State::Running: WTFAIL(_T("Internal state inconsistent (Bug-check)"));
				case State::Terminating: continue;
				case State::Terminated: return WaitResult::Signaled;
			}
		}
		return THandleWaitable::WaitFor(Timeout);
	}
}

THandle TWorkerThread::WaitHandle(void) {
	while (true) {
		if (!_ResValid) {
			switch (CurrentState()) {
				case State::Constructed: break;
				case State::Initialzing: continue;
				case State::Running: WTFAIL(_T("Internal state inconsistent (Bug-check)"));
				case State::Terminating: continue;
				case State::Terminated: return TEvent(true, true).WaitHandle();
			}
		}
		return THandleWaitable::WaitHandle();
	}
}

TString const& TWorkerThreadException::Why(void) const {
	if (rWhy.length() == 0) {
		PCTCHAR dWhy = Exception::Why().c_str();
		HEAPSTR_ERRMSGFMT(WTLogHeader _T("%s"), WorkerThreadName.c_str(), dWhy);
		const_cast<TString*>(&rWhy)->assign(std::move(__ErrorMsg));
	}
	return rWhy;
}

PCTCHAR const TWorkThreadSelfDestruct::REASON_THREADSELFDESTRUCT = _T("Self Destruction");
