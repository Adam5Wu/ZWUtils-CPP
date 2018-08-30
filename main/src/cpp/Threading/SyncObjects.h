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
 * @brief Synchronized Objects
 * @author Zhenyu Wu
 * @date Jan 08, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_SyncObj_H
#define ZWUtils_SyncObj_H

 // Project global control 
#include "Misc/Global.h"

#include "Misc/TString.h"

#include "Memory/Allocator.h"
#include "Memory/ManagedRef.h"

#include "Debug/Exception.h"
#include "Debug/SysError.h"

#include "SyncElements.h"

//#define __LOCK_DEBUG

#ifdef __LOCK_DEBUG
#undef __LOCK_DEBUG
#define __LOCK_DEBUG(s)	s
#else
#define __LOCK_DEBUG(s)	;
#endif

__LOCK_DEBUG(extern __declspec(thread) bool __IN_LOG);

/**
 * @ingroup Threading
 * @brief Lockable class
 *
 * Abstract class for suppoting lock operations
 **/
class TLockable {
	typedef TLockable _this;

protected:
	/**
	 * Acquire the lock, wait forever
	 **/
	virtual bool __Lock(WAITTIME Timeout, THandleWaitable *AbortEvent) {
		FAIL(_T("Abstract function"));
	}

	/**
	 * Try to acquire the lock in this instant, return false if failed
	 **/
	virtual bool __TryLock(__ARC_UINT Spin) {
		FAIL(_T("Abstract function"));
	}

	class TLockInfo {};

	/**
	 * Release acquired lock
	 **/
	virtual void __Unlock(TLockInfo *LockInfo) {
		FAIL(_T("Abstract function"));
	}

	__LOCK_DEBUG(TInterlockedOrdinal32<int> __Cnt{ 0 });

public:
	// WARNING: While Lockable is thread-safe (better be), the lock objects are NOT!
	// !!! DO NOT operate one Lock object with more than one threads !!!
	// Note: For performance reasons, we do not have a virtual destructor
	// Hence we seal this class and do not allow further derivation
	class TLock final : public ManagedObj {
		typedef TLock _this;
		friend class TLockable;

	protected:
		TLockable * InstRef;
		TLockInfo *__Info = nullptr;

		TLock(TLockable *xInstRef, TLockInfo *Info) : InstRef(xInstRef), __Info(Info) {
			__LOCK_DEBUG({
				int NewCnt = ++Instance->__Cnt;
				if (!__IN_LOG && __Info) {
					LOGVV(_T("%s"), TStringCast(_T("+ Locked @") << Instance
						<< _T(" (") << NewCnt << _T(')')).c_str());
				}
				});
		}

		void __Release(void) {
			if (__Info) {
				InstRef->__Unlock(__Info);
				__LOCK_DEBUG({
					int NewCnt = --Instance->__Cnt;
					if (!__IN_LOG) {
						LOGVV(_T("%s"), TStringCast(_T("- ") << (NewCnt ? _T("Locked") : _T("Unlocked"))
							<< _T(" @") << Instance << _T(" (") << NewCnt << _T(')')).c_str())
					}
					});
			}
		}

		TLockInfo* __Drop(void) {
			TLockInfo *Ret = __Info;
			return __Info = nullptr, Ret;
		}
		
	public:
		// Copy construction does not make sense, because the lock maybe non-reentrant
		TLock(_this const &) = delete;

		TLock(_this &&xLock) : InstRef(xLock.InstRef), __Info(xLock.__Info) {
			xLock.__Info = nullptr;
		}

		~TLock(void) {
			__Release();
		}

		// Copy assignment operations are not meaningful
		_this& operator=(_this const &) = delete;

		_this& operator=(_this &&xLock) {
			__Release();
			InstRef = xLock.InstRef;
			__Info = xLock.__Drop();
			return *this;
		}

		operator bool() const {
			return __Info;
		}

		bool For(TLockable const &rInstance) const {
			return rInstance.Of(*this);
		}
	};

protected:
	TLock __New_Lock(TLockInfo *LockInfo) {
		return TLock(this, LockInfo);
	}

	TLock& __Adopt(TLock &Lock) {
		return Lock.InstRef = this, Lock;
	}

	static TLockable* __SyncInst(TLock const &Lock) {
		return Lock.InstRef;
	}

	static TLockInfo* __LockInfo(TLock const &Lock) {
		return Lock.__Info;
	}

	static TLockInfo* __LockDrop(TLock &Lock) {
		return Lock.__Drop();
	}
	
