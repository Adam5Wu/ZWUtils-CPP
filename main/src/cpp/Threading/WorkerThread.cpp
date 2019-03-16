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

class TThreadCreateRecord {
private:
	TWorkerThread* const WorkerThread;
	typedef DWORD(TWorkerThread::*WTThreadMain)(void);
	WTThreadMain const ThreadMain;
	bool const SelfFree;

public:
	TThreadCreateRecord(TWorkerThread* const &xWorkerThread, WTThreadMain const &xThreadMain, bool const &xSelfFree) :
		WorkerThread(xWorkerThread), ThreadMain(xThreadMain), SelfFree(xSelfFree) {}
	~TThreadCreateRecord(void) { if (SelfFree) DEFAULT_DESTROY(TWorkerThread, WorkerThread); }

	DWORD operator()(void) {
		return (WorkerThread->*ThreadMain)();
	}
};
typedef ManagedRef<TThreadCreateRecord> MRThreadCreateRecord;

static DWORD WINAPI _ThreadProc(LPVOID Data) {
	ControlSEHTranslation(true);
	MRThreadCreateRecord ThreadCreateRecord((TThreadCreateRecord*)Data, CONSTRUCTION::HANDOFF);
	return (*ThreadCreateRecord)();
}

class TThreadAPCRecord {
private:
	TWorkerThread* const WorkerThread;
	typedef void(TWorkerThread::*WTThreadAPC)(TString const &, TWorkerThread::TAPCFunc const &);
	WTThreadAPC const ThreadAPC;
	TString const Name;
	TWorkerThread::TAPCFunc const APCFunc;

public:
	TThreadAPCRecord(TWorkerThread* const &xWorkerThread, WTThreadAPC const &xThreadAPC,
					 TString const &xName, TWorkerThread::TAPCFunc const &xAPCFunc) :
		WorkerThread(xWorkerThread), ThreadAPC(xThreadAPC), Name(xName), APCFunc(xAPCFunc) {}

	void operator()(void) {
		return (WorkerThread->*ThreadAPC)(Name, APCFunc);
	}
};
typedef ManagedRef<TThreadAPCRecord> MRThreadAPCRecord;

static VOID NTAPI _ThreadAPC(ULONG_PTR Data) {
	MRThreadAPCRecord ThreadAPCRecord((TThreadAPCRecord*)Data, CONSTRUCTION::HANDOFF);
	return (*ThreadAPCRecord)();
}

// Utility for setting thread name
// Reference: https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)  
typedef struct tagTHREADNAME_INFO {
	DWORD dwType; // Must be 0x1000.  
	LPCSTR szName; // Pointer to name (in user addr space).  
	DWORD dwThreadID; // Thread ID (-1=caller thread).  
	DWORD dwFlags; // Reserved for future use, must be zero.  
} THREADNAME_INFO;
#pragma pack(pop)  

static void SetThreadName(DWORD dwThreadID, LPCSTR threadName) {
	THREADNAME_INFO info = { 0x1000, threadName, dwThreadID, 0 };
#pragma warning(push)
#pragma warning(disable: 6320 6322) 
	__try {
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		// Do nothing, ignore exception
	}
#pragma warning(pop) 
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
#define WTFAIL(...) {														\
	SOURCEMARK																\
	throw TWorkerThreadException(*this, std::move(__SrcMark), __VA_ARGS__);	\
}

//! @ingroup Threading
//! Raise a self-destruct exception within a worker thread
#define WTDESTROY {												\
	SOURCEMARK													\
	throw TWorkThreadSelfDestruct(*this, std::move(__SrcMark));	\
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
	MRThreadCreateRecord Forward(CONSTRUCTION::EMPLACE, this, &TWorkerThread::__CallForwarder, xSelfFree);
	HANDLE rThread = (HANDLE)_beginthreadex(nullptr, (UINT)StackSize, (__ThreadProc)&_ThreadProc,
											&Forward, CREATE_SUSPENDED, (PUINT)&_ThreadID);
	if (rThread == nullptr) SYSFAIL(_T("Failed to create thread for worker '%s'"), Name.c_str());
	return Forward.Drop(), rThread;
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
	Deallocate();
}

