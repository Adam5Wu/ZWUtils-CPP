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
 * @brief Synchronized Message Queue
 * @author Zhenyu Wu
 * @date Feb 10, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_SyncQueue_H
#define ZWUtils_SyncQueue_H

 // Project global control 
#include "Misc/Global.h"

#include "Misc/TString.h"

#include "Debug/Debug.h"
#include "Debug/Exception.h"

#include "SyncElements.h"
#include "SyncObjects.h"

#include <deque>

// Lite version performs 20% better in low contention scenario, via bypassing SyncObj layer
#define __SDQ_LITE

#define __SDQ_SYNCSPIN	DEFAULT_CRITICALSECTION_SPIN

// Enables iterator access to the underlying deque
// - The queue must be Push+Pop locked, otherwise iterators do not make sense (iterating a changing deque?)
#define __SDQ_ITERATORS

#ifdef __SDQ_ITERATORS
// Enables mutable iterator access to the underlying deque
// - If disabled, only const iterators will be available, and they are concurrently avaiable to all threads;
// - If enabled, without concurrent const iterator support, any iterator will only be available to one thread at a time.
#define __SDQ_MUTABLE_ITERATORS

#ifdef __SDQ_MUTABLE_ITERATORS
// Enables concurrent const iterator support
// - If enabled, mutable iterators will only be available to one thread at a time, while const iterators will be
//     concurrently avaiable to all threads *when there are no active mutable iterator(s)*;
//     * When an active mutable iterator present, only thread that owns it will have concurrent access to const
//       iterators, all other threads have to wait to access either mutable or const iterators.
#define __SDQ_CONCURRENT_CONST_ITERATORS

#endif //__SDQ_MUTABLE_ITERATORS

#endif //__SDQ_ITERATORS

/**
 * @ingroup Threading
 * @brief Synchronized queue
 *
 * Synchronized blocking double-ended queue with no upper limit
 * Note: Currently implementation does not have faireness guarantee
 **/
template<class T>
class TSyncBlockingDeque : public TLockable, public TWaitable {
	typedef TSyncBlockingDeque<T> _this;
	typedef std::deque<T> Container;

#ifdef __SDQ_ITERATORS
protected:
	template<class Iter>
	class __Locked_Iterator : public Iter {
		typedef __Locked_Iterator<Iter> _this;
		friend class TSyncBlockingDeque<T>;

	protected:
		MRLock _LockRef;

		__Locked_Iterator(void) {}

		__Locked_Iterator(Iter &&xIter, MRLock &xLockRef) :
			Iter(std::move(xIter)), _LockRef(xLockRef) {}

		__Locked_Iterator(_this &&xIterator, Iter &&xIter) :
			Iter(std::move(xIter)),
			_LockRef(std::move(xIterator._LockRef)) {}

	public:
		__Locked_Iterator(_this &&xIterator) NOEXCEPT :
			Iter(std::move(xIterator)),
			_LockRef(std::move(xIterator._LockRef)) {}

		_this& operator=(_this &&xIterator) {
			Iter::operator=(std::move(xIterator));
			_LockRef = std::move(xIterator._LockRef);
			return *this;
		}

		bool Valid(void) { return !_LockRef.Empty() && *_LockRef; }
	};
#endif // __SDQ_ITERATORS

public:
	using size_type = typename Container::size_type;

#ifdef __SDQ_ITERATORS

#ifdef __SDQ_MUTABLE_ITERATORS
	typedef __Locked_Iterator<typename Container::iterator> iterator;
	typedef __Locked_Iterator<typename Container::reverse_iterator> reverse_iterator;
#endif
	typedef __Locked_Iterator<typename Container::const_iterator> const_iterator;
	typedef __Locked_Iterator<typename Container::const_reverse_iterator> const_reverse_iterator;
#endif // __SDQ_ITERATORS

protected:
	volatile bool __Cleanup = false;
	//volatile __ARC_INT PopWaiters = 0;
	//volatile __ARC_INT EmptyWaiters = 0;

#ifdef __SDQ_LITE
	TLockableCS _Sync;
	Container _Queue;
	typedef Container* TQueueAccessor;
#else
	typedef TSyncObj<Container> TSyncDeque;
	typedef typename TSyncDeque::Accessor TQueueAccessor;
	TSyncDeque SyncDeque;
#endif

	typedef TInterlockedOrdinal32<long> TSyncCounter;
	TSyncCounter PushHold = 0;
	TSyncCounter PopHold = 0;

	TEvent PushWait = { true, true };
	TEvent PopWait = { true, true };

	TEvent EmptyWait = { true, true };
	TEvent ContentWait = { true, false };

#if defined(__SDQ_ITERATORS) && defined(__SDQ_MUTABLE_ITERATORS)

#ifdef __SDQ_CONCURRENT_CONST_ITERATORS
#ifdef __ZWUTILS_SYNC_SLIMRWLOCK
	TLockableSRW SRWSync;
	TInterlockedArchInt IterWaiters = 0;
	TEvent IterWaitEvent = TEvent(CONSTRUCTION::DEFER);
#else
#error Lack of slim RW lock support, cannot support mutable iterators + concurrent const iterators!
#endif
#else
#pragma WARNING("Iterator concurrency not enabled, iterators will be available to one thread at a time!")
	TLockableCS ExclusiveSync;
#endif

#endif // __SDQ_ITERATORS && __SDQ_MUTABLE_ITERATORS

	class TSDQBaseLockInfo : public TLockInfo {
	public:
		bool const Push, Pop;
		TSDQBaseLockInfo(bool xPush, bool xPop) : Push(xPush), Pop(xPop) {}
#ifdef __SDQ_MUTABLE_ITERATORS
		virtual ~TSDQBaseLockInfo(void) {}
#endif
	};