	static void __Cascade_Unlock(TLockable *Inst, TLockInfo *LockInfo) {
		Inst->__Unlock(LockInfo);
	}

public:
	virtual ~TLockable(void) {
		__LOCK_DEBUG(if (~__Cnt) LOG(_T("There are %d unreleased locks!"), ~__Cnt));
	}

	virtual TLock Lock(WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		return __New_Lock(__Lock(Timeout, AbortEvent) ? (TLockInfo*)-1 : nullptr);
	}

	virtual TLock TryLock(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) {
		return __New_Lock(__TryLock(SpinCount) ? (TLockInfo*)-1 : nullptr);
	}

	bool Of(TLock const &Lock) const {
		return __SyncInst(Lock) == this;
	}

	bool By(TLock const &Lock) const {
		return Lock && Of(Lock);
	}

	TLock NullLock(void) const {
		return const_cast<_this*>(this)->__New_Lock(nullptr);
	}
};

template<class T>
/**
 * @ingroup Threading
 * @brief Lockable adapter for synchronization premises
 **/
class TLockableSyncPerms : public TLockable, protected T {
	typedef TLockableSyncPerms _this;
public:
	TLockableSyncPerms(void) {}
	TLockableSyncPerms(T && xSync) : T(std::move(xSync)) {}

	T& operator*(void) {
		return *this;
	}
};

// !Lockable using critical section
class TLockableCS : public TLockableSyncPerms<TCriticalSection> {
	typedef TLockableCS _this;
private:
	TInterlockedArchInt WaitCnt = 0;
	TEvent WaitEvent = TEvent(CONSTRUCTION::DEFER);

	void __Signal_Event(TEvent &Event) {
		// We are in a transient state of deferred event creation
		// Just hold the breath for a while
		while (!Event.Allocated()) SwitchToThread();
		Event.Set();
	}

	bool __Lock_Abortable(WAITTIME Timeout, THandleWaitable *AbortEvent);

protected:
	bool __Lock(WAITTIME Timeout, THandleWaitable *AbortEvent) override {
		if (Timeout != FOREVER || AbortEvent)
			return __Lock_Abortable(Timeout, AbortEvent);
		// Fallback to legacy CS behavior
		return Enter(), true;
	}

	bool __TryLock(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) override {
#ifdef _DEBUG
		if (!SpinCount) FAIL(_T("Spin count must be a natrual number"));
#endif
		while (--SpinCount && !TryEnter());
		return SpinCount || TryEnter();
	}

	void __Unlock(TLockInfo *LockInfo) override {
		Leave();
		if (~WaitCnt) __Signal_Event(WaitEvent);
	}
};

#ifdef __ZWUTILS_SYNC_SLIMRWLOCK

class TLockableSRW : public TLockableSyncPerms<TSRWLock> {
	typedef TLockableSRW _this;
private:
	TInterlockedArchInt RWaitCnt = 0;
	TInterlockedArchInt WWaitCnt = 0;
	TEvent RWaitEvent = TEvent(CONSTRUCTION::DEFER);
	TEvent WWaitEvent = TEvent(CONSTRUCTION::DEFER);

	void __Signal_Event(TEvent &Event) {
		// We are in a transient state of deferred event creation
		// Just hold the breath for a while
		while (!Event.Allocated()) SwitchToThread();
		Event.Set();
	}
public:
	bool ReadYieldToWrite = false;

protected:
	bool __Lock_Read_Do(TInterlockedArchInt &WaitCnt, TEvent &WaitEvent, WAITTIME Timeout, THandleWaitable *AbortEvent);
	bool __Lock_Write_Do(TInterlockedArchInt &WaitCnt, TEvent &WaitEvent, WAITTIME Timeout, THandleWaitable *AbortEvent);

	bool __Lock_Read_Probe(__ARC_UINT SpinCount) {
		if (ReadYieldToWrite && ~WWaitCnt) return false;
		if (TryRead(SpinCount)) {
			if (~RWaitCnt && (!ReadYieldToWrite || !~WWaitCnt)) {
				__Signal_Event(RWaitEvent);
			}
			return true;
		}
		return false;
	}
	bool __Lock_Read(WAITTIME Timeout, THandleWaitable *AbortEvent) {
		return __Lock_Read_Do(RWaitCnt, RWaitEvent, Timeout, AbortEvent);
	}

	bool __Lock_Write_Probe(__ARC_UINT SpinCount) {
		return TryWrite(SpinCount);
	}
	bool __Lock_Write(WAITTIME Timeout, THandleWaitable *AbortEvent) {
		return __Lock_Write_Do(WWaitCnt, WWaitEvent, Timeout, AbortEvent);
	}

