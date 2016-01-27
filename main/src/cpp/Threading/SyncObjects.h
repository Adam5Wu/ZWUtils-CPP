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
 * @brief Synchronized Objects
 * @author Zhenyu Wu
 * @date Jan 08, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_SyncObj_H
#define ZWUtils_SyncObj_H

#include "Misc/Global.h"
#include "Misc/TString.h"

#include "Memory/Allocator.h"
#include "Memory/ManagedRef.h"

#include "Debug/Exception.h"
#include "Debug/SysError.h"

#include "SyncElements.h"

//#define __LOCK_DEBUG(s)	s

#ifndef DBGVV
#ifdef __LOCK_DEBUG
#undef __LOCK_DEBUG
#endif
#endif

#ifndef __LOCK_DEBUG
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
	virtual bool __Lock(TWaitable *AbortEvent)
	{ FAIL(_T("Abstract function")); }
	/**
	 * Try to acquire the lock in this instant, return false if failed
	 **/
	virtual bool __TryLock(void)
	{ FAIL(_T("Abstract function")); }
	/**
	 * Release acquired lock
	 **/
	virtual void __Unlock(void)
	{ FAIL(_T("Abstract function")); }

	__LOCK_DEBUG(TInterlockedOrdinal32<int> __Cnt{0});

public:
	virtual ~TLockable(void)
	{ __LOCK_DEBUG(if (~__Cnt) LOG(_T("There are %d unreleased locks!"), ~__Cnt)); }

	// WARNING: While Lockable is thread-safe (better be), the objects are NOT!
	// !!! DO NOT operate one Lock object with more than one threads !!!
	class TLock {
		typedef TLock _this;
		friend class TLockable;

	private:
		TLockable &Instance;

	protected:
		TLock(TLockable &xInstance, bool const &xLocked) : Instance(xInstance), Locked(xLocked) {
			__LOCK_DEBUG(
				if (!__IN_LOG && xLocked) {
					LOGVV(_T("%s"), TStringCast(_T("+ Locked @") << (void*)&xInstance
						<< _T(" (") << ++xInstance.__Cnt << _T(')')).c_str());
				});
		}

	public:
		bool const Locked;

		// Copy construction does not make sense, because the lock maybe non-reentrant
		TLock(_this const &) = delete;
		// Move construction is OK, because we may want to return local lock
		TLock(_this &&xLock) : Instance(xLock.Instance), Locked(xLock.Locked)
		{ *const_cast<bool*>(&xLock.Locked) = false; }

		virtual ~TLock(void)
		{ Release(); }

		bool Release(void) {
			if (Locked) {
				Instance.__Unlock();
				__LOCK_DEBUG(
					if (!__IN_LOG) {
						LOGVV(_T("%s"), TStringCast(_T("- ") << (--Instance.__Cnt ? _T("Locked") : _T("Unlocked"))
							<< _T(" @") << &Instance << _T(" (") << ~Instance.__Cnt << _T(')')).c_str())
					});
				return true;
			}
			return false;
		}

		// Assignment operations are wacky, the meaning is hard to reason, therefore better disable it
		_this& operator=(_this const &) = delete;
		_this& operator=(_this &&) = delete;

		operator bool() const
		{ return Locked; }

		bool For(TLockable const &rInstance) const
		{ return std::addressof(Instance) == std::addressof(rInstance); }
	};

	virtual TLock Lock(TWaitable *AbortEvent = nullptr)
	{ return{*this, __Lock(AbortEvent)}; }

	virtual TLock TryLock(void)
	{ return{*this, __TryLock()}; }

	TLock NullLock(void)
	{ return{*this, false}; }
};

template<class T>
/**
 * @ingroup Threading
 * @brief Lockable adapter for synchronization premises
 **/
class TLockableSyncPerms : public TLockable {
	typedef TLockableSyncPerms _this;
protected:
	//! This premise is used to implement required operations
	ManagedRef<T> _SyncPerms;
	TLockableSyncPerms(T *xSync) :
		_SyncPerms(xSync != nullptr ? xSync : DefaultObjAllocator<T>().Create(), CONSTRUCTION::HANDOFF) {}
};

// !Lockable using critical section
class TLockableCS : public TLockableSyncPerms < TCriticalSection > {
	typedef TLockableCS _this;

protected:
	bool __Lock(TWaitable *AbortEvent) override
	{ return (AbortEvent && AbortEvent->WaitFor(0) == WaitResult::Signaled) ? false : (_SyncPerms->Enter(), true); }
	bool __TryLock(void) override
	{ return _SyncPerms->TryEnter(); }
	void __Unlock(void) override
	{ _SyncPerms->Leave(); }

public:
	TLockableCS(TCriticalSection *xSync = nullptr) : TLockableSyncPerms(xSync) {}
};

template<class W>
// !Lockable using waitable synchronization premises
class TLockableHandleWaitable : public TLockableSyncPerms < W > {
	ENFORCE_DERIVE(THandleWaitable, W);
	typedef TLockableHandleWaitable _this;

protected:
	bool __Lock(TWaitable *AbortEvent) override {
		if (AbortEvent) {
			WaitResult Ret = WaitMultiple({{*_SyncPerms, *dynamic_cast<THandleWaitable*>(AbortEvent)}}, false);
			switch (Ret) {
				case WaitResult::Error: SYSFAIL(_T("Failed to lock synchronization premises"));
				case WaitResult::Signaled_0: return true;
				case WaitResult::Signaled_1: return false;
				default: SYSFAIL(_T("Unable to lock synchronization premises"));
			}
		} else {
			WaitResult Ret = _SyncPerms->WaitFor();
			switch (Ret) {
				case WaitResult::Error: SYSFAIL(_T("Failed to lock synchronization premises"));
				case WaitResult::Signaled: return true;
				default: SYSFAIL(_T("Unable to lock synchronization premises"));
			}
		}
	}
	bool __TryLock(void) override {
		WaitResult Ret = _SyncPerms->WaitFor(0);
		switch (Ret) {
			case WaitResult::Error: SYSFAIL(_T("Failed to lock synchronization premises"));
			case WaitResult::Signaled: return true;
			case WaitResult::TimedOut: return false;
			default: SYSFAIL(_T("Unable to lock synchronization premises"));
		}
	}
	TLockableHandleWaitable(W *xSync = nullptr) : TLockableSyncPerms(xSync) {}
};

