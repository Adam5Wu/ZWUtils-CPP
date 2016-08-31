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

	X* _ObjExchange(X *) override
	{ FAIL(_T("Function not available")); }

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

	bool Empty(void) const override
	{ return false; }
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

	bool Allocated(void) const
	{ return _ResValid; }
	bool Invalidate(void)
	{ return TInitResource<bool>(_ResValid, [&](bool &) { _ResValid = false; }), _ResValid; }
	virtual bool Deallocate(void)
	{ return TInitResource<bool>(_ResValid, [&](bool &X) { if (X) _Dealloc(_ResRef), _ResValid = false; }), _ResValid; }

	bool Empty(void) const override
	{ return !Allocated(); }

	void Clear(void) override
	{ Deallocate(); }

};

#ifdef WINDOWS

typedef TAllocResource<HANDLE> THandle;
typedef TAllocResource<HMODULE> TModule;

#endif

//---------------------
// Fixed size buffers

template<typename T>
class _TTypedBuffer : public TAllocResource < T* > {
	typedef _TTypedBuffer _this;

protected:
	static void* Alloc(IAllocator &xAlloc, size_t xSize)
	{ return xSize ? xAlloc.Alloc(xSize) : nullptr; }

	static void Dealloc(IAllocator &xAlloc, T* &xBuf)
	{ xAlloc.Dealloc(xBuf); }

	_TTypedBuffer(IAllocator &xAlloc = DefaultAllocator()) : _TTypedBuffer(sizeof(T), xAlloc) {}
	_TTypedBuffer(size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) :
		TAllocResource([=, &xAlloc] {return (T*)Alloc(xAlloc, xSize); }, [=, &xAlloc](T* &X) {Dealloc(xAlloc, X); }) {}
	_TTypedBuffer(T* const &xBuffer, IAllocator &xAlloc = DefaultAllocator()) :
		TAllocResource(xBuffer, [=, &xAlloc](T* &X) {Dealloc(xAlloc, X); }) {}
	_TTypedBuffer(T* const &xBuffer, size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) :
		TAllocResource(xBuffer, [=, &xAlloc](T* &X) {Dealloc(xAlloc, X); }, [=, &xAlloc] {return (T*)Alloc(xAlloc, xSize); }) {}

	// Move construction
	_TTypedBuffer(_this &&xResource) : TAllocResource(std::move(xResource)) {}

public:
	T* operator&(void) const
	{ return *_ObjPointer(); }
};

template<typename T>
class TTypedBuffer : public _TTypedBuffer < T > {
	typedef TTypedBuffer _this;

public:
	TTypedBuffer(IAllocator &xAlloc = DefaultAllocator()) : _TTypedBuffer(sizeof(T), xAlloc) {}
	TTypedBuffer(size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) : _TTypedBuffer(xSize, xAlloc) {}
	TTypedBuffer(T* const &xBuffer, IAllocator &xAlloc = DefaultAllocator()) : _TTypedBuffer(xBuffer, xAlloc) {}
	TTypedBuffer(T* const &xBuffer, size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) :
		_TTypedBuffer(xBuffer, xSize, xAlloc) {}

	// Move construction
	TTypedBuffer(_this &&xResource) : _TTypedBuffer(std::move(xResource)) {}

	// Move assignment
	using TAllocResource::operator=;

	T& operator*(void) const
	{ return **_ObjPointer(); }
	T* operator->(void) const
	{ return *_ObjPointer(); }
};

template<>
class TTypedBuffer<void> : public _TTypedBuffer < void >{
	typedef TTypedBuffer _this;

public:
	TTypedBuffer(size_t const &xSize = 0, IAllocator &xAlloc = DefaultAllocator()) : _TTypedBuffer(xSize, xAlloc) {}
	TTypedBuffer(void* const &xBuffer, IAllocator &xAlloc = DefaultAllocator()) : _TTypedBuffer(xBuffer, xAlloc) {}
	TTypedBuffer(void* const &xBuffer, size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) :
		_TTypedBuffer(xBuffer, xSize, xAlloc) {}

	// Move construction
	TTypedBuffer(_this &&xResource) : _TTypedBuffer(std::move(xResource)) {}

	// Move assignment
	using TAllocResource::operator=;
};

typedef TTypedBuffer<void> TFixedBuffer;


//-----------------------
// Dynamic size buffers

#define DYNBUFFER_OVERPROVISION_THRESHOLD	0x1000