	bool __TryLock_Read(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) {
		return __Lock_Read_Probe(SpinCount);
	}
	bool __TryLock_Write(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) {
		return __Lock_Write_Probe(SpinCount);
	}

#define __Impl_Unlock(relfunc)							\
	relfunc();											\
	if (~WWaitCnt) __Signal_Event(WWaitEvent);			\
	else if (~RWaitCnt) __Signal_Event(RWaitEvent)

	void __Unlock_Read(void) {
		__Impl_Unlock(EndRead);
	}
	void __Unlock_Write(void) {
		__Impl_Unlock(EndWrite);
	}

	class TSRWLockInfo : public TLockInfo {
	public:
		bool const Exclusive;
		TSRWLockInfo(bool xExclusive) : Exclusive(xExclusive) {}
	};

	static TSRWLockInfo __ReadLockInfo;
	static TSRWLockInfo __WriteLockInfo;

	void __Unlock(TLockInfo *LockInfo) override {
		TSRWLockInfo *__Info = static_cast<TSRWLockInfo*>(LockInfo);
		__Info->Exclusive ? __Unlock_Write() : __Unlock_Read();
	}

public:
	TLock Lock_Read(WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		return __New_Lock(__Lock_Read(Timeout, AbortEvent) ? &__ReadLockInfo : nullptr);
	}

	TLock Lock_Write(WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		return __New_Lock(__Lock_Write(Timeout, AbortEvent) ? &__WriteLockInfo : nullptr);
	}

	TLock Lock(WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) override {
		return Lock_Write(Timeout, AbortEvent);
	}

	TLock TryLock_Read(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) {
		return __New_Lock(__TryLock_Read(SpinCount) ? &__ReadLockInfo : nullptr);
	}

	TLock TryLock_Write(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) {
		return __New_Lock(__TryLock_Write(SpinCount) ? &__WriteLockInfo : nullptr);
	}

	TLock TryLock(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) override {
		return TryLock_Write(SpinCount);
	}

	bool ForWrite(TLock const &Lock) const {
		TSRWLockInfo *__Info = static_cast<TSRWLockInfo*>(TLockable::__LockInfo(Lock));
		return __Info && __Info->Exclusive;
	}

	bool WriteLocked(TLock const &Lock) const {
		TSRWLockInfo *__Info = static_cast<TSRWLockInfo*>(TLockable::__LockInfo(Lock));
		return __Info && __Info->Exclusive;
	}
};

#endif

template<class W>
// !Lockable using waitable synchronization premises
class TLockableHandleWaitable : public TLockableSyncPerms<W> {
	ENFORCE_DERIVE(THandleWaitable, W);
	typedef TLockableHandleWaitable _this;

protected:
	bool __Lock(WAITTIME Timeout, THandleWaitable *AbortEvent) override {
		WaitResult WRet = AbortEvent ?
			WaitMultiple({ *(THandleWaitable*)this, *AbortEvent }, false, Timeout) :
			WaitFor(Timeout);
		switch (WRet) {
			case WaitResult::Error: SYSFAIL(_T("Failed to lock synchronization premises"));
			case WaitResult::Signaled:
			case WaitResult::Signaled_0: return true;
			case WaitResult::TimedOut:
			case WaitResult::Signaled_1: return false;
			default: SYSFAIL(_T("Unable to lock synchronization premises"));
		}
	}

	bool __TryLock_Once(void) {
		WaitResult WRet = WaitFor(0);
		switch (WRet) {
			case WaitResult::Error: SYSFAIL(_T("Failed to lock synchronization premises"));
			case WaitResult::Signaled: return true;
			case WaitResult::TimedOut: return false;
			default: SYSFAIL(_T("Unable to lock synchronization premises"));
		}
	}

	bool __TryLock(__ARC_UINT SpinCount) override {
#ifdef _DEBUG
		if (!SpinCount) FAIL(_T("Spin count must be a natrual number"));
#endif
		while (--SpinCount && !__TryLock_Once());
		return SpinCount || __TryLock_Once();
	}
};

// !Lockable using semaphore
class TLockableSemaphore : public TLockableHandleWaitable<TSemaphore> {
	typedef TLockableSemaphore _this;

protected:
	void __Unlock(TLockInfo *LockInfo) override {
		Signal();
	}
};

// !Lockable using mutex
class TLockableMutex : public TLockableHandleWaitable<TMutex> {
	typedef TLockableMutex _this;

protected:
	void __Unlock(TLockInfo *LockInfo) override {
		Release();
	}
};

