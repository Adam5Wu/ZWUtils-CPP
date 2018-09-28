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

// [AdvUtils] Data chunk object for Java transport

#include "NativeChunk.h"

UINT8 const TNativeChunkV1::DATATYPE_RAW = 0;
NativeChunkBuf::TSIG const TNativeChunkV1::SIG{{'C', 'X', 'J', '1'}};

void TNativeChunkV1::Init(PNativeChunkBuf Buffer) {
	Buffer->SIG = SIG;
	PNativeChunkBufV1 BufferV1 = (PNativeChunkBufV1)Buffer;
	BufferV1->RetTicket.Value[0] = this;
	Reset();
}

bool TNativeChunkV1::Reset(void) {
	if (_ResValid) {
		PNativeChunkBufV1 BufferV1 = (PNativeChunkBufV1)_ResRef;
		BufferV1->Type = DataType;
		BufferV1->Flags = 0;
		BufferV1->Counts = 0;
	}
	FreePos = 0;
	return _ResValid;
}

void* TNativeChunkV1::AllocBuffer(size_t xSize) {
	if (!ProbeData(xSize)) return nullptr;

	PNativeChunkBufV1 BufferV1 = (PNativeChunkBufV1)Refer();
	void* Ret = &BufferV1->Data + FreePos;
	FreePos += xSize;
	return Ret;
}
