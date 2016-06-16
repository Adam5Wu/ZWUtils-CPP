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

// [Utilities] Synchronization Premises

#include "SyncElements.h"

#include "Debug/SysError.h"

#ifdef WINDOWS
#include <Windows.h>

WAITTIME const FOREVER = INFINITE;

HANDLE DupWaitHandle(HANDLE const &sHandle, HANDLE const &sProcess = GetCurrentProcess(),
					 HANDLE const &tProcess = GetCurrentProcess(), BOOL Inheritable = FALSE) {
	HANDLE Ret;
	if (DuplicateHandle(sProcess, sHandle, tProcess, &Ret, SYNCHRONIZE, Inheritable, 0) == 0)
		SYSFAIL(_T("Failed to duplicate wait handle"));
	return Ret;
}

void SafeCloseHandle(HANDLE const &Handle, bool Assert = false) {
	if (CloseHandle(Handle) == 0)
		SYSERRLOG(_T("WARNING: Failed to close handle"));
}

inline WaitResult __FilterWaitResult(DWORD Ret, size_t ObjCnt, bool WaitAll) {
	if ((Ret >= WAIT_OBJECT_0) && (Ret < WAIT_OBJECT_0 + ObjCnt)) {
		// High frequency event
		__SYNC_DEBUG(if (__IN_LOG) { LOGVV(_T("NOTE: Wait Succeed")); });
		return WaitAll ? WaitResult::Signaled : (WaitResult)((int)WaitResult::Signaled_0 - WAIT_OBJECT_0 + Ret);
	} if (Ret == WAIT_OBJECT_0 + ObjCnt) {
		__SYNC_DEBUG(LOGV(_T("NOTE: Wait Msg")));
		return WaitResult::Message;
	} else if ((Ret >= WAIT_ABANDONED_0) && (Ret < WAIT_ABANDONED_0 + ObjCnt)) {
		LOGVV(_T("WARNING: Wait Succeed (with abandoned mutex)"));
		return WaitAll ? WaitResult::Abandoned : (WaitResult)((int)WaitResult::Abandoned_0 - WAIT_ABANDONED_0 + Ret);
	} if (Ret == WAIT_IO_COMPLETION) {
		__SYNC_DEBUG(LOGV(_T("NOTE: Wait APC")));
		return WaitResult::APC;
	} else if (Ret == WAIT_TIMEOUT) {
		// High frequency event
		__SYNC_DEBUG(LOGVV(_T("NOTE: Wait timed out")));
		return WaitResult::TimedOut;
	} else if (Ret == WAIT_FAILED) {
		SYSERRLOG(_T("WARNING: Wait failed"));
		return WaitResult::Error;
	} else {
		LOG(_T("WARNING: Wait possibly failed - Unrecognized return value (%d)"), Ret);
		return WaitResult::Error;
	}
}

WaitResult MsgWaitMultiple(std::vector<HANDLE> const &WaitHandles, bool WaitAll, DWORD WakeMask, WAITTIME Timeout, bool WaitAPC) {
	size_t ObjCnt = WaitHandles.size();
	DWORD WaitFlag = (WaitAll ? MWMO_WAITALL : 0) | (WaitAPC ? MWMO_ALERTABLE : 0) | (WakeMask != 0 ? MWMO_INPUTAVAILABLE : 0);
	DWORD Ret = MsgWaitForMultipleObjectsEx((DWORD)ObjCnt, &WaitHandles.front(), Timeout, WakeMask, WaitFlag);
	return __FilterWaitResult(Ret, ObjCnt, WaitAll);
}

WaitResult WaitMultiple(std::vector<HANDLE> const &WaitHandles, bool WaitAll, WAITTIME Timeout, bool WaitAPC, bool WaitMsg) {
	if (WaitMsg) {
		return MsgWaitMultiple(WaitHandles, WaitAll, QS_ALLEVENTS, Timeout, WaitAPC);
	} else {
		size_t ObjCnt = WaitHandles.size();
		DWORD Ret = WaitForMultipleObjectsEx((DWORD)ObjCnt, &WaitHandles.front(), WaitAll, Timeout, WaitAPC);
		return __FilterWaitResult(Ret, ObjCnt, WaitAll);
	}
}

