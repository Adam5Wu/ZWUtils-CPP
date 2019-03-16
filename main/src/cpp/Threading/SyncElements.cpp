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

#include "SyncElements.h"

#include "Debug/SysError.h"

#ifdef WINDOWS
#include <Windows.h>

WAITTIME const FOREVER = INFINITE;

unsigned int WaitSlot(WaitResult const &X, WaitResult const &Base) {
	return (unsigned int)X - (unsigned int)Base;
}

TString WaitResultToString(WaitResult const &WRet) {
	switch (WRet) {
		case WaitResult::Signaled: return _T("Signaled");
		case WaitResult::TimedOut:  return _T("TimedOut");
		case WaitResult::Error: return _T("Error");
		case WaitResult::Abandoned: return _T("Abandoned");
		case WaitResult::APC: return _T("APC");
		case WaitResult::Message: return _T("Message");
		default:
			if (WRet >= WaitResult::Signaled_0 && WRet <= WaitResult::Signaled_MAX) {
				return TStringCast(_T("Signaled #") << WaitSlot_Signaled(WRet));
			} else if (WRet >= WaitResult::Abandoned_0 && WRet <= WaitResult::Abandoned_MAX) {
				return TStringCast(_T("Abandoned #") << WaitSlot_Abandoned(WRet));
			}
	}
	return TStringCast(_T("Unknown Wait Result (") << std::hex << std::uppercase << (unsigned int)WRet << _T(')'));
}

HANDLE DupWaitHandle(HANDLE const &sHandle, HANDLE const &sProcess = GetCurrentProcess(),
					 HANDLE const &tProcess = GetCurrentProcess(), BOOL Inheritable = FALSE) {
	HANDLE Ret;
	if (DuplicateHandle(sProcess, sHandle, tProcess, &Ret, SYNCHRONIZE, Inheritable, 0) == 0)
		SYSFAIL(_T("Failed to duplicate wait handle"));
	return Ret;
}

void SafeCloseHandle(HANDLE const &Handle, bool Assert = false) {
	if (!CloseHandle(Handle)) {
		if (Assert) SYSFAIL(_T("Failed to close handle"));
		SYSERRLOG(_T("WARNING: Failed to close handle"));
	}
}

