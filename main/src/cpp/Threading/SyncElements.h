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
 * @addtogroup Threading Threading Supporting Utilities
 * @file
 * @brief Synchronization Elements
 * @author Zhenyu Wu
 * @date Jan 07, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_SyncElements_H
#define ZWUtils_SyncElements_H

 // Project global control 
#include "Misc/Global.h"

#include "Misc/TString.h"
#include "Misc/Types.h"

#include "Memory/Resource.h"

#include "Debug/Debug.h"
#include "Debug/Logging.h"
#include "Debug/Exception.h"

#ifdef WINDOWS
#include <Windows.h>
#endif

#include <vector>

//#define __SYNC_DEBUG

#ifdef __SYNC_DEBUG
#undef __SYNC_DEBUG
#define __SYNC_DEBUG(s)	s
#else
#define __SYNC_DEBUG(s)	;
#endif

__SYNC_DEBUG(extern __declspec(thread) bool __IN_LOG);

typedef unsigned int WAITTIME;
extern WAITTIME const FOREVER;

enum class WaitResult : unsigned int {
	Signaled,
	TimedOut,
	Error,
	Abandoned,
	APC,
	Message,

	Signaled_0 = 100,
#if MAXIMUM_WAIT_OBJECTS < 8
#error Come on, you can do better than that!
#endif
	Signaled_1 = Signaled_0 + 1,
	Signaled_2 = Signaled_0 + 2,
	Signaled_3 = Signaled_0 + 3,
	Signaled_4 = Signaled_0 + 4,
	Signaled_5 = Signaled_0 + 5,
	Signaled_6 = Signaled_0 + 6,
	Signaled_7 = Signaled_0 + 7,
	//...
	Signaled_MAX = Signaled_0 + MAXIMUM_WAIT_OBJECTS - 1,

	Abandoned_0 = Signaled_MAX + 1,
	Abandoned_1 = Abandoned_0 + 1,
	Abandoned_2 = Abandoned_0 + 2,
	Abandoned_3 = Abandoned_0 + 3,
	Abandoned_4 = Abandoned_0 + 4,
	Abandoned_5 = Abandoned_0 + 5,
	Abandoned_6 = Abandoned_0 + 6,
	Abandoned_7 = Abandoned_0 + 7,
	//...
	Abandoned_MAX = Abandoned_0 + MAXIMUM_WAIT_OBJECTS - 1,
};

//! Calculate the slot of the result result (no sanity check, make sure pass in sane values)
unsigned int WaitSlot(WaitResult const &X, WaitResult const &Base);

TString WaitResultToString(WaitResult const &WRet);

/**
 * @ingroup Threading
 * @brief Waitable base class
 *
 * Base class for all waitable objects
 **/
class TWaitable {
public:
	virtual ~TWaitable() {}

	/**
	 * Wait for a given amount of time or until signaled
	 **/
	virtual WaitResult WaitFor(WAITTIME Timeout) const {
		FAIL(_T("Abstract function"));
	}
};

/**
 * @ingroup Threading
 * @brief Fix-delay waitable class
 *
 * Pseudo-waitable that impose a fixed maximum delay and succeed
 **/
class TDelayWaitable : public TWaitable {
protected:
	WAITTIME const Delay;

public:
	TDelayWaitable(WAITTIME const xDelay) : Delay(xDelay) {}

#ifdef WINDOWS
	WaitResult WaitFor(WAITTIME Timeout) const override {
		return Timeout < Delay ? Sleep(Timeout), WaitResult::TimedOut : Sleep(Delay), WaitResult::Signaled;
	}
#endif
};

/**
 * @ingroup Threading
 * @brief Handle waitable base class
 *
 * Base class for all handle-based waitable objects
 **/
