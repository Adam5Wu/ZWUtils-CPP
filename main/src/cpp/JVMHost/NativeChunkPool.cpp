/*
Copyright (c) 2005 - 2018, Zhenyu Wu; 2012 - 2018, NEC Labs America Inc.
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

#ifdef __ZWUTILS_JAVAEMBED

// [AdvUtils] Data chunk pool for Java transport

#include "NativeChunkPool.h"

#include "Debug/Exception.h"
#include "Debug/Logging.h"

TNativeChunkPool::TNativeChunkPool(size_t xSlackLimit) :
	Store(_T("NativeChunks")), PoolLimit(0), WaterMark(0), SlackLimit(xSlackLimit) {
	if (SlackLimit < 1) FAIL(_T("Invalid slack count, expect at least 1"));
}

TNativeChunkPool::~TNativeChunkPool(void) {
	auto StoreLock = Store.Lock();
	// Check for balanced pool
	auto StoreSize = Store.Length();
	if (StoreSize != PoolLimit) {
		LOG(_T("WARNING: Unbalanced storage, delta %d"), StoreSize - PoolLimit);
	}

	DEBUG_DO({
		// Remove pooled chunks
		MRNativeChunk DiscardChunk;
		while (Store.Pop_Back(DiscardChunk, 0));
			 });
}

void TNativeChunkPool::Charge(size_t Count, TChunkCreator const &Creator) {
	PoolLimit += Count;
	while (Count--) Store.Push_Back(MRNativeChunk(Creator(), CONSTRUCTION::HANDOFF));
}

bool TNativeChunkPool::Acquire(MRNativeChunk &Chunk, WAITTIME Timeout, THandleWaitable *AbortEvent) {
	return Store.Pop_Front(Chunk, Timeout, AbortEvent);
}

void TNativeChunkPool::Return(TNativeChunk *Chunk) {
	MRNativeChunk iEntry(Chunk, CONSTRUCTION::HANDOFF);
	size_t WaterLevel = PoolLimit - Store.Push_Front(std::move(iEntry)) + 1;
	if (WaterLevel > WaterMark) {
		WaterMark = WaterLevel;
		return;
	}
	size_t Slack = WaterMark - WaterLevel;
	if (Slack > SlackLimit) {
		TLockable::MRLock StoreLock(CONSTRUCTION::EMPLACE, Store.Lock());
		// At this point no more get-from-pool can happen

		// Test water level again in race-free condition
		WaterLevel = PoolLimit - Store.Length();
		if (WaterMark <= WaterLevel) return;
		Slack = WaterMark - WaterLevel;
		if (Slack <= SlackLimit) return;

		// We can safely proceed with release
		auto Iter = Store.cbegin(StoreLock);
		while (--Slack) (*(++Iter))->Release();
		WaterMark = 1;
	}
}

#endif