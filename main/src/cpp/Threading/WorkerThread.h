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

/**
 * @addtogroup Threading Threading Support Utilities
 * @file
 * @brief Generic Worker Thread
 * @author Zhenyu Wu
 * @date Feb 02, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_WorkerThread_H
#define ZWUtils_WorkerThread_H

#include "Misc/Global.h"
#include "Misc/TString.h"

#include "Memory/Allocator.h"
#include "Memory/ManagedRef.h"

#include "Debug/Exception.h"

#include "SyncElements.h"
#include "SyncObjects.h"

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
	virtual TBuffer Run(TWorkerThread &WorkerThread, TBuffer &Arg)
	{ FAIL(_T("Abstract function")); }

	/**
	 * Notify the stop request from worker thread
	 **/
	virtual void StopNotify(void) {}
};

/**
 * @ingroup Threading
 * @brief Generic worker thread template
 *
 * Can wrap "runnable" class instance into a thread
 **/
class TWorkerThread : public THandleWaitable {
	typedef TWorkerThread _this;
	friend class ManagedRef < _this > ;
	friend class IObjAllocator < _this > ;
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
	void __DestroyThread(TString const &Name);

#ifdef WINDOWS
	void CollectSEHException(struct _EXCEPTION_POINTERS *SEH);
	DWORD __CallForwarder(void);
#endif

protected:
	ManagedRef<TRunnable> rRunnable;
	ManagedRef<Exception> rException;

	TBuffer rInputData;
	TBuffer rReturnData;

	TInterlockedOrdinal32<State> _State = State::Constructed;

#ifdef WINDOWS
	DWORD _ThreadID;
#endif

	/**
	 * Create a thread for executing the Runnable object instance
	 * @note Thread is not running until one of start function is called
	 * @note Use caution when starting self-free thread -- the instance may be freed *ANY MOMENT* after start.
	 *       Basically, once started, you should give up *ALL* active control and communication.
	 **/
	template<class IRunnable>
	TWorkerThread(TString const &xName, IRunnable &xRunnable, bool xSelfFree = false, size_t xStackSize = 0) :
		THandleWaitable(__CreateThread(xStackSize, xSelfFree), [&, xName](HANDLE &X) {__DestroyThread(xName); }),
		rRunnable(std::addressof(xRunnable), CONSTRUCTION::HANDOFF), Name(xName) {
		__StateNotify(~_State);
	}

	void __Pre_Destroy(void);

public:
	TString const Name;

	~TWorkerThread(void) { Deallocate(); }

	WaitResult WaitFor(WAITTIME Timeout = FOREVER) override;
	THandle WaitHandle(void) override;

	/**
	 * Start thread execution (if already started, will raise exception)
	 **/
	void Start(TBuffer &&xInputData = TBuffer(nullptr));

	/**
	 * Start thread execution, and self-free when finish
	 void Handoff_Start(TBuffer &&xInputData = TBuffer(nullptr));

	 /**
	 * Get worker thread ID
	 **/
	__ARC_UINT ThreadID(void);

	/**
	 * Signal the thread to terminate
	 * @return Previous worker thread state
	 **/
	State SignalTerminate(void);

	/**
	 * Get current the thread state
	 **/
	State CurrentState(void)
	{ return ~_State; }

	/**
	 * Get the return data of the thread
	 * @note Must be called AFTER worker thread goes to wtsTerminated
	 * @note The caller does *NOT* own the retrieved data, DO NOT FREE!
	 **/
	void* ReturnData(void);

	/**
	 * Get the Exception object that terminated the thread
	 * @note Must be called AFTER worker thread goes to wtsTerminated
	 * @note The caller does *NOT* own the retrieved exception, DO NOT FREE!
	 **/
	Exception* FatalException(void);

	typedef std::function<void(TWorkerThread &, State const &) throw()> TStateNotice;
	typedef TAllocResource<TString> TNotificationStub;
	static TNotificationStub StateNotify(TString const &Name, State const &rState, TStateNotice const &Func);

	template<class IRunnable>
	static TWorkerThread* Create(TString const &xName, IRunnable &xRunnable, bool xSelfFree = false, size_t xStackSize = 0) {
		return DefaultObjAllocator<_this>().Create(RLAMBDANEW(_this, xName, xRunnable, xSelfFree, xStackSize));
	}

protected:
	typedef std::vector<std::pair<TString, TStateNotice>> TSubscriberList;
	static TSyncObj<TSubscriberList> Subscribers[(unsigned int)State::__MAX_STATES];

	void __StateNotify(State const &rState);
};

//! @ingroup Threading
//! Worker thread specific exception
class TWorkerThreadException : public Exception {
	typedef TWorkerThreadException _this;

protected:
	template<typename... Params>
	TWorkerThreadException(TWorkerThread const &xWorkerThread, TString &&xSource, PCTCHAR ReasonFmt, Params&&... xParams) :
		Exception(std::move(xSource), ReasonFmt, xParams...), WorkerThreadName(xWorkerThread.Name) {}

public:
	TString const WorkerThreadName;

	template<typename... Params>
	static _this* Create(TWorkerThread const &xWorkerThread, TString &&xSource, PCTCHAR ReasonFmt, Params&&... xParams) {
		return DefaultObjAllocator<_this>().Create(RLAMBDANEW(_this, xWorkerThread, std::move(xSource), ReasonFmt, xParams...));
	}

	TString const& Why(void) const override;
};

class TWorkThreadSelfDestruct : public TWorkerThreadException {
	typedef TWorkThreadSelfDestruct _this;

protected:
	static PCTCHAR const REASON_THREADSELFDESTRUCT;

	TWorkThreadSelfDestruct(TWorkerThread const &xWorkerThread, TString &&xSource) :
		TWorkerThreadException(xWorkerThread, std::move(xSource), REASON_THREADSELFDESTRUCT) {}

public:
	static _this* Create(TWorkerThread const &xWorkerThread, TString &&xSource) {
		return DefaultObjAllocator<_this>().Create(RLAMBDANEW(_this, xWorkerThread, std::move(xSource)));
	}
};

#endif //ZWUtils_WorkerThread_H

