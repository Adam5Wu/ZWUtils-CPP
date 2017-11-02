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
 * @addtogroup Utilities Basic Supporting Utilities
 * @file
 * @brief Memory Management Support
 * @author Zhenyu Wu
 * @date Jan 07, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_Allocator_H
#define ZWUtils_Allocator_H

 // Project global control 
#include "Misc/Global.h"

#include <xstddef>

/**
 * @ingroup Utilities
 * @brief Abstract memory allocator
 *
 * Manages the life cycle of a piece of memory
 **/
class IAllocator {
	typedef IAllocator _this;

protected:
	virtual ~IAllocator(void) {}

public:
	/**
	 * Allocate a piece of RAM
	 **/
#ifdef _DEBUG
	virtual void* Alloc(size_t Size, char const *FILE = __FILE__, int LINE = __LINE__);
#else
	virtual void* Alloc(size_t Size);
#endif

	/**
	 * Deallocate a piece of memory
	 **/
	virtual void Dealloc(void *Mem);

	/**
	 * Resize a piece of allocated memory
	 **/
	virtual void* Realloc(void *Mem, size_t Size);

	/**
	 * Get the size of a piece of allocated memory
	 **/
	virtual size_t Size(void *Mem);

	/**
	 * Transfer ownership of an allocated memory from another allocator
	 **/
	virtual void* Transfer(void *Mem, _this &OAlloc);
};

inline bool operator ==(IAllocator const &A, IAllocator const &B) {
	return std::addressof(A) == std::addressof(B);
}

inline bool operator !=(IAllocator const &A, IAllocator const &B) {
	return !(A == B);
}

/**
 * @ingroup Utilities
 * @brief Simple memory allocator
 *
 * Delegate management to RTL memory manager
 **/
class SimpleAllocator : public IAllocator {
	typedef SimpleAllocator _this;

public:
	~SimpleAllocator(void) override {}

#ifdef _DEBUG
	void* Alloc(size_t Size, char const *FILE, int LINE) override;
#else
	void* Alloc(size_t Size) override;
#endif
	void Dealloc(void *Mem) override;
	void* Realloc(void *Mem, size_t Size) override;
	size_t Size(void *Mem) override;
	void* Transfer(void *Mem, IAllocator &OAlloc) override;
};

IAllocator& DefaultAllocator(void);

/**
 * @ingroup Utilities
 * @brief External managed memory allocator
 **/
class ExtAllocator : public IAllocator {
	typedef ExtAllocator _this;

public:
	~ExtAllocator(void) override {}

#ifdef _DEBUG
	void* Alloc(size_t Size, char const *FILE, int LINE) override;
#else
	void* Alloc(size_t Size) override;
#endif
	void Dealloc(void *Mem) override {}
	void* Realloc(void *Mem, size_t Size) override;
	size_t Size(void *Mem) override;
	void* Transfer(void *Mem, IAllocator &OAlloc) override;
};

IAllocator& DummyAllocator(void);

#endif
