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

/**
 * @addtogroup AdvUtils Advanced Supporting Utilities
 * @file
 * @brief Data chunk pool for Java transport
 * @author Zhenyu Wu
 * @date Sep 27, 2018: Refactored business logic independent code from AgentLib-NG
 **/

#ifndef ZWUtils_NativeChunkPool_H
#define ZWUtils_NativeChunkPool_H

 // Project global control
#include "Misc/Global.h"

#include "NativeChunk.h"
#include "Threading/SyncContainers.h"

#include <functional>

class TNativeChunkPool {
protected:
	typedef TSyncBlockingDeque<MRNativeChunk> TChunkStore;
	TChunkStore Store;

	size_t PoolLimit;
	size_t WaterMark;

	TNativeChunkPool(size_t xSlackLimit);

	typedef std::function<TNativeChunk*(void)> TChunkCreator;
	// The setup function is suppose to be called before using the pool, in single-thread context
	void Charge(size_t Count, TChunkCreator const &Creator);

public:
	size_t const SlackLimit;

	virtual ~TNativeChunkPool(void);

	// This function can be called safely in multi-thread context
	bool Acquire(MRNativeChunk &Chunk, WAITTIME Timeout = FOREVER, THandleWaitable *AbortEvent = nullptr);

	// This function must be called in single-thread context
	void Return(TNativeChunk *Chunk);
};

#endif //ZWUtils_NativeChunkPool_H

#endif