	static TSDQBaseLockInfo __PushLockInfo;
	static TSDQBaseLockInfo __PopLockInfo;

#ifdef __SDQ_ITERATORS
	template<class Iter>
	using FCIterGetter = Iter(Container::*)(void) const;

#ifdef __SDQ_MUTABLE_ITERATORS
	template<class Iter>
	using FMIterGetter = Iter(Container::*)(void);

#ifdef __SDQ_CONCURRENT_CONST_ITERATORS
#ifdef __ZWUTILS_SYNC_SLIMRWLOCK
	class TSDQDynamicLockInfo : public TSDQBaseLockInfo {
	protected:
		TSDQDynamicLockInfo(bool xPush, bool xPop) : TSDQBaseLockInfo(xPush, xPop) {}
	public:
		virtual void __Release(TSyncBlockingDeque<T> &SDQueue) {};
	};

	class TSDQPushPopLockInfo : public TSDQDynamicLockInfo {
	public:
		TLock * _SharedLock = nullptr;
		TLock * _ExclusiveLock = nullptr;

		TSDQPushPopLockInfo(void) : TSDQDynamicLockInfo(true, true) {}
	};

	class TSDQIterLockInfo : public TSDQDynamicLockInfo {
	public:
		MRLock _IterLock;
		MRLock _SDQLock;

		TSDQIterLockInfo(TLock &&xIterLock, TLock &xSDQLock) :
			TSDQDynamicLockInfo(false, false),
			_IterLock(CONSTRUCTION::EMPLACE, std::move(xIterLock)),
			_SDQLock(&xSDQLock) {}

		void __Release(TSyncBlockingDeque<T> &SDQueue) override;
	};

	TSDQPushPopLockInfo* __PushPopLock_Check(TLock const &Lock) const;

	void __Lock_Demote(TSDQPushPopLockInfo *LockInfo, bool isExclusive);

	MRLock __GetExclusiveIterLock(TLock& BaseLock, TSDQPushPopLockInfo *LockInfo, WAITTIME Timeout, THandleWaitable *AbortEvent);
	MRLock __GetSharedIterLock(TLock& BaseLock, TSDQPushPopLockInfo *LockInfo, WAITTIME Timeout, THandleWaitable *AbortEvent);

#else
#error Unimplemented sharing support!
#endif

#else
	class TSDQDynamicLockInfo : public TSDQBaseLockInfo {
	protected:
		MRLock _IterLock;
		MRLock _SDQLock;
	public:
		TSDQDynamicLockInfo(TLock &&xIterLock, MRLock &xSDQLock) :
			TSDQBaseLockInfo(false, false),
			_IterLock(CONSTRUCTION::EMPLACE, std::move(xIterLock)),
			_SDQLock(xSDQLock) {}
	};

	static TSDQBaseLockInfo __PushPopLockInfo;
	void __PushPopLock_Check(TLock const &Lock) const;
#endif


	template<class Iter>
	__Locked_Iterator<Iter> __Create_Iterator(FCIterGetter<Iter> const &IterGetter, MRLock &LockRef,
											  WAITTIME Timeout, THandleWaitable *AbortEvent) const;

	template<class Iter>
	__Locked_Iterator<Iter> __Create_Iterator(FMIterGetter<Iter> const &IterGetter, MRLock &LockRef,
											  WAITTIME Timeout, THandleWaitable *AbortEvent);
#else 
	static TSDQBaseLockInfo __PushPopLockInfo;
	void __PushPopLock_Check(TLock const &Lock) const;

	template<class Iter>
	__Locked_Iterator<Iter> __Create_Iterator(FCIterGetter<Iter> const &IterGetter, MRLock &LockRef) const;
#endif

#else
	static TSDQBaseLockInfo __PushPopLockInfo;
#endif // __SDQ_ITERATORS

	void __Lock_Sanity(TLock const &Lock) const;
	void __Unlock(TLockInfo *LockInfo) override;

	TLock __Accessor_Sync(void) {
#ifdef __SDQ_LITE
		return _Sync.Lock();
#else
		return SyncDeque.Lock();
#endif
	}

	static WaitResult __WaitFor_Event(TEvent &Event, TimeStamp &EntryTS, WAITTIME &Timeout, THandleWaitable *AbortEvent);

	TQueueAccessor __Accessor_Pickup_Safe(void);
	TQueueAccessor __Accessor_Pickup_Gated(TSyncCounter &Hold, TEvent &Sync, TimeStamp &EntryTS,
										   WAITTIME &Timeout, THandleWaitable *AbortEvent);

#define __Impl__Push(method)					\
	if (Accessor->empty()) {					\
		EmptyWait.Reset();						\
		ContentWait.Set();						\
	}											\
	Accessor->method;							\
	return Accessor->size();

	size_type __Push_Front(TQueueAccessor &Accessor, T const &entry) {
		__Impl__Push(push_front(entry));
	}

	size_type __Push_Front(TQueueAccessor &Accessor, T &&entry) {
		__Impl__Push(push_front(std::move(entry)));
	}

	size_type __Push_Back(TQueueAccessor &Accessor, T const &entry) {
		__Impl__Push(push_back(entry));
	}

	size_type __Push_Back(TQueueAccessor &Accessor, T &&entry) {
		__Impl__Push(push_back(std::move(entry)));
	}

#define __Impl__Pop(dir)						\
	entry = std::move(Accessor->dir());			\
	Accessor->pop_##dir();						\
	if (Accessor->empty()) {					\
		EmptyWait.Set();						\
		ContentWait.Reset();					\
	}

	void __Pop_Front(TQueueAccessor &Accessor, T &entry) {
		__Impl__Pop(front);
	}

	void __Pop_Back(TQueueAccessor &Accessor, T &entry) {
		__Impl__Pop(back);
	}

public:
	TString const Name;

	TSyncBlockingDeque(TString const &xName) : Name(xName) {}
	TSyncBlockingDeque(TString &&xName) : Name(std::move(xName)) {}
	~TSyncBlockingDeque(void) override;

	TLock Lock_Push(void) {
		if (!PushHold++) PushWait.Reset();
		return __New_Lock(&__PushLockInfo);
	}

	TLock Lock_Pop(void) {
		if (!PopHold++) PopWait.Reset();
		return __New_Lock(&__PopLockInfo);
	}

