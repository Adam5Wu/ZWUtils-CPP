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
 * @brief Resource Management Support
 * @author Zhenyu Wu
 * @date	Feb 12, 2015: Refactored from Allocator
 **/

#ifndef ZWUtils_Resource_H
#define ZWUtils_Resource_H

 // Project global control 
#include "Misc/Global.h"

#include "Reference.h"

// Order is important here!
#include "Debug/Exception.h"

#include <functional>

template<typename X>
class TResource : public Reference<X> {
	typedef TResource _this;

protected:
	X* _ObjPointer(void) const override {
		return &const_cast<_this*>(this)->Refer();
	}

	X* _ObjExchange(X*) override {
		FAIL(_T("Function not available"));
	}

	virtual X& Refer(void) {
		FAIL(_T("Abstract function"));
	}

public:
	typedef std::function<X(void)> TResAlloc;
	typedef std::function<void(X &)> TResDealloc;

	static X NoAlloc() {
		FAIL(_T("Function not available"));
	}
	static void NoDealloc(X &) {
		FAIL(_T("Function not available"));
	}
	static void NullDealloc(X &) {}

	virtual ~TResource(void) {}
};

template<typename X>
class TInitResource : public TResource<X> {
	typedef TInitResource _this;

protected:
	X _ResRef;
	TResDealloc _Dealloc;

	X& Refer(void) override {
		return _ResRef;
	}

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

	bool Empty(void) const override {
		return false;
	}
};

template<typename X>
class TAllocResource : public TResource<X> {
	typedef TAllocResource _this;

protected:
	X _ResRef;
	bool _ResValid = false;
	TResAlloc _Alloc;
	TResDealloc _Dealloc;

	X& Refer(void) override {
		if (!_ResValid) {
			_ResRef = _Alloc();
			_ResValid = true;
		}
		return _ResRef;
	}

public:
	TAllocResource(TResAlloc const &xAlloc, TResDealloc const &xDealloc) :
		_Alloc(xAlloc), _Dealloc(xDealloc) {}
	TAllocResource(X const &xResRef, TResDealloc const &xDealloc, TResAlloc const &xAlloc = NoAlloc) :
		_ResRef(xResRef), _ResValid(true), _Dealloc(xDealloc), _Alloc(xAlloc) {}

	~TAllocResource(void) override {
		Deallocate();
	}

	// Copy consutrction does not make sense
	TAllocResource(_this const &) = delete;
	// Move construction
	TAllocResource(_this &&xResource) NOEXCEPT :
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

	bool Allocated(void) const {
		return _ResValid;
	}

	_this &Validate(void) {
		return Refer(), *this;
	}

	virtual bool Invalidate(void) {
		return _ResValid ? _ResValid = false, true : false;
	}

	virtual bool Deallocate(void) {
		return _ResValid ? _Dealloc(_ResRef), Invalidate() : false;
	}

	bool Empty(void) const override {
		return !Allocated();
	}

	void Clear(void) override {
		Deallocate();
	}

	X* Drop(void) override {
		X* Ret = Allocated() ? &_ResRef : nullptr;
		return Invalidate(), Ret;
	}

};

#ifdef WINDOWS

class THandle : public TAllocResource<HANDLE> {
	typedef THandle _this;

public:
	static HANDLE ValidateHandle(HANDLE const &Ref);
	static void HandleDealloc_Standard(HANDLE &Res);
	static void HandleDealloc_BestEffort(HANDLE &Res);

