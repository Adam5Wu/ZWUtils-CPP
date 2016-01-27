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
 * @addtogroup Threading Threading Supporting Utilities
 * @file
 * @brief Synchronization Elements
 * @author Zhenyu Wu
 * @date Jan 07, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_SyncElements_H
#define ZWUtils_SyncElements_H

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

//#define __SYNC_DEBUG(s)	s

#ifndef DBGVV
#ifdef __SYNC_DEBUG
#undef __SYNC_DEBUG
#endif
#endif

#ifndef __SYNC_DEBUG
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
	Abandoned_0 = 100 + MAXIMUM_WAIT_OBJECTS,
};

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
	virtual WaitResult WaitFor(WAITTIME Timeout)
	{ FAIL(_T("Abstract function")); }
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
	WaitResult WaitFor(WAITTIME Timeout) override
	{ return Sleep(min(Timeout, Delay)), WaitResult::Signaled; }
#endif
};

/**
 * @ingroup Threading
 * @brief Handle waitable base class
 *
 * Base class for all handle-based waitable objects
 **/
class THandleWaitable : protected THandle, public TWaitable {
public:
	THandleWaitable(HANDLE const &xHandle, TResDealloc const &xDealloc, TResAlloc const &xAlloc = NoAlloc) :
		THandle(xHandle, xDealloc, xAlloc) {}

	/**
	 * Wait for a given amount of time or until signaled
	 **/
	WaitResult WaitFor(WAITTIME Timeout = FOREVER) override;

	/**
	 * Get the waitable handle for advanced operations
	 **/
	virtual THandle WaitHandle(void);
};

WaitResult WaitSingle(THandleWaitable &Waitable, WAITTIME Timeout = FOREVER, bool WaitAPC = false, bool WaitMsg = false);
WaitResult WaitMultiple(std::vector<std::reference_wrapper<THandleWaitable>> const &Waitables, bool WaitAll, WAITTIME Timeout = FOREVER, bool WaitAPC = false, bool WaitMsg = false);

/**
 * @ingroup Threading
 * @brief Semaphore
 *
 * (Optionally Named) Semaphore
 **/
class TSemaphore : public THandleWaitable {
protected:
	HANDLE Create(long Initial, long Maximum, TString const &Name);
	static void Destroy(HANDLE const &rSemaphore);

public:
	TSemaphore(long Initial = 0, long Maximum = 0x7FFFFFFF, TString const &Name = EMPTY_TSTRING()) :
		THandleWaitable(Create(Initial, Maximum, Name), Destroy) {}

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
	static void Destroy(HANDLE const &rMutex);

public:
	TMutex(bool Acquired = false, TString const &Name = EMPTY_TSTRING()) :
		THandleWaitable(Create(Acquired, Name), Destroy) {}

	/**
	 * Acquire the lock, wait forever
	 **/
	WaitResult Acquire(void)
	{ return WaitFor(); }

	/**
	 * Try to acquire the lock within given time
	 **/
	WaitResult TryAcquire(WAITTIME Timeout = 0)
	{ return WaitFor(Timeout); }

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
	static void Destroy(HANDLE const &rEvent);

public:
	TEvent(bool ManualReset = false, bool Initial = false, TString const &Name = EMPTY_TSTRING()) :
		THandleWaitable(Create(ManualReset, Initial, Name), Destroy) {}

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

/**
 * @ingroup Threading
 * @brief Critical Section
 *
 * Lightweight synchronization for single thread
 * @note Not a waitable object (no handle)
 **/
class TCriticalSection {
private:
	RTL_CRITICAL_SECTION rCriticalSection;

public:
	TCriticalSection(bool Entered = false, unsigned int SpinCount = 5120) {
		InitializeCriticalSectionAndSpinCount(&rCriticalSection, SpinCount);
		if (Entered) Enter();
	}

	~TCriticalSection(void) {
		DEBUGV_DO_OR({if (!TryEnter()) LOG(_T("WARNING: Freeing an acquired critical section!"));}, {Enter();});
		DeleteCriticalSection(&rCriticalSection);
	}

	/**
	 * Enter the critical section
	 **/
	void Enter(void)
	{ EnterCriticalSection(&rCriticalSection); }

	/**
	 * Try enter the critical section
	 **/
	bool TryEnter(void)
	{ return TryEnterCriticalSection(&rCriticalSection) != 0; }

	/**
	 * Leave the critical section
	 **/
	void Leave(void)
	{ LeaveCriticalSection(&rCriticalSection); }
};

#endif

#endif //ZWUtils_SyncElements_H