	TLock Lock_PushPop(void) {
		if (!PushHold++) PushWait.Reset();
		if (!PopHold++) PopWait.Reset();
#if defined(__SDQ_ITERATORS) && defined(__SDQ_MUTABLE_ITERATORS) && defined(__SDQ_CONCURRENT_CONST_ITERATORS)
		return __New_Lock(DEFAULT_NEW(TSDQPushPopLockInfo));
#else
		return __New_Lock(&__PushPopLockInfo);
#endif // __SDQ_ITERATORS && __SDQ_MUTABLE_ITERATORS && __SDQ_CONCURRENT_CONST_ITERATORS
	}

	void Sync(TLock const &Lock) {
		__Lock_Sanity(Lock);
		__Accessor_Sync();
	}

	TLock Lock(WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) override {
		auto Ret = Lock_PushPop();
		__Accessor_Sync();
		return std::move(Ret);
	}

	TLock TryLock(__ARC_UINT SpinCount = 1) override {
#ifdef _DEBUG
		if (!SpinCount) FAIL(_T("Spin count must be a natrual number"));
#endif
		TLock Ret = NullLock();
		while (--SpinCount && !(Ret = std::move(Lock(0))));
		return SpinCount ? std::move(Ret) : Lock(0);
	}

	THandleWaitable EmptyWaitable(void) {
		return EmptyWait.DupWaitable();
	}

	THandleWaitable ContentWaitable(void) {
		return ContentWait.DupWaitable();
	}

#ifdef __SDQ_ITERATORS

#ifdef __SDQ_MUTABLE_ITERATORS
	// Mutable iterators are available for a thread at a time
	iterator begin(MRLock &PushPopLock, WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr);
	iterator end(MRLock &PushPopLock, WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr);
	reverse_iterator rbegin(MRLock &PushPopLock, WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr);
	reverse_iterator rend(MRLock &PushPopLock, WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr);

	iterator erase(iterator &Iter);
	iterator insert(iterator &Iter, T const &Val);
	reverse_iterator erase(reverse_iterator &Iter);
	reverse_iterator insert(reverse_iterator &Iter, T const &Val);

#ifdef __SDQ_CONCURRENT_CONST_ITERATORS
	// Constant iterators are shared across all threads, block/by mutable iterators
#else
	// Constant iterators are available for a thread at a time
#endif
	const_iterator cbegin(MRLock &PushPopLock, WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) const;
	const_iterator cend(MRLock &PushPopLock, WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) const;
	const_reverse_iterator crbegin(MRLock &PushPopLock, WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) const;
	const_reverse_iterator crend(MRLock &PushPopLock, WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) const;
#else
	// Constant iterators are shared across all threads
	const_iterator cbegin(MRLock &PushPopLock) const;
	const_iterator cend(MRLock &PushPopLock) const;
	const_reverse_iterator crbegin(MRLock &PushPopLock) const;
	const_reverse_iterator crend(MRLock &PushPopLock) const;
#endif

#endif // __SDQ_ITERATORS

	/**
	 * Put an object into the queue-front
	 **/
	size_type Push_Front(T const &entry, WAITTIME const &Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		WAITTIME _Timeout = Timeout;
		return Push_Front(entry, _Timeout, AbortEvent);
	}
	size_type Push_Front(T &&entry, WAITTIME const &Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		WAITTIME _Timeout = Timeout;
		return Push_Front(std::move(entry), _Timeout, AbortEvent);
	}
	size_type Push_Front(T const &entry, WAITTIME &Timeout, THandleWaitable *AbortEvent = nullptr);
	size_type Push_Front(T &&entry, WAITTIME &Timeout, THandleWaitable *AbortEvent = nullptr);

	/**
	 * Put an object into the queue-back
	 **/
	size_type Push_Back(T const &entry, WAITTIME const &Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		WAITTIME _Timeout = Timeout;
		return Push_Back(entry, _Timeout, AbortEvent);
	}
	size_type Push_Back(T &&entry, WAITTIME const &Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		WAITTIME _Timeout = Timeout;
		return Push_Back(std::move(entry), _Timeout, AbortEvent);
	}
	size_type Push_Back(T const &entry, WAITTIME &Timeout, THandleWaitable *AbortEvent = nullptr);
	size_type Push_Back(T &&entry, WAITTIME &Timeout, THandleWaitable *AbortEvent = nullptr);

	/**
	 * Try get an object from the queue-front with given timeout
	 **/
	bool Pop_Front(T &entry, WAITTIME const &Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		WAITTIME _Timeout = Timeout;
		return Pop_Front(entry, _Timeout, AbortEvent);
	}
	bool Pop_Front(T &entry, WAITTIME &Timeout, THandleWaitable *AbortEvent = nullptr);

	/**
	 * Try get an object from the queue-back with given timeout
	 **/
	bool Pop_Back(T &entry, WAITTIME const &Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		WAITTIME _Timeout = Timeout;
		return Pop_Back(entry, _Timeout, AbortEvent);
	}
	bool Pop_Back(T &entry, WAITTIME &Timeout, THandleWaitable *AbortEvent = nullptr);

	/**
	 * Return the instantaneous length of the queue
	 **/
	size_type Length(void) const {
		auto Ret = (const_cast<_this*>(this)->__Accessor_Pickup_Safe())->size();
#ifdef __SDQ_LITE
		(*const_cast<_this*>(this)->_Sync).Leave();
#endif
		return Ret;
	}

	void Deflate(void) const {
		(const_cast<_this*>(this)->__Accessor_Pickup_Safe())->shrink_to_fit();
#ifdef __SDQ_LITE
		(*const_cast<_this*>(this)->_Sync).Leave();
#endif
	}

	/**
	 * Try waiting for queue to become empty and hold lock on the queue
	 **/
	TLock DrainAndLock(WAITTIME const &Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr) {
		WAITTIME _Timeout = Timeout;
		return DrainAndLock(_Timeout, AbortEvent);
	}
	TLock DrainAndLock(WAITTIME &Timeout, THandleWaitable *AbortEvent = nullptr);

	WaitResult WaitFor(WAITTIME Timeout) const override {
		return ContentWait.WaitFor(Timeout);
	}

};

