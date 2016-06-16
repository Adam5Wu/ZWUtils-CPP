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
 * @brief Synchronized Message Queue
 * @author Zhenyu Wu
 * @date Feb 10, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_SyncQueue_H
#define ZWUtils_SyncQueue_H

#include "Misc/Global.h"
#include "Misc/TString.h"

#include "Debug/Debug.h"
#include "Debug/Exception.h"

#include "SyncElements.h"
#include "SyncObjects.h"

#include <deque>

/**
 * @ingroup Threading
 * @brief Synchronized queue
 *
 * Synchronized blocking double-ended queue with no upper limit
 * Note: Currently implementation does not have faireness guarantee
 **/
template<class T>
class TSyncBlockingDeque : TLockable {
	typedef TSyncBlockingDeque _this;
	typedef std::deque<T> Container;

public:
	typedef typename Container::size_type size_type;

protected:
	volatile bool __Cleanup;

	typedef TSyncObj<Container> TSyncDeque;
	TSyncDeque SyncDeque;
	TEvent EntryEvent, EmptyEvent, PreEntryEvent;

	typedef TInterlockedOrdinal32<long> TSyncCounter;
	TSyncCounter EntryWaiters = 0;
	TSyncCounter EmptyWaiters = 0;
	TSyncCounter PreEntryWaiters = 0;

	typename TSyncDeque::Accessor __Safe_Pickup(void);

	bool __Signal_Event(TSyncCounter &Counter, TEvent &SEvent, TEvent &REvent)
	{ return (~Counter) ? REvent.Reset(), SEvent.Set(), true : false; }

	bool __WaitFor_Event(TEvent &Event, WAITTIME Timeout, TimeStamp const &StartTime, THandleWaitable *AbortEvent);

	void __PreEntry_Check(void);

	typedef void(Container::*TEntryCPush)(T const &entry);
	typedef void(Container::*TEntryMPush)(T &&entry);
	size_type __Push_Do(TEntryCPush const &EntryPush, T const &entry);
	size_type __Push_Do(TEntryMPush const &EntryPush, T &&entry);

	typedef T&(Container::*TEntryGet)(void);
	typedef void(Container::*TEntryDiscard)(void);
	bool __Pop_Do(TEntryGet const &EntryGet, TEntryDiscard const &EntryDiscard, T &entry);

	bool __Pop_Front(T &entry)
	{ return __Pop_Do(&Container::front, &Container::pop_front, entry); }

	bool __Pop_Back(T &entry)
	{ return __Pop_Do(&Container::back, &Container::pop_back, entry); }

	typedef bool(_this::*TPopFunc)(T &entry);
	bool __Pop(TPopFunc const &PopFunc, T &entry, WAITTIME Timeout, THandleWaitable *AbortEvent);

	TLock __LockEmpty(void)
	{ return __Safe_Pickup()->empty() ? SyncDeque.Lock() : SyncDeque.NullLock(); }

	typedef typename Container::const_iterator(Container::*TGetCIter)(void) const;
	typename Container::const_iterator LockedGetCIter(TLock const &Lock, TGetCIter const &GetCIter);

public:
	TString const Name;

	TSyncBlockingDeque(TString const &xName) : Name(xName), __Cleanup(false), EntryEvent(true), EmptyEvent(true, true), PreEntryEvent(true) {}
	~TSyncBlockingDeque() override;

	TLock Lock(TWaitable *AbortEvent = nullptr) override
	{ return SyncDeque.Lock(AbortEvent); }

	TLock TryLock(void) override
	{ return SyncDeque.TryLock(); }

	typename Container::const_iterator Locked_cbegin(TLock const &Lock)
	{ return LockedGetCIter(Lock, static_cast<TGetCIter>(&Container::cbegin)); }

	typename Container::const_iterator Locked_cend(TLock const &Lock)
	{ return LockedGetCIter(Lock, static_cast<TGetCIter>(&Container::cend)); }

	typename Container::const_iterator Locked_crbegin(TLock const &Lock)
	{ return LockedGetCIter(Lock, static_cast<TGetCIter>(&Container::crbegin)); }

	typename Container::const_iterator Locked_crend(TLock const &Lock)
	{ return LockedGetCIter(Lock, static_cast<TGetCIter>(&Container::crend)); }

	/**
	 * Put an object into the queue-front
	 **/
	size_type Push_Front(T const &entry)
	{ return __Push_Do(static_cast<TEntryCPush>(&Container::push_front), entry); }

	size_type Push_Front(T &&entry)
	{ return __Push_Do(static_cast<TEntryMPush>(&Container::push_front), std::move(entry)); }