void TWorkerThread::__DestroyThread(TString const &Name, HANDLE &X) {
	if (GetCurrentThreadId() != _ThreadID) {
		State PrevState = SignalTerminate();
		if (PrevState <= State::Terminating) {
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
#ifndef NDEBUG
#ifdef UNICODE
	TString ErrMsg;
	CString TName = WStringtoCString(CP_ACP, Name, ErrMsg);
	if (!ErrMsg.empty()) {
		LOG(_T("WARNING: Error during converting thread name - %s"), ErrMsg.c_str());
	}
	SetThreadName(_ThreadID, TName.c_str());
#else
	SetThreadName(_ThreadID, Name.c_str());
#endif
#endif

	DWORD Ret = 0;
	auto iCurState = _State.CompareAndSwap(State::Initialzing, State::Running);
	switch (iCurState) {
		case State::Initialzing:
			__StateNotify(State::Running);
			WTLOGV(_T("Running"));
			try {
				rReturnData = rRunnable->Run(*this, rInputData);
			} catch (_ECR_ e) {
				rException = { &e, CONSTRUCTION::CLONE };
				DEBUG_DO(if (dynamic_cast<TWorkThreadSelfDestruct const *>(&e) == nullptr) {
					WTLOG(_T("WARNING: Abnormal termination due to unhanded ZWUtils Exception"));
					e.Show();
				});
			} catch (std::exception &e) {
				WTLOG(_T("WARNING: Abnormal termination due to unhanded std::exception - %S"), e.what());
				rException = { STDException::Wrap(std::move(e)), CONSTRUCTION::HANDOFF };
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
	return _ThreadID;
}

TWorkerThread::State TWorkerThread::SignalTerminate(void) {
	while (true) {
		auto iCurState = _State.CompareAndSwap(State::Running, State::Terminating);
		switch (iCurState) {
			case State::Constructed:
				// Try to switch to terminating before someone start it
				iCurState = _State.CompareAndSwap(State::Constructed, State::Terminating);
				// If failed, loop back and try again
				if (iCurState != State::Constructed) continue;
				// We have switched the state, now let loose the thread so it can finish its course
				WTLOGV(_T("Terminated before start"));
				ResumeThread(Refer());
				break;

			case State::Initialzing:
				// Someone already started the thread, and it has not reached running state
				SwitchToThread();
				// Loop back and try again
				continue;

			case State::Running:
				// The thread was in running and we switched it to terminating, so we should do notify
				__StateNotify(State::Terminating);
				rRunnable->StopNotify(*this);
				// The thread will do the state switch to terminated, as well as notify
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

void TWorkerThread::__APCForwarder(TString const &Name, TAPCFunc const &APCFunc) {
	WTLOGV(_T("Invoking APC '%s'..."), Name.c_str());
	try {
		APCFunc();
	} catch (_ECR_ e) {
		LOGEXCEPTIONV(e, _T("Exception during APC exeuction"));
	}
}

void TWorkerThread::QueueAPC(TString const &Name, TAPCFunc const &Func) {
	MRThreadAPCRecord Forward(CONSTRUCTION::EMPLACE, this, &TWorkerThread::__APCForwarder, Name, Func);
	if (!QueueUserAPC(_ThreadAPC, **this, (ULONG_PTR)&Forward)) {
		SYSFAIL(_T("Failed to queue APC for worker '%s'"), Name.c_str());
	}
	Forward.Drop();
}

#endif

void* TWorkerThread::ReturnData(void) {
	State iCurState = CurrentState();
	if (iCurState != State::Terminated)
		WTFAIL(_T("Could not get return data while in state '%s'"), STR_State(iCurState));
	return &rReturnData;
}

Exception const* TWorkerThread::FatalException(bool Prune) {
	State iCurState = CurrentState();
	if (iCurState != State::Terminated)
		WTFAIL(_T("Could not get fatal exception while in state '%s'"), STR_State(iCurState));
	return Prune ? rException.Drop() : &rException;
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

// --- TWorkerThreadException

TWorkerThreadException* TWorkerThreadException::MakeClone(IAllocator &xAlloc) const {
	CascadeObjAllocator<_this> _Alloc(xAlloc);
	auto *iRet = _Alloc.Create(RLAMBDANEW(_this, *this));
	return _Alloc.Drop(iRet);
}

TString const& TWorkerThreadException::Why(void) const {
	if (rWhy.empty()) {
		HEAPSTR_ERRMSGFMT(WTLogHeader _T("%s"), WorkerThreadName.c_str(), Exception::Why().c_str());
		rWhy.assign(std::move(__ErrorMsg));
	}
	return rWhy;
}

PCTCHAR const TWorkThreadSelfDestruct::REASON_THREADSELFDESTRUCT = _T("Self Destruction");