#define SDQLogTag _T("SyncDQ '%s'")
#define SDQLogHeader _T("{") SDQLogTag _T("} ")

//! @ingroup Threading
//! Synchronized queue specific exception
class TSyncBlockingDequeException : public Exception {
	typedef TSyncBlockingDequeException _this;

public:
	TString const ContainerName;

	template <typename T, typename... Params>
	TSyncBlockingDequeException(TSyncBlockingDeque<T> const &xContainer, TString &&xSource,
								LPCTSTR ReasonFmt, Params&&... xParams) :
		Exception(std::move(xSource), ReasonFmt, std::forward<Params>(xParams)...), ContainerName(xContainer.Name) {}

	TSyncBlockingDequeException(_this &&xException) NOEXCEPT
		: Exception(std::move(xException)), ContainerName(std::move(xException.ContainerName)) {}

	TSyncBlockingDequeException(_this const &xException)
		: Exception(xException), ContainerName(xException.ContainerName) {}

	virtual _this* MakeClone(IObjAllocator<void> &_Alloc) const override {
		return DEFAULT_NEW(_this, *this);
	}

	TString const& Why(void) const override {
		if (rWhy.empty()) {
			HEAPSTR_ERRMSGFMT(SDQLogHeader _T("%s"), ContainerName.c_str(), Exception::Why().c_str());
			rWhy.assign(std::move(__ErrorMsg));
		}
		return rWhy;
	}
};

//! @ingroup Threading
//! Raise an exception within a synchronized queue with formatted string message
#define __SDQFAIL(inst, ...) {													\
	SOURCEMARK;																	\
	throw TSyncBlockingDequeException(inst, std::move(__SrcMark), __VA_ARGS__);	\
}

//! Perform logging within a synchronized queue
#define SDQLOG(s, ...) LOG(SDQLogHeader s, Name.c_str(), __VA_ARGS__)
#define SDQLOGV(s, ...) LOGV(SDQLogHeader s, Name.c_str(), __VA_ARGS__)
#define SDQLOGVV(s, ...) LOGVV(SDQLogHeader s, Name.c_str(), __VA_ARGS__)

#define DESTRUCTION_MESSAGE _T("Destruction in progress...")

template<class T>
typename TSyncBlockingDeque<T>::TSDQBaseLockInfo TSyncBlockingDeque<T>::__PushLockInfo = { true,false };

template<class T>
typename TSyncBlockingDeque<T>::TSDQBaseLockInfo TSyncBlockingDeque<T>::__PopLockInfo = { false,true };

#ifdef __SDQ_ITERATORS

#if defined(__SDQ_MUTABLE_ITERATORS) && defined(__SDQ_CONCURRENT_CONST_ITERATORS)

#ifdef __ZWUTILS_SYNC_SLIMRWLOCK

template<class T>
void TSyncBlockingDeque<T>::TSDQIterLockInfo::__Release(TSyncBlockingDeque<T> &SDQueue) {
	auto LinkedLockBaseInfo = static_cast<TSDQBaseLockInfo*>(__LockInfo(*_SDQLock));
	auto LinkedLockInfo = dynamic_cast<TSDQPushPopLockInfo*>(LinkedLockBaseInfo);
	SDQueue.__Lock_Demote(LinkedLockInfo, SDQueue.SRWSync.ForWrite(*_IterLock));

	// Release lock and then send signal if needed
	_IterLock.Clear();
	if (~SDQueue.IterWaiters) {
		// We are in a transient state of deferred event creation
		// Just hold the breath for a while
		while (!SDQueue.IterWaitEvent.Allocated()) SwitchToThread();
		SDQueue.IterWaitEvent.Set();
	}
}

#else
#error Unimplemented sharing support!
#endif

#endif

#endif // __SDQ_ITERATORS

#define SDQFAIL(...) __SDQFAIL(*this, __VA_ARGS__)

#ifdef __SDQ_LITE
#define __SyncLock_RAII		TInitResource<int> __RAII(0, [&](int &) {(*_Sync).Leave(); })
#define __SyncLock_RAII_C	TInitResource<int> __RAII(0, [&](int &) {(*const_cast<_this*>(this)->_Sync).Leave(); })
#else
#define __SyncLock_RAII
#define __SyncLock_RAII_C
#endif

#ifdef __SDQ_ITERATORS

#ifdef __SDQ_MUTABLE_ITERATORS

#ifdef __SDQ_CONCURRENT_CONST_ITERATORS

template<class T>
typename TSyncBlockingDeque<T>::TSDQPushPopLockInfo*
TSyncBlockingDeque<T>::__PushPopLock_Check(TLock const &Lock) const {
	__Lock_Sanity(Lock);
	TSDQBaseLockInfo *__Info = static_cast<TSDQBaseLockInfo*>(__LockInfo(Lock));
	auto PushPopLockInfo = dynamic_cast<TSDQPushPopLockInfo*>(__Info);
	if (!PushPopLockInfo) SDQFAIL(_T("Operation requires a push-pop lock"));
	return PushPopLockInfo;
}

template<class T>
void TSyncBlockingDeque<T>::__Lock_Demote(TSDQPushPopLockInfo *LockInfo, bool isExclusive) {
	if (isExclusive) {
		if (LockInfo->_SharedLock) {
			// Downgrade to shared lock
			auto QueueLock = __Accessor_Sync();
			{
				auto LinkedLockBaseInfo = static_cast<TSDQBaseLockInfo*>(__LockInfo(*LockInfo->_ExclusiveLock));
				auto LinkedLockInfo = dynamic_cast<TSDQIterLockInfo*>(LinkedLockBaseInfo);
				__LockDrop(*LinkedLockInfo->_IterLock);
			}
			(*SRWSync).EndWrite();
			{
				auto LinkedLockBaseInfo = static_cast<TSDQBaseLockInfo*>(__LockInfo(*LockInfo->_SharedLock));
				auto LinkedLockInfo = dynamic_cast<TSDQIterLockInfo*>(LinkedLockBaseInfo);
				*LinkedLockInfo->_IterLock = SRWSync.Lock_Read();
			}
		}
		LockInfo->_ExclusiveLock = nullptr;
	} else {
		LockInfo->_SharedLock = nullptr;
	}
}