	THandle(TResAlloc const &xAlloc, TResDealloc const &xDealloc = HandleDealloc_Standard) :
		TAllocResource(xAlloc, xDealloc) {}
	THandle(HANDLE const &xResRef, TResDealloc const &xDealloc = HandleDealloc_Standard, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(ValidateHandle(xResRef), xDealloc, xAlloc) {}
	THandle(CONSTRUCTION::VALIDATED_T const&, HANDLE const &xResRef, TResDealloc const &xDealloc = HandleDealloc_Standard, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(xResRef, xDealloc, xAlloc) {}

#if _MSC_VER <= 1900
	// Older MS compilers are buggy at inheriting methods from template
	THandle(_this const &) = delete;
	THandle(_this &&xHandle) NOEXCEPT : TAllocResource(std::move(xHandle)) {}
	_this& operator=(_this const &) = delete;
	_this& operator=(_this &&xHandle)
	{ TAllocResource::operator=(std::move(xHandle)); }
#endif

	static _this Dummy(HANDLE const &xHandle)
	{ return _this(CONSTRUCTION::VALIDATED, xHandle, NullDealloc); }
};

class TModule : public TAllocResource<HMODULE> {
public:
	static HMODULE ValidateHandle(HMODULE const &Ref);
	static void HandleDealloc_Standard(HMODULE &Res);
	static void HandleDealloc_BestEffort(HMODULE &Res);

	TModule(TResAlloc const &xAlloc, TResDealloc const &xDealloc = HandleDealloc_Standard) :
		TAllocResource(xAlloc, xDealloc) {}
	TModule(HMODULE const &xResRef, TResDealloc const &xDealloc = HandleDealloc_Standard, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(ValidateHandle(xResRef), xDealloc, xAlloc) {}
	TModule(CONSTRUCTION::VALIDATED_T const&, HMODULE const &xResRef, TResDealloc const &xDealloc = HandleDealloc_Standard, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(xResRef, xDealloc, xAlloc) {}

	static TModule GetLoaded(TString const &Name);
	static TModule const MAIN;
};

#endif

//---------------------
// Fixed size buffers

#include "Allocator.h"

template<typename T>
class _TTypedBuffer : public TAllocResource<T*> {
	typedef _TTypedBuffer _this;

protected:
	size_t const _Size;

	static void* Alloc(IAllocator &xAllocator, size_t xSize) {
		if (!xSize) return nullptr;
		void* NewBuf = xAllocator.Alloc(xSize);
		if (!NewBuf) FAIL(_T("Memory allocation failure"));
		return NewBuf;
	}

	static void Dealloc(IAllocator &xAllocator, T* &xBuf) {
		xAllocator.Dealloc(xBuf);
	}

	_TTypedBuffer(IAllocator &xAllocator = DefaultAllocator())
		: _TTypedBuffer(sizeof(T), xAllocator) {}

	_TTypedBuffer(size_t const &xSize, IAllocator &xAllocator = DefaultAllocator()) :
		TAllocResource<T*>([=, &xAllocator] {return (T*)Alloc(xAllocator, xSize); },
						   [=, &xAllocator](T* &X) {Dealloc(xAllocator, X); }),
		_Size(xSize) {}

	_TTypedBuffer(T *xBuffer, IAllocator &xAllocator = DefaultAllocator())
		: _TTypedBuffer(xBuffer, sizeof(T), xAllocator) {}

	_TTypedBuffer(T *xBuffer, size_t const &xSize, IAllocator &xAllocator = DefaultAllocator()) :
		TAllocResource<T*>(xBuffer, [=, &xAllocator](T* &X) {Dealloc(xAllocator, X); },
						   [=, &xAllocator] {return (T*)Alloc(xAllocator, xSize); }),
		_Size(xSize) {}

public:
	//_TTypedBuffer(_this const &) = delete;
	_TTypedBuffer(_this &&xBuffer) NOEXCEPT
		: TAllocResource<T*>(std::move(xBuffer)), _Size(xBuffer._Size) {}

	//_this& operator=(_this const &) = delete;
	_this& operator=(_this &&xBuffer) {
		TAllocResource<T*>::operator=(std::move(xBuffer));
		*const_cast<size_t*>(&_Size) = xBuffer._Size;
		return *this;
	}

	size_t GetSize(void) const {
		return _Size;
	}

	T* operator&(void) const {
		return *_ObjPointer();
	}
};

template<typename T>
class TTypedBuffer : public _TTypedBuffer<T> {
	typedef TTypedBuffer _this;

public:
	TTypedBuffer(IAllocator &xAllocator = DefaultAllocator()) : _TTypedBuffer<T>(sizeof(T), xAllocator) {}
	TTypedBuffer(size_t const &xSize, IAllocator &xAllocator = DefaultAllocator()) :
		_TTypedBuffer<T>(xSize, xAllocator) {}
	TTypedBuffer(T &xBuffer, IAllocator &xAllocator = DefaultAllocator()) :
		_TTypedBuffer<T>(&xBuffer, xAllocator) {}
	TTypedBuffer(T &xBuffer, size_t const &xSize, IAllocator &xAllocator = DefaultAllocator()) :
		_TTypedBuffer<T>(&xBuffer, xSize, xAllocator) {}

#if _MSC_VER <= 1900
	// Older MS compilers are buggy at inheriting methods from template
	//TTypedBuffer(_this const &) = delete;
	TTypedBuffer(_this &&xBuffer) NOEXCEPT : _TTypedBuffer<T>(std::move(xBuffer)) {}
	//_this& operator=(_this const &) = delete;
	_this& operator=(_this &&xBuffer)
	{ return _TTypedBuffer<T>::operator=(std::move(xBuffer)), *this; }
#endif

	static _this ArrayOf(size_t const &Count, IAllocator &xAllocator = DefaultAllocator()) {
		return { sizeof(T) * Count, xAllocator };
	}

	static _this Unmanaged(void *xBuffer, size_t const &xSize = 0) {
		return { xBuffer, xSize, DummyAllocator() };
	}

	T& operator*(void) const {
		return **_ObjPointer();
	}
	T* operator->(void) const {
		return *_ObjPointer();
	}
	T& operator[](size_t idx) const {
		return (*_ObjPointer())[idx];
	}
};

template<>
class TTypedBuffer<void> : public _TTypedBuffer<void> {
	typedef TTypedBuffer _this;

public:
	TTypedBuffer(void) : TTypedBuffer(0, DummyAllocator()) {}
	TTypedBuffer(size_t const &xSize, IAllocator &xAllocator = DefaultAllocator()) :
		_TTypedBuffer(xSize, xAllocator) {}
	TTypedBuffer(void *xBuffer, size_t const &xSize, IAllocator &xAllocator = DefaultAllocator()) :
		_TTypedBuffer(xBuffer, xSize, xAllocator) {}

#if _MSC_VER <= 1900
	// Older MS compilers are buggy at inheriting methods from template
	//TTypedBuffer(_this const &) = delete;
	TTypedBuffer(_this &&xBuffer) NOEXCEPT : _TTypedBuffer<void>(std::move(xBuffer)) {}
	//_this& operator=(_this const &) = delete;
	_this& operator=(_this &&xBuffer)
	{ return _TTypedBuffer<void>::operator=(std::move(xBuffer)), *this; }
#endif

	template<class T>
	static _this Wrap(T *xBuffer, IAllocator &xAllocator = DefaultAllocator()) {
		return { xBuffer, sizeof(T), xAllocator };
	}

	static _this Unmanaged(void *xBuffer, size_t const &xSize = 0) {
		return { xBuffer, xSize, DummyAllocator() };
	}

};

typedef TTypedBuffer<void> TFixedBuffer;


//-----------------------
// Dynamic size buffers

#define DYNBUFFER_OVERPROVISION_MINIMUM     0x8       // 8B
#define DYNBUFFER_OVERPROVISION_NOSHRINK    0x100     // 256B
#define DYNBUFFER_OVERPROVISION_FIXBLOCK	0x1000000 // 16MB

template<typename T>
class _TTypedDynBuffer : public TAllocResource<T*> {
	typedef _TTypedDynBuffer _this;

private:
	size_t _PVSize;

protected:
	IAllocator & _Allocator;
	size_t _Size;

	size_t ProvisionSize(size_t xSize) {
		if (xSize >= DYNBUFFER_OVERPROVISION_FIXBLOCK)
			return (xSize + DYNBUFFER_OVERPROVISION_FIXBLOCK - 1) & ~(DYNBUFFER_OVERPROVISION_FIXBLOCK - 1);

		size_t Ret = (xSize >= DYNBUFFER_OVERPROVISION_NOSHRINK) ?
			DYNBUFFER_OVERPROVISION_NOSHRINK : DYNBUFFER_OVERPROVISION_MINIMUM;
		while (Ret < xSize) Ret <<= 1;
		return Ret;
	}

	T* Realloc(T* xBuf, size_t xSize) {
		if ((xSize < _Size && _PVSize > DYNBUFFER_OVERPROVISION_NOSHRINK) || xSize > _PVSize) {
			size_t PVSize = ProvisionSize(xSize);
			if (PVSize != _PVSize) {
				T* NewBuf = (T*)(xBuf ? _Allocator.Realloc(xBuf, PVSize) : _Allocator.Alloc(PVSize));
				if (NewBuf != nullptr) {
					_PVSize = PVSize;
					xBuf = NewBuf;
				}
			}
		}
		_Size = (xSize > _Size) ? std::min(xSize, _PVSize) : xSize;
		return xBuf;
	}

	void Dealloc(T* &xBuf) {
		_Allocator.Dealloc(xBuf);
		_PVSize = 0;
	}

	_TTypedDynBuffer(IAllocator &xAllocator = DefaultAllocator()) :
		_TTypedDynBuffer(sizeof(T), xAllocator) {}
	_TTypedDynBuffer(size_t const &xSize, IAllocator &xAllocator = DefaultAllocator()) :
		TAllocResource([&] { return Realloc(nullptr, _Size); }, [&](T* &X) { Dealloc(X); }),
		_PVSize(0), _Allocator(xAllocator), _Size(xSize) {}
	_TTypedDynBuffer(T* const &xBuffer, size_t const &xSize, IAllocator &xAllocator = DefaultAllocator()) :
		TAllocResource(xBuffer, [&](T* &X) { Dealloc(X); }, [&] { return Realloc(nullptr, _Size); }),
		_PVSize(xSize), _Allocator(xAllocator), _Size(xSize) {}

	// Move construction
	_TTypedDynBuffer(_this &&xResource) NOEXCEPT :
		TAllocResource(xResource._ResRef, [&](T* &X) { Dealloc(X); }, [&] { return Realloc(nullptr, _Size); }),
		_PVSize(xResource._PVSize), _Allocator(xResource._Allocator), _Size(xResource._Size) {
		_ResValid = xResource._ResValid;
		xResource.Invalidate();
	}

	// Move assignment
	_this& operator=(_this &&xResource) {
		T* TransBuf = xResource._ResRef;
		if (xResource._ResValid) {
			TransBuf = _Allocator.Transfer(TransBuf, xResource._Allocator);
			if (!TransBuf) FAIL(_T("Incompatible allocator"));
			// Remember to discard existing buffer
		}
		if (_ResValid) _Allocator.Dealloc(_ResRef);
		_ResRef = TransBuf;

		_PVSize = xResource._PVSize;
		_Size = xResource._Size;
		_ResValid = xResource._ResValid;
		// Do not move alloc/dealloc function since they are bound to instance
		xResource.Invalidate();
		return *this;
	}

public:
	T * operator&(void) const {
		return *_ObjPointer();
	}

	size_t GetSize(void) const {
		return _Size;
	}

	bool SetSize(size_t const &NewSize) {
		_ResRef = Realloc(_ResValid ? _ResRef : nullptr, NewSize);
		_ResValid = _ResRef != nullptr;
		return NewSize == _Size;
	}

	bool Grow(size_t const &AddSize) {
		return SetSize(_Size + AddSize);
	}

	virtual bool Invalidate(void) {
		return TAllocResource::Invalidate() ? _PVSize = 0, true : false;
	}

};

template<typename T>
class TTypedDynBuffer : public _TTypedDynBuffer<T> {
	typedef TTypedDynBuffer _this;

public:
	TTypedDynBuffer(IAllocator &xAllocator = DefaultAllocator()) : _TTypedDynBuffer(sizeof(T), xAllocator) {}
	TTypedDynBuffer(size_t const &xSize, IAllocator &xAllocator = DefaultAllocator()) :
		_TTypedDynBuffer(xSize, xAllocator) {}
	TTypedDynBuffer(T &xBuffer, size_t const &xSize, IAllocator &xAllocator = DefaultAllocator()) :
		_TTypedDynBuffer(&xBuffer, xSize, xAllocator) {}

#if _MSC_VER <= 1900
	// Older MS compilers are buggy at inheriting methods from template
	//TTypedDynBuffer(_this const &) = delete;
	TTypedDynBuffer(_this &&xBuffer) NOEXCEPT : _TTypedDynBuffer<T>(std::move(xBuffer)) {}
	//_this& operator=(_this const &) = delete;
	_this& operator=(_this &&xBuffer)
	{ return _TTypedDynBuffer<T>::operator=(std::move(xBuffer)), *this; }
#endif

	T& operator*(void) const {
		return **_ObjPointer();
	}

	T* operator->(void) const {
		return *_ObjPointer();
	}
};

template<>
class TTypedDynBuffer<void> : public _TTypedDynBuffer<void> {
	typedef TTypedDynBuffer _this;

public:
	TTypedDynBuffer(size_t const &xSize = 0, IAllocator &xAllocator = DefaultAllocator()) :
		_TTypedDynBuffer(xSize, xAllocator) {}
	TTypedDynBuffer(void* const &xBuffer, size_t const &xSize, IAllocator &xAllocator = DefaultAllocator()) :
		_TTypedDynBuffer(xBuffer, xSize, xAllocator) {}

#if _MSC_VER <= 1900
	// Older MS compilers are buggy at inheriting methods from template
	//TTypedDynBuffer(_this const &) = delete;
	TTypedDynBuffer(_this &&xBuffer) NOEXCEPT : _TTypedDynBuffer<void>(std::move(xBuffer)) {}
	//_this& operator=(_this const &) = delete;
	_this& operator=(_this &&xBuffer)
	{ return _TTypedDynBuffer<void>::operator=(std::move(xBuffer)), *this; }
#endif
};

typedef TTypedDynBuffer<void> TDynBuffer;

#endif