//#define __SyncObj_Lite

/**
 * @ingroup Threading
 * @brief Generic inline wrapper for synchronizing any object
 *
 * Wraps around an object with a lockable class to protect asynchronous accesses
 **/
#ifdef __SyncObj_Lite

#pragma WARNING("Lite version of SyncObj does not support accordinated synchronization (shared lock)")

template<class TObject, class L = TLockableCS>
class TSyncObj : public L {
	ENFORCE_DERIVE(TLockable, L);
	typedef TSyncObj _this;

protected:
	TObject _Instance;

public:
	TSyncObj(void) {}

	template<typename... Params>
	TSyncObj(L &&xLock, Params&&... xParams) :
		_Instance(std::forward<Params>(xParams)...), L(std::move(xLock)) {}

	template<
		typename X, typename... Params,
		typename = typename std::enable_if<!std::is_assignable<X, L&&>::value>::type
	>
		TSyncObj(X &&xParam, Params&&... xParams) :
		_Instance(std::forward<X>(xParam), std::forward<Params>(xParams)...) {}

	// Copy and move constructions are hard to reason, therefore better disable it
	TSyncObj(_this const &xSyncObj) = delete;
	TSyncObj(_this &&xSyncObj) = delete;

	// Assignment operations are wacky, the meaning is hard to reason, therefore better disable it
	_this& operator=(_this const &) = delete;
	_this& operator=(_this &&) = delete;

	// Note: For performance reasons, we do not have a virtual destructor
	// Hence we seal this class and do not allow further derivation
	class Accessor final {
		typedef Accessor _this;
		friend class TSyncObj<TObject, L>;

	protected:
		using TLock = typename L::TLock;
		TLock _Lock;

		Accessor(TLock &&xLock) : _Lock(std::move(xLock)) {}

		MEMBERFUNC_PROBE(toString);

		TObject* _AccessObjRef(void) const {
			return std::addressof(static_cast<TSyncObj*>(TLockable::__SyncInst(_Lock))->_Instance);
		}

		template<typename X = TObject>
		auto _toString(void) const -> decltype(std::enable_if<Has_toString<X>::value, TString>::type()) {
			return Valid() ? TStringCast(_T("#SObj(L):") << (*this)->toString()) : TStringCast(_T("#SObj(U)") << _AccessObjRef());
		}

		template<typename X = TObject, typename = void>
		auto _toString(void) const -> decltype(std::enable_if<!Has_toString<X>::value, TString>::type()) {
			return TStringCast(_T("#SObj(") << (Valid() ? _T('L') : _T('U')) << _T("):") << _AccessObjRef());
		}

		TObject* _ObjPointer(void) const {
			if (!Valid()) FAIL(_T("Invalid accessor state"));
			return _AccessObjRef();
		}

	public:
		// Move construction for returning accessors
		Accessor(_this &&xAccessor) : _Lock(std::move(xAccessor._Lock)) {}

		TString toString(void) const {
			return _toString();
		}

		bool Valid(void) const {
			return _Lock;
		}

		operator bool() const {
			return Valid();
		}

		// Duplicate partial implementation of Reference to avoid cost of virtual function call
		_this* operator~(void) {
			return this;
		}
		_this const* operator~(void) const {
			return this;
		}

		TObject* operator&(void) const {
			return _ObjPointer();
		}
		TObject& operator*(void) const {
			return *_ObjPointer();
		}
		TObject* operator->(void) const {
			return _ObjPointer();
		}

	};

	/**
	 * Lock and return an accessor of managed T instance
	 **/
	Accessor Pickup(WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		// Clone the implementation of Lock() to avoid cost of virtual function call
		return { Lock(Timeout, AbortEvent) };
	}

	/**
	 * Try to lock and return an accessor of managed T instance, check validty before access
	 **/
	Accessor TryPickup(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) {
		// Clone the implementation of TryLock() to avoid cost of virtual function call
		return { TryLock(Spin) };
	}

	/**
	 * Return an invalid accessor
	 **/
	Accessor NullAccessor(void) {
		return { NullLock() };
	}

};

#else

template<class TObject, class L = TLockableCS>
class TSyncObj : public TLockable {
	ENFORCE_DERIVE(TLockable, L);
	typedef TSyncObj _this;

public:
	typedef ManagedRef<L> MRLockable;
	typedef IObjAllocator<L> TLAlloc;

protected:
	TObject _Instance;
	MRLockable _Lockable;