#define __IMPL_IterLockOp(sync_raii, free_olock, regain_olock_raii, replace_olock, spin_nlock, single_nlock, opname)	\
	TAllocResource<__ARC_INT> WaitCounter([&] { return IterWaiters++; }, [&](__ARC_INT &) { --IterWaiters; });			\
	TimeStamp EntryTS;																									\
	while (true) {																										\
		{																												\
			sync_raii;																									\
			free_olock;																									\
			{																											\
				regain_olock_raii;																						\
				if (spin_nlock) {																						\
					replace_olock;																						\
					break;																								\
				}																										\
				/* Allocate wait counter */																				\
				if (!WaitCounter.Allocated()) {																			\
					if (*WaitCounter) {																					\
						/* Check if we are racing against the event object allocation */								\
						while (!IterWaitEvent.Allocated()) SwitchToThread();											\
					}																									\
					/* Check again before wait */																		\
					if (single_nlock) {																					\
						replace_olock;																					\
						break;																							\
					}																									\
				}																										\
			}																											\
		}																												\
		/* Calculate remaining wait time */																				\
		if (Timeout != FOREVER) {																						\
			if (!EntryTS) {																								\
				EntryTS = TimeStamp::Now();																				\
			} else {																									\
				TimeStamp Now = TimeStamp::Now();																		\
				INT64 WaitDur = (Now - EntryTS).GetValue(TimeUnit::MSEC);												\
				Timeout = Timeout > WaitDur ? Timeout - (WAITTIME)WaitDur : 0;											\
				EntryTS = Now;																							\
			}																											\
		}																												\
		/* Perform the wait */																							\
		WaitResult WRet = AbortEvent ?																					\
			WaitMultiple({ IterWaitEvent, *AbortEvent }, false, Timeout) :												\
			IterWaitEvent.WaitFor(Timeout);																				\
		/* Analyze the result */																						\
		switch (WRet) {																									\
			case WaitResult::Error: SYSFAIL(_T("Failed to ") opname);													\
			case WaitResult::Signaled:																					\
			case WaitResult::Signaled_0: continue;																		\
			case WaitResult::Signaled_1:																				\
			case WaitResult::TimedOut: break;																			\
			default: SYSFAIL(_T("Unable to ") opname);																	\
		}																												\
		break;																											\
	}

#define __IMPL_LinkLock(exlock, lockbase, container)						\
	container = &(															\
		*Ret = const_cast<_this*>(this)->__New_Lock(						\
			DEFAULT_NEW(TSDQIterLockInfo, std::move(exlock), lockbase)		\
		)																	\
	)

template<class T>
typename TLockable::MRLock TSyncBlockingDeque<T>::__GetExclusiveIterLock(TLock& BaseLock,
																		 TSDQPushPopLockInfo *LockInfo, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	// Check if we have already promoted
	if (LockInfo->_ExclusiveLock) return { LockInfo->_ExclusiveLock };

	MRLock Ret(CONSTRUCTION::EMPLACE, SRWSync.NullLock());
	// Check if we have a shared iterator lock
	if (LockInfo->_SharedLock) {
		auto LinkedLockBaseInfo = static_cast<TSDQBaseLockInfo*>(__LockInfo(*LockInfo->_SharedLock));
		auto LinkedLockInfo = dynamic_cast<TSDQIterLockInfo*>(LinkedLockBaseInfo);
		__IMPL_IterLockOp(auto QueueLock = __Accessor_Sync(),
						  {
							  (*SRWSync).EndRead(); __LockDrop(*LinkedLockInfo->_IterLock);
						  },
						  TAllocResource<int> __LockRecover_RAII(0,
																 [&](int &) { *LinkedLockInfo->_IterLock = SRWSync.Lock_Read(); }
						  ),
						  {
							  __IMPL_LinkLock(*Ret, BaseLock, LockInfo->_ExclusiveLock);
							  __LockRecover_RAII.Invalidate();
						  },
							  *Ret = SRWSync.TryLock_Write(),
							  *Ret = SRWSync.TryLock_Write(1),
							  _T("promote to exclusive lock")
							  );
	} else {
		__IMPL_IterLockOp(auto QueueLock = __Accessor_Sync(), {}, {},
						  __IMPL_LinkLock(*Ret, BaseLock, LockInfo->_ExclusiveLock),
						  *Ret = SRWSync.TryLock_Write(),
						  *Ret = SRWSync.TryLock_Write(1),
						  _T("acquire exclusive lock")
		);
	}
	return std::move(Ret);
}

template<class T>
typename TLockable::MRLock TSyncBlockingDeque<T>::__GetSharedIterLock(TLock& BaseLock,
																	  TSDQPushPopLockInfo *LockInfo, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	// Check if we already have a shared iterator lock
	if (LockInfo->_SharedLock) return { LockInfo->_SharedLock };

	MRLock Ret(CONSTRUCTION::EMPLACE, SRWSync.NullLock());
	// Check if we are in exclusive mode
	if (LockInfo->_ExclusiveLock) {
		__IMPL_LinkLock(*Ret, BaseLock, LockInfo->_SharedLock);
	} else {
		__IMPL_IterLockOp({}, {}, {},
						  __IMPL_LinkLock(*Ret, BaseLock, LockInfo->_SharedLock),
						  *Ret = SRWSync.TryLock_Read(),
						  *Ret = SRWSync.TryLock_Read(1),
						  _T("acquire shared lock")
		);
	}
	return std::move(Ret);
}

#define __IMPL_Create_Iterator												\
	if (*IterLock) {														\
		auto Queue = const_cast<_this*>(this)->__Accessor_Pickup_Safe();	\
		__SyncLock_RAII_C;													\
		return { ((*Queue).*IterGetter)(), IterLock };						\
	} else return {}