template<typename T>
class _TTypedDynBuffer : public TAllocResource < T* > {
	typedef _TTypedDynBuffer _this;

private:
	size_t _PVSize = 0;

protected:
	IAllocator &_Alloc;
	size_t _Size;

	T* Alloc(T* &xBuf) {
		if (_Size < DYNBUFFER_OVERPROVISION_THRESHOLD) {
			_PVSize = _Size;
			return _Size ? (T*)(xBuf ? _Alloc.Realloc(xBuf, _Size) : _Alloc.Alloc(_Size)) : Dealloc(xBuf);
		} else {
			if (_Size < _PVSize) return xBuf;
			_PVSize = max(_PVSize, DYNBUFFER_OVERPROVISION_THRESHOLD);
			while ((_PVSize <<= 1) < _Size);
			return _Alloc.Realloc(xBuf, _PVSize);
		}
	}

	T* Dealloc(T* &xBuf)
	{ return _Alloc.Dealloc(xBuf), xBuf = nullptr; }

	_TTypedDynBuffer(IAllocator &xAlloc = DefaultAllocator()) : _TTypedDynBuffer(sizeof(T), xAlloc) { _ResRef = nullptr; }
	_TTypedDynBuffer(size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) :
		TAllocResource([&] { return Alloc(_ResRef); }, [&](T* &X) { Dealloc(X); }), _Alloc(xAlloc), _Size(xSize) { _ResRef = nullptr; }
	_TTypedDynBuffer(T* const &xBuffer, size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) :
		TAllocResource(xBuffer, [&](T* &X) { Dealloc(X); }, [&] { return Alloc(_ResRef); }), _Alloc(xAlloc), _Size(xSize) {}

	// Move construction
	_TTypedDynBuffer(_this &&xResource) :
		TAllocResource(std::move(xResource)), _Alloc(xResource._Alloc), _Size(xResource._Size) {}

	// Move assignment
	_this& operator=(_this &&xResource) {
		if (!isCompatible(xResource)) FAIL(_T("Incompatible allocator"));
		_Size = xResource._Size;
		TAllocResource::operator=(std::move(xResource));
	}

public:
	T* operator&(void) const
	{ return *_ObjPointer(); }

	bool isCompatible(_this const &xResource) const
	{ return xResource._Alloc == _Alloc; }

	size_t GetSize(void) const
	{ return _Size; }

	size_t SetSize(size_t const &NewSize) {
		size_t OldSize = _Size;
		if (NewSize < DYNBUFFER_OVERPROVISION_THRESHOLD) {
			_Size = (_Size != NewSize) ? Invalidate(), NewSize : _Size;
		} else {
			if (NewSize > _PVSize) Invalidate();
			_Size = NewSize;
		}
		return OldSize;
	}

	size_t Grow(size_t const &AddSize)
	{ return SetSize(_Size + AddSize); }
};

template<typename T>
class TTypedDynBuffer : public _TTypedDynBuffer < T > {
	typedef TTypedDynBuffer _this;

public:
	TTypedDynBuffer(IAllocator &xAlloc = DefaultAllocator()) : _TTypedDynBuffer(sizeof(T), xAlloc) {}
	TTypedDynBuffer(size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) : _TTypedDynBuffer(xSize, xAlloc) {}
	TTypedDynBuffer(T* const &xBuffer, size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) :
		_TTypedDynBuffer(xBuffer, xSize, xAlloc) {}

	// Move construction
	TTypedDynBuffer(_this &&xResource) : _TTypedDynBuffer(std::move(xResource)) {}

	// Move assignment
	using _TTypedDynBuffer::operator=;

	T& operator*(void) const
	{ return **_ObjPointer(); }
	T* operator->(void) const
	{ return *_ObjPointer(); }
};

template<>
class TTypedDynBuffer<void> : public _TTypedDynBuffer < void >{
	typedef TTypedDynBuffer _this;

public:
	TTypedDynBuffer(size_t const &xSize = 0, IAllocator &xAlloc = DefaultAllocator()) : _TTypedDynBuffer(xSize, xAlloc) {}
	TTypedDynBuffer(void* const &xBuffer, size_t const &xSize, IAllocator &xAlloc = DefaultAllocator()) :
		_TTypedDynBuffer(xBuffer, xSize, xAlloc) {}

	// Move construction
	TTypedDynBuffer(_this &&xResource) : _TTypedDynBuffer(std::move(xResource)) {}

	// Move assignment
	using _TTypedDynBuffer::operator=;
};

typedef TTypedDynBuffer<void> TDynBuffer;

#endif