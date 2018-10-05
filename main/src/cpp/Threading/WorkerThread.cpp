/*
Copyright (c) 2005 - 2017, Zhenyu Wu; 2012 - 2017, NEC Labs America Inc.
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
	~TThreadRecord(void) { if (SelfFree) DEFAULT_DESTROY(TWorkerThread, WorkerThread); }

	DWORD operator()(void) {
		return (WorkerThread->*ThreadMain)();
	}
};
typedef ManagedRef<TThreadRecord> MRThreadRecord;

static DWORD WINAPI _ThreadProc(LPVOID PThreadRecord) {
	MRThreadRecord ThreadRecord((TThreadRecord*)PThreadRecord, CONSTRUCTION::HANDOFF);
	return (*ThreadRecord)();
}

#endif

#define WTLogTag _T("WThread '%s'")
#define WTLogHeader _T("{") WTLogTag _T("} ")

void TWorkerThread::__StateNotify(State const &rState) {
	auto LSubscriberList(LSubscribers[(unsigned int)rState].Pickup());
	for (size_t i = 0; i < LSubscriberList->size(); i++) {
		auto & entry = LSubscriberList->at(i);
		LOGVV(WTLogHeader _T("Notifying [%s] local event '%s'..."), Name.c_str(), STR_State(rState), entry.first.c_str());
		entry.second(*this, rState);
	}

	auto GSubscriberList(GSubscribers[(unsigned int)rState].Pickup());
	for (size_t i = 0; i < GSubscriberList->size(); i++) {
		auto & entry = GSubscriberList->at(i);
		LOGVV(WTLogHeader _T("Notifying [%s] global event '%s'..."), Name.c_str(), STR_State(rState), entry.first.c_str());
		entry.second(*this, rState);
	}
}

TWorkerThread::TNotificationStub TWorkerThread::StateNotify(TString const &Name, State const &rState, TStateNotice const &Func) {
	auto SubscriberList(LSubscribers[(unsigned int)rState].Pickup());
	for (auto &entry : *SubscriberList) {
		if (entry.first.compare(Name) == 0)
			FAIL(WTLogHeader _T("[%s] event '%s' already registered"), Name.c_str(), STR_State(rState), Name.c_str());
	}
	SubscriberList->emplace_back(Name, Func);

	return { Name, [&, rState](TString const &EvtName) {
		auto UnsubscriberList(LSubscribers[(unsigned int)rState].Pickup());
		auto Iter = UnsubscriberList->begin();
		while (Iter != UnsubscriberList->end()) {
			if (Iter->first.compare(EvtName) == 0) {
				UnsubscriberList->erase(Iter);
				return;
			}
		}
		LOG(WTLogHeader _T("WARNING: Failed to unregister [%s] event '%s'"), Name.c_str(), STR_State(rState), EvtName.c_str());
		}
	};
}

TSyncObj<TWorkerThread::TSubscriberList> TWorkerThread::GSubscribers[(unsigned int)State::__MAX_STATES];

TWorkerThread::TNotificationStub TWorkerThread::GStateNotify(TString const &Name, State const &rState, TStateNotice const &Func) {
	auto SubscriberList(GSubscribers[(unsigned int)rState].Pickup());
	for (auto &entry : *SubscriberList) {
		if (entry.first.compare(Name) == 0)
			FAIL(_T("WorkerThread [%s] global event '%s' already registered"), STR_State(rState), Name.c_str());
	}
	SubscriberList->emplace_back(Name, Func);

	return { Name, [&, rState](TString const &EvtName) {
		auto UnsubscriberList(GSubscribers[(unsigned int)rState].Pickup());
		auto Iter = UnsubscriberList->begin();
		while (Iter != UnsubscriberList->end()) {
			if (Iter->first.compare(EvtName) == 0) {
				UnsubscriberList->erase(Iter);
				return;
			}
		}
		LOG(_T("WARNING: Failed to unregister WorkerThread [%s] global event '%s'"), STR_State(rState), EvtName.c_str());
		}
	};
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

#include <process.h>

typedef unsigned(__stdcall *__ThreadProc) (void *);

HANDLE TWorkerThread::__CreateThread(size_t StackSize, bool xSelfFree) {
	MRThreadRecord Foward(CONSTRUCTION::EMPLACE, this, &TWorkerThread::__CallForwarder, xSelfFree);
	HANDLE rThread = (HANDLE)_beginthreadex(nullptr, (UINT)StackSize, (__ThreadProc)&_ThreadProc,
		&Foward, CREATE_SUSPENDED, (PUINT)&_ThreadID);
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

void TWorkerThread::__DestroyThread(TString const &Name, HANDLE &X) {
	if (GetCurrentThreadId() != _ThreadID) {
		State PrevState = SignalTerminate();
		if ((PrevState == State::Running) || (PrevState == State::Terminating)) {
			WTLOGV(_T("WARNING: Waiting for worker termination..."));
			WaitFor();
		}
	} else {
		WTLOGV(_T("Self destruction in progress..."));
	}

	// Close the thread handle
	THandle::HandleDealloc_BestEffort(X);
}

DWORD TWorkerThread::__CallForwarder(void) {
	DWORD Ret = 0;
	auto iCurState = _State.CompareAndSwap(State::Initialzing, State::Running);
	switch (iCurState) {
		case State::Initialzing:
			__StateNotify(State::Running);
			WTLOGV(_T("Running"));
			try {
				rReturnData = rRunnable->Run(*this, rInputData);
			} catch (Exception *e) {
				rException = ManagedRef<Exception>(e, CONSTRUCTION::HANDOFF);
				DEBUGV_DO(if (dynamic_cast<TWorkThreadSelfDestruct*>(e) == nullptr) {
					WTLOG(_T("WARNING: Abnormal termination due to unhanded ZWUtils Exception - %s"), rException->Why().c_str());
					auto *STE = dynamic_cast<STException*>(e);
					if (STE != nullptr) STE->ShowStack();
				});
			}
			// Fall through...

		case State::Terminating:
			WTLOGV(_T("Terminated"));
			break;

		default:
			WTLOG(_T("WARNING: Unexpected state [%s]"), STR_State(iCurState));
	}
	__StateNotify(_State = State::Terminated);
	return Ret;
}

void TWorkerThread::Start(TFixedBuffer &&xInputData) {
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

DWORD TWorkerThread::ThreadID(void) {
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
				__StateNotify(State::Terminating), rRunnable->StopNotify(*this);
				break;
		}
		return iCurState;
	}
}

bool TWorkerThread::AbortIO(void) {
	if (!CancelSynchronousIo(Refer()))
		return GetLastError() == ERROR_NOT_FOUND;
	return true;
}

#endif

void* TWorkerThread::ReturnData(void) {
	State iCurState = CurrentState();
	if (iCurState != State::Terminated)
		WTFAIL(_T("Could not get return data while in state '%s'"), STR_State(iCurState));
	return *rReturnData;
}

Exception* TWorkerThread::FatalException(bool Prune) {
	State iCurState = CurrentState();
	if (iCurState != State::Terminated)
		WTFAIL(_T("Could not get fatal exception while in state '%s'"), STR_State(iCurState));
	return Prune? rException.Drop() : &rException;
}

WaitResult TWorkerThread::WaitFor(WAITTIME Timeout) const {
	while (!_ResValid) {
		switch (CurrentState()) {
			case State::Constructed:
				// Thread not started, cannot wait
				WTFAIL(_T("Not started"));
				break;

			case State::Initialzing:
				// Starting up, yield and try again
				SwitchToThread();
				continue;

			case State::Running:
				// Cannot be in this state!
				WTFAIL(_T("Internal state inconsistent (Bug-check)"));
				break;

			case State::Terminating:
				// Pre-start terminated
			case State::Terminated:
				// Post termination
				return WaitResult::Signaled;
		}
	}
	return THandleWaitable::WaitFor(Timeout);
}

THandle TWorkerThread::WaitHandle(void) {
	while (!_ResValid) {
		switch (CurrentState()) {
			case State::Constructed:
				// Thread not started, cannot wait
				WTFAIL(_T("Not started"));
				break;

			case State::Initialzing:
				// Starting up, yield and try again
				SwitchToThread();
				continue;

			case State::Running:
				// Cannot be in this state!
				WTFAIL(_T("Internal state inconsistent (Bug-check)"));
				break;

			case State::Terminating:
				// Pre-start terminated
			case State::Terminated:
				// Post termination
				return TEvent(true, true).WaitHandle();
		}
	}
	return THandleWaitable::WaitHandle();
}

TString const& TWorkerThreadException::Why(void) const {
	if (rWhy.empty()) {
		HEAPSTR_ERRMSGFMT(WTLogHeader _T("%s"), WorkerThreadName.c_str(), Exception::Why().c_str());
		rWhy.assign(std::move(__ErrorMsg));
	}
	return rWhy;
}

PCTCHAR const TWorkThreadSelfDestruct::REASON_THREADSELFDESTRUCT = _T("Self Destruction");
