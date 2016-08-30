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

// [Utilities] Memory Management Support

#include "Allocator.h"

#include "Debug/Exception.h"

// IAllocator
#ifdef _DEBUG
void* IAllocator::Alloc(size_t Size, char const *FILE, int LINE) {
#else
void* IAllocator::Alloc(size_t Size) {
#endif
	FAIL(_T("Abstract function"));
}

void IAllocator::Dealloc(void *Mem) {
	FAIL(_T("Abstract function"));
}

void* IAllocator::Realloc(void *Mem, size_t Size) {
	FAIL(_T("Abstract function"));
}

size_t IAllocator::Size(void *Mem) {
	FAIL(_T("Abstract function"));
}

void* IAllocator::Transfer(void *Mem, _this &OAlloc) {
	FAIL(_T("Abstract function"));
}

// SimpleAllocator
#ifdef _DEBUG
void* SimpleAllocator::Alloc(size_t Size, char const *FILE, int LINE) {
	return _malloc_dbg(Size, _NORMAL_BLOCK, FILE, LINE);
#else
void* SimpleAllocator::Alloc(size_t Size) {
	return malloc(Size);
#endif
}

void SimpleAllocator::Dealloc(void *Mem) {
	free(Mem);
}

void* SimpleAllocator::Realloc(void *Mem, size_t Size) {
	return realloc(Mem, Size);
}

size_t SimpleAllocator::Size(void *Mem) {
	return _msize(Mem);
}

void* SimpleAllocator::Transfer(void *Mem, IAllocator &OAlloc) {
	return (*this == OAlloc) ? Mem : nullptr;
}

IAllocator& DefaultAllocator(void) {
	static SimpleAllocator __IoFU;
	return __IoFU;
}

// ExtAllocator
#ifdef _DEBUG
void* ExtAllocator::Alloc(size_t Size, char const *FILE, int LINE) {
#else
void* ExtAllocator::Alloc(size_t Size) {
#endif
	FAIL(_T("Should not reach"));
}

void* ExtAllocator::Realloc(void *Mem, size_t Size) {
	FAIL(_T("Should not reach"));
}

size_t ExtAllocator::Size(void *Mem) {
	FAIL(_T("Should not reach"));
}

void* ExtAllocator::Transfer(void *Mem, IAllocator &OAlloc) {
	return (dynamic_cast<_this*>(&OAlloc) != nullptr) ? Mem : nullptr;
}
