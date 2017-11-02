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

// [Utilities] Synchronization Premises

#include "SyncObjects.h"

#define _IMPL_AbortableLock(spin_trylock,single_trylock,objname)										\
	TAllocResource<__ARC_INT> WaitCounter([&] { return WaitCnt++; }, [&](__ARC_INT &) { --WaitCnt; });	\
	TimeStamp EntryTS;																					\
	while (! ##spin_trylock## ) {																		\
		/* Allocate wait counter */																		\
		if (!WaitCounter.Allocated()) {																	\
			if (*WaitCounter) {																			\
				/* Check if we are racing against the event object allocation */						\
				while (!WaitEvent.Allocated()) SwitchToThread();										\
			}																							\
			/* Check again before wait */																\
			if ( ##single_trylock## ) break;															\
		}																								\
		/* Calculate remaining wait time */																\
		if (Timeout != FOREVER) {																		\
			if (!EntryTS) {																				\
				EntryTS = TimeStamp::Now();																\
			} else {																					\
				TimeStamp Now = TimeStamp::Now();														\
				INT64 WaitDur = (Now - EntryTS).GetValue(TimeUnit::MSEC);								\
				Timeout = Timeout > WaitDur ? Timeout - (WAITTIME)WaitDur : 0;							\
				EntryTS = Now;																			\
			}																							\
		}																								\
		/* Perform the wait */																			\
		WaitResult WRet = AbortEvent ?																	\
			WaitMultiple({ WaitEvent, *AbortEvent }, false, Timeout) :									\
			WaitEvent.WaitFor(Timeout);																	\
		/* Analyze the result */																		\
		switch (WRet) {																					\
			case WaitResult::Error: SYSFAIL(_T("Failed to lock " ##objname## ));						\
			case WaitResult::Signaled:																	\
			case WaitResult::Signaled_0: break;															\
			case WaitResult::Signaled_1:																\
			case WaitResult::TimedOut: return false;													\
			default: SYSFAIL(_T("Unable to lock" ##objname## ));										\
		}																								\
	}																									\
	return true;

bool TLockableCS::__Lock_Abortable(WAITTIME Timeout, THandleWaitable *AbortEvent) {
	_IMPL_AbortableLock(__TryLock(), TryEnter(), "critical section");
}

#ifdef __ZWUTILS_SYNC_SLIMRWLOCK

TLockableSRW::TSRWLockInfo TLockableSRW::__ReadLockInfo = { false };
TLockableSRW::TSRWLockInfo TLockableSRW::__WriteLockInfo = { true };

bool TLockableSRW::__Lock_Read_Do(TInterlockedArchInt &WaitCnt, TEvent &WaitEvent, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	_IMPL_AbortableLock(__TryLock_Read(), __Lock_Read_Probe(1), "SRW lock");
}

bool TLockableSRW::__Lock_Write_Do(TInterlockedArchInt &WaitCnt, TEvent &WaitEvent, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	_IMPL_AbortableLock(__TryLock_Write(), __Lock_Write_Probe(1), "SRW lock");
}

#endif