class THandleWaitable : protected THandle, public TWaitable {
	typedef THandleWaitable _this;
protected:
	bool WaitOnly = false;
public:
	THandleWaitable(TResAlloc const &xAlloc, TResDealloc const &xDealloc = THandle::HandleDealloc_Standard) :
		THandle(xAlloc, xDealloc) {}
	THandleWaitable(HANDLE const &xHandle, TResDealloc const &xDealloc = THandle::HandleDealloc_Standard, TResAlloc const &xAlloc = NoAlloc) :
		THandle(xHandle, xDealloc, xAlloc) {}

	// Move construction
	THandleWaitable(_this &&xHandleWaitable) :
		THandle(std::move(xHandleWaitable)), WaitOnly(xHandleWaitable.WaitOnly) {}

	// Move assignment
	_this& operator=(_this &&xHandleWaitable) {
		THandle::operator=(std::move(xHandleWaitable));
		WaitOnly = xHandleWaitable.WaitOnly;
		return *this;
	}

	/**
	 * Wait for a given amount of time or until signaled
	 **/
	WaitResult WaitFor(WAITTIME Timeout = FOREVER) const override;

	/**
	 * Get the waitable handle for advanced operations
	 **/
	virtual THandle WaitHandle(void);

	/**
	 * Get a duplicated THandleWaitable instance 
	 **/
	THandleWaitable DupWaitable(void);

	//! Expose handle allocation query API
	using THandle::Allocated;

	// Restore addressof operator
	THandleWaitable const* operator&(void) const {
		return this;
	}

	THandleWaitable* operator&(void) {
		return this;
	}

};

WaitResult WaitSingle(THandleWaitable &Waitable,
					  WAITTIME Timeout = FOREVER, bool WaitAPC = false, bool WaitMsg = false);
WaitResult WaitMultiple(std::vector<std::reference_wrapper<THandleWaitable>> const &Waitables, bool WaitAll,
						WAITTIME Timeout = FOREVER, bool WaitAPC = false, bool WaitMsg = false);

// Raw API, make sure all handles are waitable!
WaitResult WaitMultiple(std::vector<HANDLE> const &WaitHandles, bool WaitAll,
						WAITTIME Timeout = FOREVER, bool WaitAPC = false, bool WaitMsg = false);

/**
 * @ingroup Threading
 * @brief Semaphore
 *
 * (Optionally Named) Semaphore
 **/
class TSemaphore : public THandleWaitable {
protected:
	HANDLE Create(long Initial, long Maximum, TString const &Name);

public:
	TSemaphore(long Initial = 0, long Maximum = 0x7FFFFFFF, TString const &Name = EMPTY_TSTRING()) :
		THandleWaitable(Create(Initial, Maximum, Name)) {}
	TSemaphore(CONSTRUCTION::DEFER_T const&, long Initial = 0, long Maximum = 0x7FFFFFFF, TString const &Name = EMPTY_TSTRING()) :
		THandleWaitable([=] { return Create(Initial, Maximum, Name); }) {}

	THandle SignalHandle(void);

	/**
	 * Signal the semaphore with given count
	 * @return Previous semaphore count
	 **/
	long Signal(long Count = 1);
};

/**
 * @ingroup Threading
 * @brief Mutex
 *
 * (Optionally Named) Mutex
 **/
class TMutex : public THandleWaitable {
protected:
	HANDLE Create(bool Acquired, TString const &Name);

public:
	TMutex(bool Acquired = false, TString const &Name = EMPTY_TSTRING()) :
		THandleWaitable(Create(Acquired, Name)) {}
	TMutex(CONSTRUCTION::DEFER_T const&, bool Acquired = false, TString const &Name = EMPTY_TSTRING()) :
		THandleWaitable([=] { return Create(Acquired, Name); }) {}

	/**
	 * Acquire the lock, wait forever
	 **/
	WaitResult Acquire(void) {
		return WaitFor();
	}

	/**
	 * Try to acquire the lock within given time
	 **/
	WaitResult TryAcquire(WAITTIME Timeout = 0) {
		return WaitFor(Timeout);
	}

	/**
	 * Release the lock
	 **/
	void Release(void);
};