	void __Unlock(TLockInfo *LockInfo) override {
		__Cascade_Unlock(&_Lockable, LockInfo);
	}

public:
	template<typename... Params>
	TSyncObj(TLAlloc &xLAlloc = DefaultObjAllocator<L>(), Params&&... xParams) :
		_Instance(std::forward<Params>(xParams)...), _Lockable(CONSTRUCTION::EMPLACE, xLAlloc) {}

	template<typename... Params>
	TSyncObj(MRLockable &xLockable, Params&&... xParams) :
		_Instance(std::forward<Params>(xParams)...), _Lockable(xLockable) {}

	template<
		typename X, typename... Params,
		typename = typename std::enable_if<!std::is_assignable<X, TLAlloc&>::value>::type,
		typename = typename std::enable_if<!std::is_assignable<X, MRLockable&>::value>::type
	>
		TSyncObj(X &&xParam, Params&&... xParams) :
		TSyncObj(DefaultObjAllocator<L>(), xParam, std::forward<Params>(xParams)...) {}

	// Copy and move constructions are hard to reason, therefore better disable it
	TSyncObj(_this const &xSyncObj) = delete;
	TSyncObj(_this &&xSyncObj) = delete;

	// Assignment operations are wacky, the meaning is hard to reason, therefore better disable it
	_this& operator=(_this const &) = delete;
	_this& operator=(_this &&) = delete;

	// Note: For performance reasons, we do not have a virtual destructor
	// Hence we seal this class and do not allow further derivation
	class Accessor final {
		typedef Accessor _this;
		friend class TSyncObj<TObject, L>;

	protected:
		TLock _Lock;

		Accessor(TLock &&xLock) : _Lock(std::move(xLock)) {}

		MEMBERFUNC_PROBE(toString);

		TObject* _AccessObjRef(void) const {
			return std::addressof(static_cast<TSyncObj*>(TLockable::__SyncInst(_Lock))->_Instance);
		}

		template<typename X = TObject>
		auto _toString(void) const -> decltype(std::enable_if<Has_toString<X>::value, TString>::type()) {
			return Valid() ? TStringCast(_T("#SObj(L):") << (*this)->toString()) : TStringCast(_T("#SObj(U)") << _AccessObjRef());
		}

		template<typename X = TObject, typename = void>
		auto _toString(void) const -> decltype(std::enable_if<!Has_toString<X>::value, TString>::type()) {
			return TStringCast(_T("#SObj(") << (Valid() ? _T('L') : _T('U')) << _T("):") << _AccessObjRef());
		}

		TObject* _ObjPointer(void) const {
			if (!Valid()) FAIL(_T("Invalid accessor state"));
			return _AccessObjRef();
		}

	public:
		// Move construction for returning accessors
		Accessor(_this &&xAccessor) : _Lock(std::move(xAccessor._Lock)) {}

		TString toString(void) const {
			return _toString();
		}

		bool Valid(void) const {
			return _Lock;
		}

		operator bool() const {
			return Valid();
		}

		// Duplicate partial implementation of Reference to avoid cost of virtual function call
		_this* operator~(void) {
			return this;
		}
		_this const* operator~(void) const {
			return this;
		}

		TObject* operator&(void) const {
			return _ObjPointer();
		}
		TObject& operator*(void) const {
			return *_ObjPointer();
		}
		TObject* operator->(void) const {
			return _ObjPointer();
		}

	};

	TLock Lock(WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) override {
		auto iRet = _Lockable->Lock(Timeout, AbortEvent);
		return std::move(__Adopt(iRet));
	}

	TLock TryLock(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) override {
		auto iRet = _Lockable->TryLock(SpinCount);
		return std::move(__Adopt(iRet));
	}

	/**
	 * Lock and return an accessor of managed T instance
	 **/
	Accessor Pickup(WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		// Clone Lock function to avoid virtual function call cost
		auto iRet = _Lockable->Lock(Timeout, AbortEvent);
		return { std::move(__Adopt(iRet)) };
	}

	/**
	 * Try to lock and return an accessor of managed T instance, check validty before access
	 **/
	Accessor TryPickup(__ARC_UINT SpinCount = DEFAULT_CRITICALSECTION_SPIN) {
		// Clone TryLock function to avoid virtual function call cost
		auto iRet = _Lockable->TryLock(SpinCount);
		return { std::move(__Adopt(iRet)) };
	}

	/**
	 * Return an invalid accessor
	 **/
	Accessor NullAccessor(void) {
		return { NullLock() };
	}

};

#endif

#endif