template<class T>
template<class Iter>
typename TSyncBlockingDeque<T>::__Locked_Iterator<Iter> TSyncBlockingDeque<T>::__Create_Iterator(
	FCIterGetter<Iter> const &IterGetter, MRLock &LockRef, WAITTIME Timeout, THandleWaitable *AbortEvent) const {
	auto LockInfo = __PushPopLock_Check(*LockRef);
	auto IterLock = const_cast<_this*>(this)->__GetSharedIterLock(*LockRef, LockInfo, Timeout, AbortEvent);
	__IMPL_Create_Iterator;
}

template<class T>
template<class Iter>
typename TSyncBlockingDeque<T>::__Locked_Iterator<Iter> TSyncBlockingDeque<T>::__Create_Iterator(
	FMIterGetter<Iter> const &IterGetter, MRLock &LockRef, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	auto LockInfo = __PushPopLock_Check(*LockRef);
	auto IterLock = __GetExclusiveIterLock(*LockRef, LockInfo, Timeout, AbortEvent);
	__IMPL_Create_Iterator;
}

#else

template<class T>
typename TSyncBlockingDeque<T>::TSDQBaseLockInfo TSyncBlockingDeque<T>::__PushPopLockInfo = { true,true };

template<class T>
void TSyncBlockingDeque<T>::__PushPopLock_Check(TLock const &Lock) const {
	__Lock_Sanity(Lock);
	TSDQBaseLockInfo *__Info = static_cast<TSDQBaseLockInfo*>(__LockInfo(Lock));
	if (!__Info->Push || !__Info->Pop) SDQFAIL(_T("Operation requires a push-pop lock"));
}

#define __IMPL_Create_Iterator																			\
	__PushPopLock_Check(*LockRef);																		\
	auto IterLock = (const_cast<_this*>(this)->ExclusiveSync).Lock(Timeout, AbortEvent);				\
	if (IterLock) {																						\
		MRLock DynamicLock(CONSTRUCTION::EMPLACE, const_cast<_this*>(this)->__New_Lock(					\
			DEFAULT_NEW(TSDQDynamicLockInfo, std::move(IterLock), LockRef)								\
		));																								\
		auto Queue = const_cast<_this*>(this)->__Accessor_Pickup_Safe();								\
		__SyncLock_RAII_C;																				\
		return { ((*Queue).*IterGetter)(), DynamicLock };												\
	} else return {}

template<class T>
template<class Iter>
typename TSyncBlockingDeque<T>::__Locked_Iterator<Iter> TSyncBlockingDeque<T>::__Create_Iterator(
	FCIterGetter<Iter> const &IterGetter, MRLock &LockRef, WAITTIME Timeout, THandleWaitable *AbortEvent) const {
	__IMPL_Create_Iterator;
}

template<class T>
template<class Iter>
typename TSyncBlockingDeque<T>::__Locked_Iterator<Iter> TSyncBlockingDeque<T>::__Create_Iterator(
	FMIterGetter<Iter> const &IterGetter, MRLock &LockRef, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	__IMPL_Create_Iterator;
}

#endif

#else

template<class T>
typename TSyncBlockingDeque<T>::TSDQBaseLockInfo TSyncBlockingDeque<T>::__PushPopLockInfo = { true,true };

template<class T>
void TSyncBlockingDeque<T>::__PushPopLock_Check(TLock const &Lock) const {
	__Lock_Sanity(Lock);
	TSDQBaseLockInfo *__Info = static_cast<TSDQBaseLockInfo*>(__LockInfo(Lock));
	if (!__Info->Push || !__Info->Pop) SDQFAIL(_T("Operation requires a push-pop lock"));
}

template<class T>
template<class Iter>
typename TSyncBlockingDeque<T>::__Locked_Iterator<Iter> TSyncBlockingDeque<T>::__Create_Iterator(
	FCIterGetter<Iter> const &IterGetter, MRLock &LockRef) const {
	__PushPopLock_Check(*LockRef);
	auto Queue = const_cast<_this*>(this)->__Accessor_Pickup_Safe();
	__SyncLock_RAII_C;
	return { ((*Queue).*IterGetter)(), LockRef };
	}

#endif

#else

template<class T>
typename TSyncBlockingDeque<T>::TSDQBaseLockInfo TSyncBlockingDeque<T>::__PushPopLockInfo = { true,true };

#endif // __SDQ_ITERATORS

template<class T>
void TSyncBlockingDeque<T>::__Lock_Sanity(TLock const &Lock) const {
	if (!By(Lock)) SDQFAIL(_T("Incorrect locking context"));
}

template<class T>
void TSyncBlockingDeque<T>::__Unlock(TLockInfo *LockInfo) {
	TSDQBaseLockInfo *__Info = static_cast<TSDQBaseLockInfo*>(LockInfo);
	if (__Info->Push) {
		if (!--PushHold) PushWait.Set();
	}
	if (__Info->Pop) {
		if (!--PopHold) PopWait.Set();
	}

#if defined(__SDQ_ITERATORS) && defined(__SDQ_MUTABLE_ITERATORS)
	TSDQDynamicLockInfo *__AdvInfo = dynamic_cast<TSDQDynamicLockInfo*>(__Info);
	if (__AdvInfo) {
#ifdef __SDQ_CONCURRENT_CONST_ITERATORS
		__AdvInfo->__Release(*this);
#endif
		DEFAULT_DESTROY(TSDQDynamicLockInfo, __AdvInfo);
	}
#endif // __SDQ_ITERATORS && __SDQ_MUTABLE_ITERATORS
}

template<class T>
WaitResult TSyncBlockingDeque<T>::__WaitFor_Event(TEvent &Event, TimeStamp &EntryTS,
												  WAITTIME &Timeout, THandleWaitable *AbortEvent) {
	WaitResult WRet = AbortEvent ?
		WaitMultiple({ Event, *AbortEvent }, false, Timeout) :
		Event.WaitFor(Timeout);

	if (Timeout != FOREVER) {
		TimeStamp Now = TimeStamp::Now();
		INT64 WaitDur = (Now - EntryTS).GetValue(TimeUnit::MSEC);
		Timeout = Timeout > WaitDur ? Timeout - (WAITTIME)WaitDur : 0;
		EntryTS = Now;
	}
	return WRet;
}

