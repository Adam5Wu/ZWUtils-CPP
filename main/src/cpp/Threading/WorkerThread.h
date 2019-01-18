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

/**
 * @addtogroup Threading Threading Support Utilities
 * @file
 * @brief Generic Worker Thread
 * @author Zhenyu Wu
 * @date Feb 02, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_WorkerThread_H
#define ZWUtils_WorkerThread_H

 // Project global control 
#include "Misc/Global.h"

#include "Misc/TString.h"

#include "Memory/Allocator.h"
#include "Memory/ManagedRef.h"

#include "Debug/Exception.h"

#include "SyncElements.h"
#include "SyncObjects.h"

#include <functional>
#include <vector>

class TWorkerThread;

/**
 * @ingroup Threading
 * @brief Runnable template
 *
 * Defines the required function call interfaces for TWorkerThread wrapper
 **/
class TRunnable {
	friend TWorkerThread;
public:
	virtual ~TRunnable(void) {}

	/**
	 * The "main" function of the runnable object
	 **/
	virtual TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) {
		FAIL(_T("Abstract function"));
	}

	/**
	 * Notify the stop request from worker thread
	 **/
	virtual void StopNotify(TWorkerThread &WorkerThread) {}
};

typedef ManagedRef<TRunnable> MRRunnable;
typedef ManagedRef<Exception> MRException;

/**
 * @ingroup Threading
 * @brief Generic worker thread template
 *
 * Can wrap "runnable" class instance into a thread
 **/
class TWorkerThread : public THandleWaitable {
	typedef TWorkerThread _this;
	friend class ManagedRef<_this>;
	friend class IObjAllocator<_this>;

public:
	enum class State : unsigned int {
		Constructed,
		Initialzing,
		Running,
		Terminating,
		Terminated,
		__MAX_STATES,
	};
	static PCTCHAR STR_State(State const &xState);

private:
	HANDLE __CreateThread(size_t StackSize, bool xSelfFree);
	void __DestroyThread(TString const &Name, HANDLE &X);
	DWORD __CallForwarder(void);

protected:
	MRRunnable rRunnable;
	MRException rException;

	TFixedBuffer rInputData;
	TFixedBuffer rReturnData;

	TInterlockedOrdinal32<State> _State = State::Constructed;

#ifdef WINDOWS
	DWORD _ThreadID;
#endif

	// This function performs pre-processing before the destructor is called
	// This is necessary because of C++'s layered object destruction -- the worker thread is still
	//   running while the wrapper class already started partial destrution, which does not end well.
	void __Pre_Destroy(void);

	/**
	 * Create a thread for executing the Runnable object instance
	 * @note Thread is not running until one of start function is called
	 * @note Use caution when starting self-free thread -- the instance may be freed *ANY MOMENT* after start.
	 *       Basically, once started, you should give up *ALL* active control and communication.
	 * @note We are hiding the constructor because we need our own allocator to manage the destruction process.
	 *       Otherwise, the __Pre_Destroy() function will NOT be executed, and some corner cases (such as,
	 *       destroying a thread before starting it) will lead to undesired outcome (crash)
	 **/
	TWorkerThread(TString const &xName, MRRunnable &&xRunnable, bool xSelfFree = false, size_t xStackSize = 0) :
		THandleWaitable(CONSTRUCTION::HANDOFF, __CreateThread(xStackSize, xSelfFree),
						[&, xName](HANDLE &X) {__DestroyThread(xName, X); }),
		rRunnable(std::move(xRunnable)), Name(xName) {
		__StateNotify(~_State);
	}

public:
	TString const Name;

	~TWorkerThread(void) {}

	WaitResult WaitFor(WAITTIME Timeout = FOREVER) const override;
	THandle WaitHandle(void) override;

	/**
	 * Start thread execution (if already started, will raise exception)
	 **/
	void Start(TFixedBuffer &&xInputData = {});

