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
 * @addtogroup Utilities Basic Supporting Utilities
 * @file
 * @brief Resource Management Support
 * @author Zhenyu Wu
 * @date	Feb 12, 2015: Refactored from Allocator
 **/

#ifndef ZWUtils_Resource_H
#define ZWUtils_Resource_H

#include "Misc/Global.h"

#include "Allocator.h"
#include "Reference.h"

// Order is important:
#include "Debug/Exception.h"

#include <functional>

template<typename X>
class TResource : public Reference < X > {
	typedef TResource _this;

protected:
	X* _ObjPointer(void) const override
	{ return &const_cast<_this*>(this)->Refer(); }

	virtual X& Refer(void)
	{ FAIL(_T("Abstract function")); }

public:
	typedef std::function<X(void)> TResAlloc;
	typedef std::function<void(X &)> TResDealloc;

	static X NoAlloc(void)
	{ FAIL(_T("Function not available")); }
	static void NoDealloc(X &)
	{ FAIL(_T("Function not available")); }
	static void NullDealloc(X &) {}

	virtual ~TResource(void) {}
};

template<typename X>
class TInitResource : public TResource < X > {
	typedef TInitResource _this;

protected:
	X _ResRef;
	TResDealloc _Dealloc;

	X& Refer(void) override
	{ return _ResRef; }

public:
	TInitResource(X const &xResRef, TResDealloc const &xDealloc) :
		_ResRef(xResRef), _Dealloc(xDealloc) {}
	~TInitResource(void) override { _Dealloc(_ResRef); }

	// Copy consutrction does not make sense
	TInitResource(_this const &) = delete;
	// Move construction is not defined
	TInitResource(_this &&) = delete;

	// Copy assignment does not make sense
	_this& operator=(_this const &) = delete;
	// Move assignment is not defined
	_this& operator=(_this &&) = delete;
};

template<typename X>
class TAllocResource : public TResource < X > {
	typedef TAllocResource _this;

protected:
	X _ResRef;
	bool _ResValid = false;
	TResAlloc _Alloc;
	TResDealloc _Dealloc;

	X& Refer(void) override
	{ return _ResValid ? _ResRef : _ResRef = _Alloc(), _ResValid = true, _ResRef; }

public:
	TAllocResource(TResAlloc const &xAlloc, TResDealloc const &xDealloc) :
		_Alloc(xAlloc), _Dealloc(xDealloc) {}
	TAllocResource(X const &xResRef, TResDealloc const &xDealloc, TResAlloc const &xAlloc = NoAlloc) :
		_ResRef(xResRef), _ResValid(true), _Dealloc(xDealloc), _Alloc(xAlloc) {}
	~TAllocResource(void) override
	{ Deallocate(); }

	// Copy consutrction does not make sense
	TAllocResource(_this const &) = delete;
	// Move construction
	TAllocResource(_this &&xResource) :
		_ResRef(std::move(xResource._ResRef)), _ResValid(xResource._ResValid),
		_Alloc(std::move(xResource._Alloc)), _Dealloc(std::move(xResource._Dealloc)) {
		xResource.Invalidate();
	}

	// Copy assignment does not make sense
	_this& operator=(_this const &) = delete;
	// Move assignment
	_this& operator=(_this &&xResource) {
		_Alloc = std::move(xResource._Alloc);
		_Dealloc = std::move(xResource._Dealloc);
		_ResRef = std::move(xResource._ResRef);
		_ResValid = xResource._ResValid;
		xResource.Invalidate();
		return *this;
	}

	bool Allocated(void)
	{ return _ResValid; }
	bool Invalidate(void)
	{ return TInitResource<bool>(_ResValid, [&](bool &) { _ResValid = false; }), _ResValid; }
	virtual bool Deallocate(void)
	{ return TInitResource<bool>(_ResValid, [&](bool &X) { if (X) _Dealloc(_ResRef), _ResValid = false; }), _ResValid; }
};

#ifdef WINDOWS

typedef TAllocResource<HANDLE> THandle;
typedef TAllocResource<HMODULE> TModule;

#endif

template<typename T>
class TTypedBuffer : public TAllocResource < T* > {
	typedef TTypedBuffer _this;

protected:
	static void* Alloc(IAllocator &xAlloc, size_t xSize)
	{ return xSize ? xAlloc.Alloc(xSize) : nullptr; }

	static void Dealloc(IAllocator &xAlloc, void* const &xBuf)
	{ xAlloc.Dealloc(xBuf); }

public:
	TTypedBuffer(IAllocator &xAlloc = DefaultAllocator()) : TTypedBuffer(sizeof(T), xAlloc) {}
	TTypedBuffer(size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) :
		TAllocResource([=, &xAlloc] {return (T*)Alloc(xAlloc, xSize); }, [=, &xAlloc](T* &X) {Dealloc(xAlloc, X); }) {}
	TTypedBuffer(T* const &xBuffer, IAllocator &xAlloc = DefaultAllocator()) :
		TAllocResource(xBuffer, [=, &xAlloc](T* &X) {Dealloc(xAlloc, X); }) {}
	TTypedBuffer(T* const &xBuffer, size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) :
		TAllocResource(xBuffer, [=, &xAlloc](T* &X) {Dealloc(xAlloc, X); }, [=, &xAlloc] {return (T*)Alloc(xAlloc, xSize); }) {}

	// Move construction
	TTypedBuffer(_this &&xResource) : TAllocResource(std::move(xResource)) {}

	// Move assignment
	using TAllocResource::operator=;

	T* operator&(void) const
	{ return *_ObjPointer(); }
	T& operator*(void) const
	{ return **_ObjPointer(); }
	T* operator->(void) const
	{ return *_ObjPointer(); }
};

template<>
class TTypedBuffer<void> : public TAllocResource < void* > {
	typedef TTypedBuffer _this;

protected:
	static void* Alloc(IAllocator &xAlloc, size_t xSize)
	{ return xSize ? xAlloc.Alloc(xSize) : nullptr; }

	static void Dealloc(IAllocator &xAlloc, void* const &xBuf)
	{ xAlloc.Dealloc(xBuf); }

public:
	TTypedBuffer(size_t const &xSize = 0, IAllocator &xAlloc = DefaultAllocator()) :
		TAllocResource([=, &xAlloc] {return Alloc(xAlloc, xSize); }, [=, &xAlloc](void* &X) {Dealloc(xAlloc, X); }) {}
	TTypedBuffer(void* const &xBuffer, IAllocator &xAlloc = DefaultAllocator()) :
		TAllocResource(xBuffer, [=, &xAlloc](void* &X) {Dealloc(xAlloc, X); }) {}
	TTypedBuffer(void* const &xBuffer, size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) :
		TAllocResource(xBuffer, [=, &xAlloc](void* &X) {Dealloc(xAlloc, X); }, [=, &xAlloc] {return Alloc(xAlloc, xSize); }) {}

	// Move construction
	TTypedBuffer(_this &&xResource) : TAllocResource(std::move(xResource)) {}

	// Move assignment
	using TAllocResource::operator=;

	void* operator&(void) const
	{ return _ObjPointer(); }
};

typedef TTypedBuffer<void> TBuffer;

#endif