	/**
	 * Put an object into the queue-back
	 **/
	size_type Push_Back(T const &entry)
	{ return __Push_Do(static_cast<TEntryCPush>(&Container::push_back), entry); }

	size_type Push_Back(T &&entry)
	{ return __Push_Do(static_cast<TEntryMPush>(&Container::push_back), std::move(entry)); }

	/**
	 * Try get an object from the queue-front with given timeout
	 **/
	bool Pop_Front(T &entry, WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr)
	{ return __Pop(&TSyncBlockingDeque<T>::__Pop_Front, entry, Timeout, AbortEvent); }

	/**
	 * Try get an object from the queue-back with given timeout
	 **/
	bool Pop_Back(T &entry, WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr)
	{ return __Pop(&TSyncBlockingDeque<T>::__Pop_Back, entry, Timeout, AbortEvent); }

	/**
	 * Return the instantaneous length of the queue
	 **/
	size_type Length(void)
	{ return SyncDeque.Pickup()->size(); }

	void AdjustSize(void)
	{ SyncDeque.Pickup()->shrink_to_fit(); }

	/**
	 * Try waiting for queue to become empty and hold lock on the queue
	 **/
	TLock EmptyLock(WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr);
};

#define SDQLogTag _T("SyncDQ '%s'")
#define SDQLogHeader _T("{") SDQLogTag _T("} ")

//! @ingroup Threading
//! Synchronized queue specific exception
class TSyncBlockingDequeException : public Exception {
	typedef TSyncBlockingDequeException _this;

protected:
	template <typename... Params>
	TSyncBlockingDequeException(TString const &xContainerName, TString &&xSource, LPCTSTR ReasonFmt, Params&&... xParams) :
		Exception(std::move(xSource), ReasonFmt, xParams...), ContainerName(xContainerName) {}

public:
	TString const ContainerName;

	template <class CSyncQueue, typename... Params>
	static _this* Create(CSyncQueue const &xSyncQueue, TString &&xSource, LPCTSTR ReasonFmt, Params&&... xParams) {
		return DefaultObjAllocator<_this>().Create(RLAMBDANEW(_this,
			xSyncQueue.Name, std::move(xSource), ReasonFmt, xParams...));
	}

	TString const& Why(void) const override {
		if (rWhy.length() == 0) {
			PCTCHAR dWhy = Exception::Why().c_str();
			HEAPSTR_ERRMSGFMT(SDQLogHeader _T("%s"), ContainerName.c_str(), dWhy);
			const_cast<TString*>(&rWhy)->assign(std::move(__ErrorMsg));
		}
		return rWhy;
	}
};

//! @ingroup Threading
//! Raise an exception within a synchronized queue with formatted string message
#define SDQFAIL(...) {																		\
	SOURCEMARK;																				\
	throw TSyncBlockingDequeException::Create(*this, std::move(__SrcMark), __VA_ARGS__);	\
}

//! Perform logging within a synchronized queue
#define SDQLOG(s, ...) LOG(SDQLogHeader s, Name.c_str(), __VA_ARGS__)
#define SDQLOGV(s, ...) LOGV(SDQLogHeader s, Name.c_str(), __VA_ARGS__)
#define SDQLOGVV(s, ...) LOGVV(SDQLogHeader s, Name.c_str(), __VA_ARGS__)

#define DESTRUCTION_MESSAGE _T("Destruction in progress...")

template<class T>
typename TSyncBlockingDeque<T>::TSyncDeque::Accessor TSyncBlockingDeque<T>::__Safe_Pickup(void) {
	while (true) {
		if (auto Queue = SyncDeque.TryPickup()) return Queue;
		if (__Cleanup) SDQFAIL(DESTRUCTION_MESSAGE);
	}
}

template<class T>
TSyncBlockingDeque<T>::~TSyncBlockingDeque() {
	auto Queue = __Safe_Pickup();
	__Cleanup = true;

	if (size_t Size = Queue->size()) {
		SDQLOG(_T("WARNING: There are %d left over entries"), (int)Size);
	}

	if (long Count = ~EntryWaiters) {
		SDQLOG(_T("WARNING: There are %d entry waiters"), Count);
		EntryEvent.Set();
	}

	if (long Count = ~EmptyWaiters) {
		SDQLOG(_T("WARNING: There are %d empty waiters"), Count);
		EmptyEvent.Set();
	}

	if (long Count = ~PreEntryWaiters) {
		SDQLOG(_T("WARNING: There are %d push waiters"), Count);
		PreEntryEvent.Set();
	}
}