/**
 * @ingroup Threading
 * @brief Event
 *
 * (Optionally Named) Event
 **/
class TEvent : public THandleWaitable {
protected:
	HANDLE Create(bool ManualReset, bool Initial, TString const &Name);

public:
	TEvent(bool ManualReset = false, bool Initial = false, TString const &Name = EMPTY_TSTRING()) :
		THandleWaitable(Create(ManualReset, Initial, Name)) {}
	TEvent(CONSTRUCTION::DEFER_T const&, bool ManualReset = false, bool Initial = false, TString const &Name = EMPTY_TSTRING()) :
		THandleWaitable([=] { return Create(ManualReset, Initial, Name); }) {}

	THandle SignalHandle(void);

	/**
	 * Set the event
	 **/
	void Set(void);

	/**
	 * Reset the event
	 **/
	void Reset(void);

	/**
	 * Pulse the event
	 **/
	void Pulse(void);
};

#ifdef WINDOWS

#if (_WIN32_WINNT >= 0x0600)
class TConditionVariable;

#define __ZWUTILS_SYNC_CONDITIONVAIRABLE
#define __ZWUTILS_SYNC_SLIMRWLOCK
#endif

#define DEFAULT_CRITICALSECTION_SPIN	1024

/**
 * @ingroup Threading
 * @brief Critical Section
 *
 * Light-weight mutex-like lock synchronization object
 * @note No handle, not a waitable object (no proper timeout support)
 **/
class TCriticalSection {
#ifdef __ZWUTILS_SYNC_CONDITIONVAIRABLE
	friend class TConditionVariable;
#endif
private:
	RTL_CRITICAL_SECTION rCriticalSection;

public:
	TCriticalSection(bool Entered = false, unsigned int SpinCount = DEFAULT_CRITICALSECTION_SPIN) {
		InitializeCriticalSectionAndSpinCount(&rCriticalSection, SpinCount);
		if (Entered) Enter();
	}

	~TCriticalSection(void) {
		DEBUGV_DO_OR({ if (!TryEnter()) LOG(_T("WARNING: Freeing an acquired critical section!")); }, { Enter(); });
		DeleteCriticalSection(&rCriticalSection);
	}

	/**
	 * Enter the critical section
	 **/
	void Enter(void) {
		EnterCriticalSection(&rCriticalSection);
	}

	/**
	 * Try enter the critical section
	 **/
	bool TryEnter(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) {
#ifdef _DEBUG
		if (!SpinCount) FAIL(_T("Spin count must be a natrual number"));
#endif
		while (--SpinCount && !TryEnterCriticalSection(&rCriticalSection));
		return SpinCount || TryEnterCriticalSection(&rCriticalSection) != 0;
	}

	/**
	 * Leave the critical section
	 **/
	void Leave(void) {
		LeaveCriticalSection(&rCriticalSection);
	}
};

#ifdef __ZWUTILS_SYNC_SLIMRWLOCK

/**
 * @ingroup Threading
 * @brief Slim Reader/Writer Locks
 *
 * Light-weight reader/writer lock synchronization object
 * @note No handle, not a waitable object (no proper timeout support)
 **/
class TSRWLock {
	friend class TConditionVariable;
private:
	RTL_SRWLOCK rSRWlock;

public:
	TSRWLock(void) {
		InitializeSRWLock(&rSRWlock);
	}

	~TSRWLock(void) {
		DEBUGV_DO_OR({ if (!TryWrite()) LOG(_T("WARNING: Freeing an acquired slim R/W lock!")); }, { BeginWrite(); });
	}

	/**
	 * Acquire read lock
	 **/
	void BeginRead(void) {
		AcquireSRWLockShared(&rSRWlock);
	}

	/**
	 * Try acquiring read lock
	 **/
	bool TryRead(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) {
#ifdef _DEBUG
		if (!SpinCount) FAIL(_T("Spin count must be a natrual number"));
#endif
		while (--SpinCount && !TryAcquireSRWLockShared(&rSRWlock));
		return SpinCount || TryAcquireSRWLockShared(&rSRWlock) != 0;
	}