	/**
	 * Start thread execution, and self-free when finish
	 void Handoff_Start(TFixedBuffer &&xInputData = TFixedBuffer(nullptr));

	 /**
	 * Get worker thread ID
	 **/
	DWORD ThreadID(void);

	/**
	 * Signal the thread to terminate
	 * @return Previous worker thread state
	 **/
	State SignalTerminate(void);

	/**
	 * Abort any asynchronous IO operation that is blocking this thread
	 * @return TRUE if Abort succeeded or no blocking IO; FALSE on other errors
	 **/
	bool AbortIO(void);

	/**
	 * Get current the thread state
	 **/
	State CurrentState(void) const {
		return ~_State;
	}

	/**
	 * Get the return data of the thread
	 * @note Must be called AFTER worker thread goes to wtsTerminated
	 * @note The caller does *NOT* own the retrieved data, DO NOT FREE!
	 **/
	void* ReturnData(void);

	/**
	 * Get the Exception object that terminated the thread
	 * @note Must be called AFTER worker thread goes to wtsTerminated
	 * @note The caller does *NOT* own the retrieved exception unless it is pruned!
	 **/
	Exception const* FatalException(bool Prune = false);

	typedef std::function<void(TWorkerThread &, State const &)> TStateNotice;
	typedef TAllocResource<TString> TNotificationStub;
	/**
	 * Register a notification callback when specified thread state is reached
	 * @note Normal destruction of the notification stub will unreigster callback when it is no longer needed
	 * @note When the notification stub life-span exceeds that of the worker thread, the stub must be manually invalidated
	 **/
	TNotificationStub StateNotify(TString const &Name, State const &rState, TStateNotice const &Func);
	static TNotificationStub GStateNotify(TString const &Name, State const &rState, TStateNotice const &Func);

	static TWorkerThread* Create(TString const &xName, MRRunnable &&xRunnable, bool xSelfFree = false, size_t xStackSize = 0) {
		return DEFAULT_NEW(_this, xName, std::move(xRunnable), xSelfFree, xStackSize);
	}

protected:
	typedef std::vector<std::pair<TString, TStateNotice>> TSubscriberList;
	TSyncObj<TSubscriberList> LSubscribers[(unsigned int)State::__MAX_STATES];
	static TSyncObj<TSubscriberList> GSubscribers[(unsigned int)State::__MAX_STATES];

	void __StateNotify(State const &rState);
};

typedef ManagedRef<TWorkerThread> MRWorkerThread;

//! @ingroup Threading
//! Worker thread specific exception
class TWorkerThreadException : public Exception {
	typedef TWorkerThreadException _this;

public:
	TString const WorkerThreadName;

	template<typename... Params>
	TWorkerThreadException(TWorkerThread const &xWorkerThread, TString &&xSource, PCTCHAR ReasonFmt, Params&&... xParams) :
		Exception(std::move(xSource), ReasonFmt, std::forward<Params>(xParams)...), WorkerThreadName(xWorkerThread.Name) {}

	TWorkerThreadException(_this &&xException) NOEXCEPT
		: Exception(std::move(xException)), WorkerThreadName(std::move(xException.WorkerThreadName)) {}

	TWorkerThreadException(_this const &xException)
		: Exception(xException), WorkerThreadName(xException.WorkerThreadName) {}

	virtual _this* MakeClone(IAllocator &xAlloc) const override;

	TString const& Why(void) const override;
};

class TWorkThreadSelfDestruct : public TWorkerThreadException {
	typedef TWorkThreadSelfDestruct _this;

protected:
	static PCTCHAR const REASON_THREADSELFDESTRUCT;

public:
	TWorkThreadSelfDestruct(TWorkerThread const &xWorkerThread, TString &&xSource) :
		TWorkerThreadException(xWorkerThread, std::move(xSource), REASON_THREADSELFDESTRUCT) {}
};

#endif //ZWUtils_WorkerThread_H