template<class T>
bool TSyncBlockingDeque<T>::__WaitFor_Event(TEvent &Event, WAITTIME Timeout, TimeStamp const &StartTime, THandleWaitable *AbortEvent) {
	// Calculate how long to wait
	DWORD Delta = 0;
	if (Timeout != FOREVER) {
		Delta = (DWORD)StartTime.To(TimeStamp::Now()).GetValue(TimeUnit::MSEC);
		if (Delta > Timeout) return false;
	}
	Timeout -= Delta;

	return AbortEvent == nullptr ? Event.WaitFor(Timeout) == WaitResult::Signaled :
		WaitMultiple({Event, *AbortEvent}, false, Timeout) == WaitResult::Signaled_0;
}

template<class T>
void TSyncBlockingDeque<T>::__PreEntry_Check(void) {
	if (~EmptyWaiters) {
		TInitResource<long> WaitTicket(PreEntryWaiters++, [&](long &) {PreEntryWaiters--; });
		PreEntryEvent.WaitFor();
	}
}

template<class T>
typename TSyncBlockingDeque<T>::size_type TSyncBlockingDeque<T>::__Push_Do(TEntryCPush const &EntryPush, T const &entry) {
	__PreEntry_Check();
	auto Queue = __Safe_Pickup();
	return ((&Queue)->*EntryPush)(entry), __Signal_Event(EntryWaiters, EntryEvent, EmptyEvent), Queue->size();
}

template<class T>
typename TSyncBlockingDeque<T>::size_type TSyncBlockingDeque<T>::__Push_Do(TEntryMPush const &EntryPush, T &&entry) {
	__PreEntry_Check();
	auto Queue = __Safe_Pickup();
	return ((&Queue)->*EntryPush)(std::move(entry)), __Signal_Event(EntryWaiters, EntryEvent, EmptyEvent), Queue->size();
}

template<class T>
bool TSyncBlockingDeque<T>::__Pop_Do(TEntryGet const &EntryGet, TEntryDiscard const &EntryDiscard, T &entry) {
	auto Queue = __Safe_Pickup();
	size_t Size = Queue->size();
	if (Size > 0) {
		entry = std::move(((&Queue)->*EntryGet)()), ((&Queue)->*EntryDiscard)();
		if (!--Size) __Signal_Event(EmptyWaiters, EmptyEvent, EntryEvent);
		return true;
	}
	return false;
}

template<class T>
bool TSyncBlockingDeque<T>::__Pop(TPopFunc const &PopFunc, T &entry, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	if ((this->*PopFunc)(entry)) return true;

	TimeStamp EnterTime = TimeStamp::Now();
	while (true) {
		{
			// Signal we are in waiting stage
			TInitResource<long> WaitTicket(EntryWaiters++, [&](long &) {EntryWaiters--; });
			// Check again before long wait
			if ((this->*PopFunc)(entry)) return true;
			// Play the waiting game...
			if (!__WaitFor_Event(EntryEvent, Timeout, EnterTime, AbortEvent)) break;
		}
		// Check after signaled wakeup
		if ((this->*PopFunc)(entry)) return true;
	}
	return false;
}

template<class T>
typename TSyncBlockingDeque<T>::TLock TSyncBlockingDeque<T>::EmptyLock(WAITTIME Timeout, THandleWaitable *AbortEvent) {
	if (TLock _Ret{__LockEmpty()}) return _Ret;

	TimeStamp EnterTime = TimeStamp::Now();
	while (true) {
		{
			// Signal we are in waiting stage
			TInitResource<long> WaitTicket(EmptyWaiters++, [&](long &) {if (--EmptyWaiters == 0) PreEntryEvent.Set(); });
			if (*WaitTicket == 0) PreEntryEvent.Reset();
			// Check again before long wait
			if (TLock _Ret{__LockEmpty()}) return _Ret;
			// Play the waiting game...
			if (!__WaitFor_Event(EmptyEvent, Timeout, EnterTime, AbortEvent)) break;
		}
		// Check after signaled wakeup
		if (TLock _Ret{__LockEmpty()}) return _Ret;
	}
	return __LockEmpty();
}

template<class T>
typename TSyncBlockingDeque<T>::Container::const_iterator TSyncBlockingDeque<T>::LockedGetCIter(TLock const &Lock, TGetCIter const &GetCIter) {
	if (!Lock.For(SyncDeque.Lockable()))
		SDQFAIL(_T("Incorrect locking context"));
	if (!Lock)
		SDQFAIL(_T("Lock not engaged"));

	auto Queue = __Safe_Pickup();
	return ((&Queue)->*GetCIter)();
}

#undef SDQFAIL
#undef SDQLOG
#undef SDQLOGV
#undef SDQLOGVV

#endif //ZWUtils_SyncQueue_H