template<class T>
typename TSyncBlockingDeque<T>::TQueueAccessor TSyncBlockingDeque<T>::__Accessor_Pickup_Safe(void) {
#ifdef __SDQ_LITE
	(*_Sync).Enter();
	if (__Cleanup) {
		(*_Sync).Leave();
		SDQFAIL(DESTRUCTION_MESSAGE);
	}
	return &_Queue;
#else
	auto Queue = SyncDeque.Pickup();
	if (__Cleanup) SDQFAIL(DESTRUCTION_MESSAGE);
	return Queue;
#endif
	}

template<class T>
typename TSyncBlockingDeque<T>::TQueueAccessor TSyncBlockingDeque<T>::__Accessor_Pickup_Gated(
	TSyncCounter &Hold, TEvent &Sync, TimeStamp &EntryTS, WAITTIME &Timeout, THandleWaitable *AbortEvent) {
	while (true) {
		while (~Hold) {
			switch (__WaitFor_Event(Sync, EntryTS, Timeout, AbortEvent)) {
				case WaitResult::Error: SYSFAIL(_T("Failed to wait for pickup event"));
				case WaitResult::Signaled:
				case WaitResult::Signaled_0: break;
				case WaitResult::Signaled_1:
				case WaitResult::TimedOut:
#ifdef __SDQ_LITE
					return nullptr;
#else
					return SyncDeque.NullAccessor();
#endif
				default: SYSFAIL(_T("Unable to wait for pickup event"));
			}
			if (__Cleanup) SDQFAIL(DESTRUCTION_MESSAGE);
		}
#ifdef __SDQ_LITE
		if ((*_Sync).TryEnter(__SDQ_SYNCSPIN)) return &_Queue;
#else
		if (auto Queue = SyncDeque.TryPickup(__SDQ_SYNCSPIN)) return Queue;
#endif
		}
	}

template<class T>
TSyncBlockingDeque<T>::~TSyncBlockingDeque(void) {
	{
		// Ensure concurrent operation finish, and future operation will be rejected
		auto _Lock = Lock_PushPop();
		__Cleanup = true;
		__Accessor_Sync();
	}

	{
#ifdef __SDQ_LITE
		auto _Queue = &this->_Queue;
#else
		auto _Queue = SyncDeque.Pickup();
#endif
		if (size_t Size = _Queue->size()) {
			SDQLOG(_T("WARNING: There are %d left over entries"), (int)Size);
		}
		if (long Count = ~PushHold) {
			SDQLOG(_T("WARNING: There are %d unreleased push hold"), Count);
		}
		if (long Count = ~PopHold) {
			SDQLOG(_T("WARNING: There are %d unreleased pop hold"), Count);
		}
}

	PushWait.Set();
	PopWait.Set();
	EmptyWait.Set();
	ContentWait.Set();
}

#ifdef __SDQ_ITERATORS

#ifdef __SDQ_MUTABLE_ITERATORS

template<class T>
typename TSyncBlockingDeque<T>::iterator TSyncBlockingDeque<T>::begin(
	MRLock &PushPopLock, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	return __Create_Iterator((FMIterGetter<typename Container::iterator>)&Container::begin,
							 PushPopLock, Timeout, AbortEvent);
}

template<class T>
typename TSyncBlockingDeque<T>::iterator TSyncBlockingDeque<T>::end(
	MRLock &PushPopLock, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	return __Create_Iterator((FMIterGetter<typename Container::iterator>)&Container::end,
							 PushPopLock, Timeout, AbortEvent);
}

template<class T>
typename TSyncBlockingDeque<T>::reverse_iterator TSyncBlockingDeque<T>::rbegin(
	MRLock &PushPopLock, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	return __Create_Iterator((FMIterGetter<typename Container::reverse_iterator>)&Container::rbegin,
							 PushPopLock, Timeout, AbortEvent);
}

template<class T>
typename TSyncBlockingDeque<T>::reverse_iterator TSyncBlockingDeque<T>::rend(
	MRLock &PushPopLock, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	return __Create_Iterator((FMIterGetter<typename Container::reverse_iterator>)&Container::rend,
							 PushPopLock, Timeout, AbortEvent);
}

template<class T>
typename TSyncBlockingDeque<T>::const_iterator TSyncBlockingDeque<T>::cbegin(
	MRLock &PushPopLock, WAITTIME Timeout, THandleWaitable *AbortEvent) const {
	return __Create_Iterator(&Container::cbegin, PushPopLock, Timeout, AbortEvent);
}

template<class T>
typename TSyncBlockingDeque<T>::const_iterator TSyncBlockingDeque<T>::cend(
	MRLock &PushPopLock, WAITTIME Timeout, THandleWaitable *AbortEvent) const {
	return __Create_Iterator(&Container::cend, PushPopLock, Timeout, AbortEvent);
}

template<class T>
typename TSyncBlockingDeque<T>::const_reverse_iterator TSyncBlockingDeque<T>::crbegin(
	MRLock &PushPopLock, WAITTIME Timeout, THandleWaitable *AbortEvent) const {
	return __Create_Iterator(&Container::crbegin, PushPopLock, Timeout, AbortEvent);
}

template<class T>
typename TSyncBlockingDeque<T>::const_reverse_iterator TSyncBlockingDeque<T>::crend(
	MRLock &PushPopLock, WAITTIME Timeout, THandleWaitable *AbortEvent) const {
	return __Create_Iterator(&Container::crend, PushPopLock, Timeout, AbortEvent);
}

template<class T>
typename TSyncBlockingDeque<T>::iterator TSyncBlockingDeque<T>::erase(iterator &Iter) {
#ifdef __SDQ_LITE
	return { std::move(Iter), _Queue.erase(Iter) };
#else
	return { std::move(Iter), __Accessor_Pickup_Safe()->erase(Iter) };
#endif
}

