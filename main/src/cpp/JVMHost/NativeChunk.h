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

/**
 * @addtogroup AdvUtils Advanced Supporting Utilities
 * @file
 * @brief Data chunk object for Java transport
 * @author Zhenyu Wu
 * @date Sep 27, 2018: Refactored business logic independent code from AgentLib-NG
 **/

#ifndef ZWUtils_NativeChunk_H
#define ZWUtils_NativeChunk_H

#include "Misc/Global.h"
#include "Misc/Types.h"
#include "Misc/Units.h"
#include "Debug/Exception.h"
#include "Memory/Resource.h"
#include "Memory/ManagedRef.h"

#pragma pack(push, 1)

typedef _declspec(align(1)) struct _NativeChunkBuf {
	_declspec(align(1))	struct TSIG {
		_declspec(align(1)) UINT8 Value[4];
	} SIG;
} NativeChunkBuf, *PNativeChunkBuf;

#pragma pack(pop)

class TNativeChunk : protected TTypedBuffer<NativeChunkBuf> {
	typedef TNativeChunk _this;

protected:
	TNativeChunk(size_t Size, IAllocator &xAlloc = DefaultAllocator()) : TTypedBuffer(Size, xAlloc) {}

	PNativeChunkBuf& Refer(void) override
	{ return _ResValid ? _ResRef : Init(TTypedBuffer::Refer()), _ResRef; }

	virtual void Init(PNativeChunkBuf Buffer)
	{ FAIL(_T("Abstract function")); }

public:
	TNativeChunk(_this &&) = delete;
	_this& operator=(_this &&) = delete;

	virtual size_t Size(void)
	{ FAIL(_T("Abstract function")); }

	virtual bool Release(void)
	{ return Deallocate(); }

	virtual void* AllocBuffer(size_t xSize)
	{ FAIL(_T("Abstract function")); }

	PNativeChunkBuf GetBufferHead(void)
	{ return (PNativeChunkBuf)Refer(); }
};

typedef ManagedRef<TNativeChunk> MRNativeChunk;

#pragma pack(push, 1)

typedef _declspec(align(1)) struct _NativeChunkBufV1 : _NativeChunkBuf {
	_declspec(align(1))	UINT8 Type;
	_declspec(align(1))	UINT8 Flags;
	_declspec(align(1))	UINT16 Counts;
	_declspec(align(1))	struct {
		_declspec(align(1)) PVOID Value[2];
	} RetTicket;
	_declspec(align(1))	UINT8 Data;
} NativeChunkBufV1, *PNativeChunkBufV1;

#pragma pack(pop)

class TNativeChunkV1 : public TNativeChunk {
	typedef TNativeChunkV1 _this;

public:
	static NativeChunkBuf::TSIG const SIG;
	static UINT8 const DATATYPE_RAW;

protected:
	UINT8 const DataType;
	size_t const DataSize;
	size_t FreePos = 0;

	void Init(PNativeChunkBuf Buffer) override;

	bool TNativeChunkV1::ProbeData(size_t xSize)
	{ return FreePos + xSize <= DataSize; }

public:
	TNativeChunkV1(size_t Size, UINT8 Type = DATATYPE_RAW, IAllocator &xAlloc = DefaultAllocator()) :
		TNativeChunk(Size, xAlloc), DataSize(Size - (sizeof(NativeChunkBufV1) - 1)), DataType(Type) {}

	virtual size_t Size(void) override
	{ return _ResValid? FreePos + sizeof(NativeChunkBufV1) - 1 : 0; }

	virtual void* AllocBuffer(size_t xSize) override;

	bool Reset(void);
};

#endif //ZWUtils_NativeChunk_H