// !Lockable using semaphore
class TLockableSemaphore : public TLockableHandleWaitable < TSemaphore > {
	typedef TLockableSemaphore _this;

protected:
	void __Unlock(void) override
	{ _SyncPerms->Signal(); }

public:
	TLockableSemaphore(TSemaphore *xSync = nullptr) : TLockableHandleWaitable(xSync) {}
};

// !Lockable using mutex
class TLockableMutex : public TLockableHandleWaitable < TMutex > {
	typedef TLockableMutex _this;

protected:
	void __Unlock(void) override
	{ _SyncPerms->Release(); }

public:
	TLockableMutex(TMutex *xSync = nullptr) : TLockableHandleWaitable(xSync) {}
};

/**
 * @ingroup Threading
 * @brief Generic inline wrapper for synchronizing any object
 *
 * Wraps around an object with a lockable class to protect asynchronous accesses
 **/
template<class TObject, class L = TLockableCS>
class TSyncObj : protected TObject, public TLockable {
	ENFORCE_DERIVE(TLockable, L);
	typedef TSyncObj _this;

protected:
	ManagedRef<L> _Lockable;

public:
	typedef IObjAllocator<L> TLAlloc;

	template<typename... Params>
	TSyncObj(TLAlloc &xLAlloc = DefaultObjAllocator<L>(), Params&&... xParams) :
		TObject(xParams...), _Lockable(CONSTRUCTION::EMPLACE, xLAlloc) {}

	template<typename... Params>
	TSyncObj(L *xLock, TLAlloc &xLAlloc = DefaultObjAllocator<L>(), Params&&... xParams) :
		TObject(xParams...), _Lockable(xLock, xLAlloc) {}

	template<
		typename X, typename... Params,
		typename = std::enable_if<!std::is_assignable<X, TLAlloc&>::value>::type,
		typename = std::enable_if<!std::is_assignable<X, L*>::value>::type
	>
	TSyncObj(X &&xParam, Params&&... xParams) :
	TSyncObj(DefaultObjAllocator<L>(), xParam, xParams...) {}

	// Special use copy constructor: Create a copy of the object w/ shared lock with original
	// Caution: Normally you don't want to use this constructor!
	TSyncObj(_this const &xSyncObj) : TObject(xSyncObj), _Lockable(xSyncObj._Lockable) {}
	// Move constructor
	TSyncObj(_this &&xSyncObj) : TObject(std::move(xSyncObj)), _Lockable(std::move(xSyncObj._Lockable)) {}

	// Assignment operations are wacky, the meaning is hard to reason, therefore better disable it
	_this& operator=(_this const &) = delete;
	_this& operator=(_this &&) = delete;

	class Accessor : public Reference < TObject > {
		typedef Accessor _this;
		friend TSyncObj;

	protected:
		TSyncObj *_Obj;
		TLock _Lock;

	protected:
		Accessor(TSyncObj *xObj, TLock &&xLock) : _Obj(xObj), _Lock(std::move(xLock)) {}

		MEMBERFUNC_PROBE(toString);

		template<typename X = TObject>
		auto _toString(void) const -> decltype(std::enable_if<Has_toString<X>::value, TString>::type())
		{ return _Lock ? TStringCast(_T("#SObj(L):") << _Obj.toString()) : TStringCast(_T("#SObj(U)@") << (void*)&_Obj); }

		template<typename X = TObject, typename = void>
		auto _toString(void) const -> decltype(std::enable_if<!Has_toString<X>::value, TString>::type())
		{ return TStringCast(_T("#SObj(") << (_Lock ? _T('L') : _T('U')) << _T("):") << (void*)&_Obj); }

		TObject* _ObjPointer(void) const override
		{ if (!_Lock.Locked) FAIL(_T("Lock not engaged")); return _Obj; }

		TObject* _ObjExchange(TObject *xObj) override
		{ FAIL(_T("Unsupported Operation")); }

	public:
		// Move construction is OK, because we may want to return local lock
		Accessor(_this &&xSyncedObj) : _Obj(xSyncedObj._Obj), _Lock(std::move(xSyncedObj._Lock)) {}

		TString toString(void) const
		{ return _toString(); }

		bool Valid(void) const
		{ return _Lock.Locked; }
		operator bool() const
		{ return Valid(); }
	};
	typedef ManagedRef<Accessor> MRSyncedObj;

	L& Lockable(void)
	{ return *_Lockable; }

	TLock Lock(TWaitable *AbortEvent = nullptr) override
	{ return _Lockable->Lock(AbortEvent); }

	TLock TryLock(void) override
	{ return _Lockable->TryLock(); }

	/**
	 * Lock and return an reference of managed T instance
	 **/
	virtual Accessor Pickup(TWaitable *AbortEvent = nullptr)
	{ return{this, Lock(AbortEvent)}; }
	/**
	 * Try to lock and return a pointer to managed T instance, NULL if could not obtain lock
	 **/
	virtual Accessor TryPickup(void)
	{ return{this, TryLock()}; }

};

#endif