template<class T>
typename TSyncBlockingDeque<T>::iterator TSyncBlockingDeque<T>::insert(iterator &Iter, T const &Val) {
#ifdef __SDQ_LITE
	return { std::move(Iter), _Queue.insert(Iter, Val) };
#else
	return { std::move(Iter), __Accessor_Pickup_Safe()->insert(Iter, Val) };
#endif
}

template<class T>
typename TSyncBlockingDeque<T>::reverse_iterator TSyncBlockingDeque<T>::erase(reverse_iterator &Iter) {
#ifdef __SDQ_LITE
	return { std::move(Iter), _Queue.erase(Iter) };
#else
	return { std::move(Iter), __Accessor_Pickup_Safe()->erase(Iter) };
#endif
}

template<class T>
typename TSyncBlockingDeque<T>::reverse_iterator TSyncBlockingDeque<T>::insert(reverse_iterator &Iter, T const &Val) {
#ifdef __SDQ_LITE
	return { std::move(Iter), _Queue.insert(Iter, Val) };
#else
	return { std::move(Iter), __Accessor_Pickup_Safe()->insert(Iter, Val) };
#endif
}

#else

template<class T>
typename TSyncBlockingDeque<T>::const_iterator TSyncBlockingDeque<T>::cbegin(MRLock &PushPopLock) const {
	return __Create_Iterator(&Container::cbegin, PushPopLock);
}

template<class T>
typename TSyncBlockingDeque<T>::const_iterator TSyncBlockingDeque<T>::cend(MRLock &PushPopLock) const {
	return __Create_Iterator(&Container::cend, PushPopLock);
}

template<class T>
typename TSyncBlockingDeque<T>::const_reverse_iterator TSyncBlockingDeque<T>::crbegin(MRLock &PushPopLock) const {
	return __Create_Iterator(&Container::crbegin, PushPopLock);
}

template<class T>
typename TSyncBlockingDeque<T>::const_reverse_iterator TSyncBlockingDeque<T>::crend(MRLock &PushPopLock) const {
	return __Create_Iterator(&Container::crend, PushPopLock);
}

#endif

#endif // __SDQ_ITERATORS

#define __Impl_Push(dir,data)																	\
	TimeStamp EntryTS = Timeout == FOREVER ? TimeStamp::Null : TimeStamp::Now();				\
	auto Accessor = __Accessor_Pickup_Gated(PushHold, PushWait, EntryTS, Timeout, AbortEvent);	\
	if (!Accessor) return -1;																	\
	{																							\
		__SyncLock_RAII;																		\
		return __Push_##dir(Accessor, data);													\
	}

template<class T>
typename TSyncBlockingDeque<T>::size_type TSyncBlockingDeque<T>::Push_Front(
	T const &entry, WAITTIME &Timeout, THandleWaitable *AbortEvent) {
	__Impl_Push(Front, entry);
}

template<class T>
typename TSyncBlockingDeque<T>::size_type TSyncBlockingDeque<T>::Push_Front(
	T &&entry, WAITTIME &Timeout, THandleWaitable *AbortEvent) {
	__Impl_Push(Front, std::move(entry));
}

template<class T>
typename TSyncBlockingDeque<T>::size_type TSyncBlockingDeque<T>::Push_Back(
	T const &entry, WAITTIME &Timeout, THandleWaitable *AbortEvent) {
	__Impl_Push(Back, entry);
}

template<class T>
typename TSyncBlockingDeque<T>::size_type TSyncBlockingDeque<T>::Push_Back(
	T &&entry, WAITTIME &Timeout, THandleWaitable *AbortEvent) {
	__Impl_Push(Back, std::move(entry));
}

#define __Impl_Pop(dir)																							\
	TimeStamp EntryTS = Timeout == FOREVER ? TimeStamp::Null : TimeStamp::Now();								\
	while (true) {																								\
		{																										\
			auto Accessor = __Accessor_Pickup_Gated(PopHold, PopWait, EntryTS, Timeout, AbortEvent);			\
			if (!Accessor) return false;																		\
			{																									\
				__SyncLock_RAII;																				\
				if (!Accessor->empty()) return __Pop_##dir(Accessor, entry), true;								\
			}																									\
		}																										\
		switch (__WaitFor_Event(ContentWait, EntryTS, Timeout, AbortEvent)) {									\
			case WaitResult::Error: SYSFAIL(_T("Failed to wait for pop event"));								\
			case WaitResult::Signaled:																			\
			case WaitResult::Signaled_0: continue;																\
			case WaitResult::Signaled_1:																		\
			case WaitResult::TimedOut: return false;															\
			default: SYSFAIL(_T("Unable to wait for pop event"));												\
		}																										\
	}

template<class T>
bool TSyncBlockingDeque<T>::Pop_Front(T &entry, WAITTIME &Timeout, THandleWaitable *AbortEvent) {
	__Impl_Pop(Front);
}

template<class T>
bool TSyncBlockingDeque<T>::Pop_Back(T &entry, WAITTIME &Timeout, THandleWaitable *AbortEvent) {
	__Impl_Pop(Back);
}

template<class T>
typename TLockable::TLock TSyncBlockingDeque<T>::DrainAndLock(WAITTIME &Timeout, THandleWaitable *AbortEvent) {
	TimeStamp EntryTS = Timeout == FOREVER ? TimeStamp::Null : TimeStamp::Now();
	auto _Lock = Lock_Push();
	while (true) {
		{
			auto Accessor = __Accessor_Pickup_Safe();
			{
				__SyncLock_RAII;
				if (Accessor->empty()) return std::move(_Lock);
			}
		}
		switch (__WaitFor_Event(EmptyWait, EntryTS, Timeout, AbortEvent)) {
			case WaitResult::Error: SYSFAIL(_T("Failed to wait for queue drain"));
			case WaitResult::Signaled:
			case WaitResult::Signaled_0: break;
			case WaitResult::Signaled_1:
			case WaitResult::TimedOut: return NullLock();
			default: SYSFAIL(_T("Unable to wait for queue drain"));
		}
	}
}

#undef SDQFAIL
#undef SDQLOG
#undef SDQLOGV
#undef SDQLOGVV

#endif //ZWUtils_SyncQueue_H