	/**
	 * Release read lock
	 **/
	void EndRead(void) {
		ReleaseSRWLockShared(&rSRWlock);
	}

	/**
	 * Acquire write lock
	 **/
	void BeginWrite(void) {
		AcquireSRWLockExclusive(&rSRWlock);
	}

	/**
	 * Try acquiring write lock
	 **/
	bool TryWrite(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) {
#ifdef _DEBUG
		if (!SpinCount) FAIL(_T("Spin count must be a natrual number"));
#endif
		while (--SpinCount && !TryAcquireSRWLockExclusive(&rSRWlock));
		return SpinCount || TryAcquireSRWLockExclusive(&rSRWlock) != 0;
	}

	/**
	 * Release write lock
	 **/
	void EndWrite(void) {
		ReleaseSRWLockExclusive(&rSRWlock);
	}

};

#endif

#ifdef __ZWUTILS_SYNC_CONDITIONVAIRABLE

/**
 * @ingroup Threading
 * @brief Condition Variable
 *
 * Light-weight synchronization control for light-weight synchronization objects
 * @note No handle, have timeout support but not a waitable (different API interface)
 **/
class TConditionVariable {
private:
	RTL_CONDITION_VARIABLE rConditionVariable;

public:
	TConditionVariable(void) {
		InitializeConditionVariable(&rConditionVariable);
	}

	~TConditionVariable(void) {
		// Do nothing
	}

	/**
	 * Wait on a critical section
	 **/
	WaitResult WaitFor(TCriticalSection &CS, WAITTIME Timeout = FOREVER);

	/**
	 * Wait on a slim read/write lock
	 **/
	WaitResult WaitFor(TSRWLock &SRW, bool isWriting, WAITTIME Timeout = FOREVER);

	/**
	 * Wake up one/all waiters (if any)
	 **/
	void Signal(bool All = false);
};

#endif

#endif

class __ClockRunner;

/**
* @ingroup Threading
* @brief Waitable alarm clock
*
* Provide ability to wait for a duration or fixed time point
* @note: Not threadsafe, if desired wrap around with TSyncObj<>
**/
class TAlarmClock : public THandleWaitable {
	friend class __ClockRunner;
protected:
	TEvent _HEvent, _WEvent;
	WaitResult _WRet;

	struct ClockRet {
		TimeStamp ExitTS;
		TimeSpan Remainder;
		WaitResult Result;
	} _ClockRet;

	static HANDLE __GetWaitHandle(TEvent &Event) {
		THandle iRet = Event.WaitHandle();
		return *iRet, *iRet.Drop();
	}

public:
	TAlarmClock(void) : THandleWaitable([&] { return __GetWaitHandle(_HEvent); }),
		_HEvent(CONSTRUCTION::DEFER, true, true), _WEvent(CONSTRUCTION::DEFER) {}

	TAlarmClock(TimeSpan const &Duration) : TAlarmClock() {
		Arm(Duration);
	}

	TAlarmClock(TimeStamp const &Clock) : TAlarmClock() {
		Arm(Clock);
	}

	~TAlarmClock(void) { Disarm(true); }

	/**
	 * Set trigger for a duration
	 **/
	void Arm(TimeSpan const &Duration) {
		return Arm(TimeStamp::Now(Duration));
	}

	/**
	 * Set trigger for a fixed time point
	 **/
	void Arm(TimeStamp const &Clock);

	/**
	 * Check if clock is armed
	 **/
	bool Armed(void) const;

	/**
	 * Disarm the clock (release all waiters)
	 **/
	bool Disarm(bool WaitFor = false);

	/**
	* Wait for a given amount of time or until signaled
	**/
	WaitResult WaitFor(WAITTIME Timeout = FOREVER) const override;
};

#endif //ZWUtils_SyncElements_H