WaitResult WaitMultiple(std::vector<std::reference_wrapper<THandleWaitable>> const&Waitables, bool WaitAll, WAITTIME Timeout, bool WaitAPC, bool WaitMsg) {
	class TWaitHandles : public std::vector < HANDLE > {
	public:
		~TWaitHandles(void) {
			for (size_type i = 0; i < size(); i++)
				SafeCloseHandle(at(i));
		}
	} WaitHandles;
	for (THandleWaitable &Waitable : Waitables) {
		THandle WaitHandle = Waitable.WaitHandle();
		WaitHandles.push_back(*WaitHandle);
		WaitHandle.Invalidate();
	}
	return WaitMultiple(WaitHandles, WaitAll, Timeout, WaitAPC, WaitMsg);
}

WaitResult WaitSingle(HANDLE HWait, WAITTIME Timeout, bool WaitAPC, bool WaitMsg) {
	if (WaitMsg) {
		return WaitMultiple({{HWait}}, true, Timeout, WaitAPC, WaitMsg);
	} else {
		DWORD Ret = WaitForSingleObjectEx(HWait, Timeout, WaitAPC);
		return __FilterWaitResult(Ret, 1, true);
	}
}

WaitResult WaitSingle(THandleWaitable &Waitable, WAITTIME Timeout, bool WaitAPC, bool WaitMsg) {
	return WaitSingle(*Waitable.WaitHandle(), Timeout, WaitAPC, WaitMsg);
}

// THandleWaitable
WaitResult THandleWaitable::WaitFor(WAITTIME Timeout) {
	return WaitSingle(Refer(), Timeout, false, false);
}

THandle THandleWaitable::WaitHandle(void) {
	return{[&] {return DupWaitHandle(Refer()); }, [](HANDLE &X) {SafeCloseHandle(X); }};
}

// TSemaphore
HANDLE TSemaphore::Create(long Initial, long Maximum, TString const &Name) {
	HANDLE Ret = CreateSemaphore(nullptr, Initial, Maximum, Name.empty() ? nullptr : Name.c_str());
	if (Ret == nullptr)
		SYSFAIL(_T("Failed to create semaphore"));
	if (!Name.empty()) {
		DWORD ErrCode = GetLastError();
		if (ErrCode == ERROR_ALREADY_EXISTS) {
			LOGV(_T("WARNING: Named semaphore '%s' already exists!"), Name.c_str());
		}
	}
	return Ret;
}

void TSemaphore::Destroy(HANDLE const &rSemaphore) {
	SafeCloseHandle(rSemaphore);
}

long TSemaphore::Signal(long Count) {
	LONG PrevCnt;
	if (ReleaseSemaphore(Refer(), Count, &PrevCnt) == 0)
		SYSFAIL(_T("Failed to signal semaphore"));
	return PrevCnt;
}

// TMutex
HANDLE TMutex::Create(bool Acquired, TString const &Name) {
	HANDLE Ret = CreateMutex(nullptr, Acquired, Name.empty() ? nullptr : Name.c_str());
	if (Ret == nullptr)
		SYSFAIL(_T("Failed to create mutex"));
	if (!Name.empty()) {
		DWORD ErrCode = GetLastError();
		if (ErrCode == ERROR_ALREADY_EXISTS) {
			LOGV(_T("WARNING: Named mutex '%s' already exists!"), Name);
		}
	}
	return Ret;
}

void TMutex::Destroy(HANDLE const &rMutex) {
	SafeCloseHandle(rMutex);
}

void TMutex::Release(void) {
	if (ReleaseMutex(Refer()) == 0)
		SYSFAIL(_T("Failed to release mutex"));
}

// TEvent
HANDLE TEvent::Create(bool ManualReset, bool Initial, TString const &Name) {
	HANDLE Ret = CreateEvent(nullptr, ManualReset, Initial, Name.empty() ? nullptr : Name.c_str());
	if (Ret == nullptr)
		SYSFAIL(_T("Failed to create event"));
	if (!Name.empty()) {
		DWORD ErrCode = GetLastError();
		if (ErrCode == ERROR_ALREADY_EXISTS) {
			LOGV(_T("WARNING: Named event '%s' already exists!"), Name);
		}
	}
	return Ret;
}

void TEvent::Destroy(HANDLE const &rEvent) {
	SafeCloseHandle(rEvent);
}

void TEvent::Set(void) {
	if (SetEvent(Refer()) == 0)
		SYSFAIL(_T("Failed to set event"));
}

void TEvent::Reset(void) {
	if (ResetEvent(Refer()) == 0)
		SYSFAIL(_T("Failed to reset event"));
}

void TEvent::Pulse(void) {
	if (PulseEvent(Refer()) == 0)
		SYSFAIL(_T("Failed to pulse event"));
}

#endif