inline WaitResult __FilterWaitResult(DWORD Ret, size_t ObjCnt, bool WaitAll) {
	if ((Ret >= WAIT_OBJECT_0) && (Ret < WAIT_OBJECT_0 + ObjCnt)) {
		// High frequency event
		__SYNC_DEBUG(if (!__IN_LOG) { LOGVV(_T("NOTE: Wait Succeed")); });
		return WaitAll ?
			WaitResult::Signaled :
			(WaitResult)((int)WaitResult::Signaled_0 - WAIT_OBJECT_0 + Ret);
	} else if (Ret == WAIT_OBJECT_0 + ObjCnt) {
		__SYNC_DEBUG(LOGV(_T("NOTE: Wait Msg")));
		return WaitResult::Message;
	} else if ((Ret >= WAIT_ABANDONED_0) && (Ret < WAIT_ABANDONED_0 + ObjCnt)) {
		LOGVV(_T("WARNING: Wait Succeed (with abandoned mutex)"));
		return WaitAll ? WaitResult::Abandoned :
			(WaitResult)((int)WaitResult::Abandoned_0 - WAIT_ABANDONED_0 + Ret);
	} else if (Ret == WAIT_IO_COMPLETION) {
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

WaitResult WaitMultiple(TWaitables const &Waitables, bool WaitAll, WAITTIME Timeout, bool WaitAPC, bool WaitMsg) {
	class TRawWaitHandles : public std::vector<HANDLE> {
	private:
		std::vector<THandle> Handles;
		void AddHandle(THandle &&Handle) {
			emplace_back(*Handle);
			Handles.emplace_back(std::move(Handle));
		}
	public:
		explicit TRawWaitHandles(TWaitables const &RefWaitables) {
			for (THandleWaitable &Waitable : RefWaitables)
				AddHandle(Waitable.WaitHandle());
		}
	} WaitHandles(Waitables);
	return WaitMultiple(WaitHandles, WaitAll, Timeout, WaitAPC, WaitMsg);
}

WaitResult WaitSingle(HANDLE HWait, WAITTIME Timeout, bool WaitAPC, bool WaitMsg) {
	if (WaitMsg) {
		return WaitMultiple({ HWait }, false, Timeout, WaitAPC, WaitMsg);
	} else {
		DWORD Ret = WaitForSingleObjectEx(HWait, Timeout, WaitAPC);
		return __FilterWaitResult(Ret, 1, true);
	}
}

WaitResult WaitSingle(THandleWaitable &Waitable, WAITTIME Timeout, bool WaitAPC, bool WaitMsg) {
	return WaitSingle(*Waitable.WaitHandle(), Timeout, WaitAPC, WaitMsg);
}

// --- THandleWaitable
WaitResult THandleWaitable::WaitFor(WAITTIME Timeout) const {
	return WaitSingle(const_cast<_this*>(this)->Refer(), Timeout, false, false);
}

THandle THandleWaitable::WaitHandle(void) {
	return WaitOnly ? THandle::Unmanaged(Refer()) : DupWaitable();
}

THandleWaitable THandleWaitable::DupWaitable(void) {
	THandleWaitable Ret([&] { return DupWaitHandle(Refer()); });
	Ret.WaitOnly = true;
	return Ret;
}

THandleWaitable THandleWaitable::Create(THandle &Handle) {
	return { CONSTRUCTION::HANDOFF, DupWaitHandle(*Handle) };
}

// --- TSemaphore
HANDLE DupSemSignalHandle(HANDLE const &sHandle, HANDLE const &sProcess = GetCurrentProcess(),
						  HANDLE const &tProcess = GetCurrentProcess(), BOOL Inheritable = FALSE) {
	HANDLE Ret;
	if (DuplicateHandle(sProcess, sHandle, tProcess, &Ret, SEMAPHORE_MODIFY_STATE, Inheritable, 0) == 0)
		SYSFAIL(_T("Failed to duplicate semaphore signal handle"));
	return Ret;
}

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

THandle TSemaphore::SignalHandle(void) {
	return { [&] { return DupSemSignalHandle(Refer()); } };
}

long TSemaphore::Signal(long Count) {
	LONG PrevCnt;
	if (ReleaseSemaphore(Refer(), Count, &PrevCnt) == 0)
		SYSFAIL(_T("Failed to signal semaphore"));
	return PrevCnt;
}

// --- TMutex
HANDLE TMutex::Create(bool Acquired, TString const &Name) {
	HANDLE Ret = CreateMutex(nullptr, Acquired, Name.empty() ? nullptr : Name.c_str());
	if (Ret == nullptr)
		SYSFAIL(_T("Failed to create mutex"));
	if (!Name.empty()) {
		DWORD ErrCode = GetLastError();
		if (ErrCode == ERROR_ALREADY_EXISTS) {
			LOGV(_T("WARNING: Named mutex '%s' already exists!"), Name.c_str());
		}
	}
	return Ret;
}

void TMutex::Release(void) {
	if (ReleaseMutex(Refer()) == 0)
		SYSFAIL(_T("Failed to release mutex"));
}

// --- TEvent
HANDLE DupEventSignalHandle(HANDLE const &sHandle, HANDLE const &sProcess = GetCurrentProcess(),
							HANDLE const &tProcess = GetCurrentProcess(), BOOL Inheritable = FALSE) {
	HANDLE Ret;
	if (DuplicateHandle(sProcess, sHandle, tProcess, &Ret, EVENT_MODIFY_STATE, Inheritable, 0) == 0)
		SYSFAIL(_T("Failed to duplicate event signal handle"));
	return Ret;
}

HANDLE TEvent::Create(bool ManualReset, bool Initial, TString const &Name) {
	HANDLE Ret = CreateEvent(nullptr, ManualReset, Initial, Name.empty() ? nullptr : Name.c_str());
	if (Ret == nullptr)
		SYSFAIL(_T("Failed to create event"));
	if (!Name.empty()) {
		DWORD ErrCode = GetLastError();
		if (ErrCode == ERROR_ALREADY_EXISTS) {
			LOGV(_T("WARNING: Named event '%s' already exists!"), Name.c_str());
		}
	}
	return Ret;
}

THandle TEvent::SignalHandle(void) {
	return { [&] { return DupEventSignalHandle(Refer()); } };
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

#if (_WIN32_WINNT >= 0x0600)

// --- TConditionVariable

WaitResult TConditionVariable::WaitFor(TCriticalSection &CS, WAITTIME Timeout) {
	if (!SleepConditionVariableCS(&rConditionVariable, &CS.rCriticalSection, Timeout)) {
		DWORD ErrCode = GetLastError();
		if (ErrCode == ERROR_TIMEOUT) return WaitResult::TimedOut;

		SetLastError(ErrCode);
		return WaitResult::Error;
	}
	return WaitResult::Signaled;
}

WaitResult TConditionVariable::WaitFor(TSRWLock &SRW, bool isWriting, WAITTIME Timeout) {
	if (!SleepConditionVariableSRW(&rConditionVariable, &SRW.rSRWlock, Timeout,
		(isWriting ? 0 : CONDITION_VARIABLE_LOCKMODE_SHARED))) {
		DWORD ErrCode = GetLastError();
		if (ErrCode == ERROR_TIMEOUT) return WaitResult::TimedOut;

		SetLastError(ErrCode);
		return WaitResult::Error;
	}
	return WaitResult::Signaled;
}

void TConditionVariable::Signal(bool All) {
	if (!All) WakeConditionVariable(&rConditionVariable);
	else WakeAllConditionVariable(&rConditionVariable);
}

#endif

#include "Threading/WorkerThread.h"

// --- TAlarmClock

class TAlarmClock_Impl : public TAlarmClock {
private:
	class TClockRunner;
	typedef ManagedRef<TClockRunner> MRClockRunner;
	MRClockRunner ClockRunner;
	TLockableCS CallbackLock;

	class TClockRunner : public TRunnable, public ManagedObj {
	private:
		TimeStamp _ExitTS;
		TAlarmCallback _Callback;
		TEvent _AbortEvent;
		bool _CallAtStop;

		TAlarmClock_Impl &_Parent;
	public:
		TClockRunner(TimeStamp const &ExitTS, TAlarmCallback const &Callback, TAlarmClock_Impl &Parent) :
			_ExitTS(ExitTS), _Callback(Callback), _AbortEvent(true), _CallAtStop(true), _Parent(Parent) {
		}

		virtual TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) override {
			TimeStamp CurTS = TimeStamp::Now();
			TimeSpan Remainder = _ExitTS - CurTS;
			WaitResult WRet = WaitResult::Signaled;

			{
				TInitResource<int> StopAction(
					0,
					[&](int &) {
						auto CBLock = _Parent.CallbackLock.Lock();
						_Parent.ClockRunner.Clear();
						if (_CallAtStop && _Callback) _Callback(_ExitTS);
					});

				DEBUGV_DO(
					{
					LOG(_T("Alarm clock armed @ %s"), CurTS.toString().c_str());
					LOG(_T("- Scheduled wake up in: %s"), Remainder.toString(TimeUnit::MSEC).c_str());
					}
				);

				try {
					bool Interrupted = false;
					while (Remainder > TimeSpan::Null && !Interrupted) {
						// Prevent too large duration value hit INTINIE
						INT64 Timeout = std::min(Remainder.GetValue(TimeUnit::MSEC), 0x7FFFFFFFLL);
						WRet = _AbortEvent.WaitFor((DWORD)Timeout);

						CurTS = TimeStamp::Now();
						Remainder = _ExitTS - CurTS;
						switch (WRet) {
							case WaitResult::Signaled:
								// Abort event signaled
								Interrupted = true;
								break;
							case WaitResult::TimedOut:
								// Finished one round of waiting
								break;
							default:
								// Unexpected
								LOGV(_T("WARNING: Unexpected clock runner wait result (%s)"), WaitResultToString(WRet).c_str());
								_CallAtStop = false;
								Interrupted = true;
								break;
						}
					}
				} catch (_ECR_ e) {
					e.Show();

					WRet = WaitResult::Error;
					CurTS = TimeStamp::Now();
					Remainder = _ExitTS - CurTS;
					_CallAtStop = false;
				}
			}

			DEBUGV_DO(
				LOG(_T("Alarm clock unarmed [%s] @ %s (event %s)"), WaitResultToString(WRet).c_str(),
					CurTS.toString().c_str(), _CallAtStop ? _T("fired") : _T("not-fired"));
			if (Remainder) {
				LOG(_T("- Wake up time compared with deadline: %s"),
					Remainder.toString(TimeUnit::MSEC).c_str());
			}
			);

			return {};
		}

		virtual void StopNotify(TWorkerThread &WorkerThread) override {
			SignalStop(false);
		}

		void SignalStop(bool TriggerCallback) {
			_CallAtStop = TriggerCallback;
			_AbortEvent.Set();
		}

	};

public:
	TAlarmClock_Impl(void) {}

	virtual ~TAlarmClock_Impl(void) override {
		Disarm(true);
	}

	virtual void Arm(TimeStamp const &Clock, TAlarmCallback const &Callback) override {
		if (Armed()) FAIL(_T("Clock already armed!"));

		ClockRunner = { CONSTRUCTION::EMPLACE, Clock, Callback, *this };
		TWorkerThread::Create(_T("AlarmClock"), MRRunnable{ &ClockRunner }, true)->Start();
	}

	virtual bool Armed(void) const override {
		return !ClockRunner.Empty();
	}

	virtual bool Fire(bool WaitFor) {
		auto CBLock = CallbackLock.Lock();
		MRClockRunner RefRunner = ClockRunner;
		if (!RefRunner.Empty()) {
			ClockRunner->SignalStop(true);
			CBLock = CallbackLock.NullLock();

			// Wait for runner to finish
			while (WaitFor && Armed()) {
				SwitchToThread();
			}
			return true;
		}
		return false;
	}

	virtual bool Disarm(bool WaitFor) {
		auto CBLock = CallbackLock.Lock();
		MRClockRunner RefRunner = ClockRunner;
		if (!RefRunner.Empty()) {
			ClockRunner->SignalStop(false);
			CBLock = CallbackLock.NullLock();

			// Wait for runner to finish
			while (WaitFor && Armed()) {
				SwitchToThread();
			}
			return true;
		}
		return false;
	}

};

MRAlarmClock TAlarmClock::Create(void) {
	return { DEFAULT_NEW(TAlarmClock_Impl), CONSTRUCTION::HANDOFF };
}

// --- TWaitableAlarmClock

void TWaitableAlarmClock::Arm(TimeStamp const &Clock) {
	if (_Alarm->Armed()) FAIL(_T("Clock already armed!"));

	_WaitEvent.Reset();
	_WaitRet = WaitResult::TimedOut;
	_Alarm->Arm(
		Clock,
		[&](TimeStamp const &DueTS) {
			_WaitRet = WaitResult::Signaled;
			_WaitEvent.Set();
		});
}

bool TWaitableAlarmClock::Fire(void) {
	return _Alarm->Fire(true);
}

bool TWaitableAlarmClock::Disarm(void) {
	if (_Alarm->Disarm(true)) {
		_WaitRet = WaitResult::Abandoned;
		_WaitEvent.Set();
		return true;
	}
	return false;
}

WaitResult TWaitableAlarmClock::WaitFor(WAITTIME Timeout) const {
	WaitResult WRet = _WaitEvent.WaitFor(Timeout);
	switch (WRet) {
		case WaitResult::Signaled:
			WRet = _WaitRet;
			break;
	}
	return WRet;
}

#endif
