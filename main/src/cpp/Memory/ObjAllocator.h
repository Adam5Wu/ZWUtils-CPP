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
 * @brief Object Management Support
 * @author Zhenyu Wu
 * @date	Feb 12, 2015: Refactored from Allocator
 **/

#ifndef ZWUtils_ObjAllocator_H
#define ZWUtils_ObjAllocator_H

 // Project global control 
#include "Misc/Global.h"

#include "Misc/Types.h"

#include <functional>

class IAllocator;

template<class T>
/**
 * @ingroup Utilities
 * @brief Abstract object allocator
 *
 * Manages the life cycle of an object
 * Note: To use the Create() routine, the object must be "default constructable"
 **/
class IObjAllocator {
	typedef IObjAllocator _this;

protected:
	virtual ~IObjAllocator(void) {}

	static T* _DefNew(void *X) { return new (X)T; }

	MEMBERFUNC_PROBE(__Pre_Destroy);

	template<class Q = T>
	static typename std::enable_if<!Has___Pre_Destroy<Q>::value>::type
		_DefDestroy(Q *Obj) { if (Obj) Obj->~Q(); }

	template<class Q = T>
	static typename std::enable_if<Has___Pre_Destroy<Q>::value>::type
		_DefDestroy(Q *Obj) { if (Obj) Obj->__Pre_Destroy(), Obj->~Q(); }

public:
	typedef std::function<T*(void*)> TNew;

	/**
	* Create an object (Construction in xNew)
	**/
	virtual T* Create(TNew const &xNew = _DefNew);

	/**
	* Destroy an object
	**/
	virtual void Destroy(T *Obj);

	/**
	* Transfer ownership of an object from another allocator
	**/
	virtual T* Transfer(T *Obj, _this &OAlloc);

	/**
	* Adopt ownership of an object bond to another raw allocator
	**/
	virtual T* Adopt(T *Obj, IAllocator &OAlloc);

	/**
	* Drop ownership of an object from this allocator, if bond to raw allocator could be maintained
	**/
	virtual T* Drop(T *Obj);

	/**
	* Get an raw buffer allocator that is compatible with this allocator
	**/
	virtual IAllocator &RAWAllocator(void) const;
};

#define RLAMBDANEW(cls, ...) [&](void *__X) {return new (__X) cls (__VA_ARGS__);}

template<class T>
bool operator ==(IObjAllocator<T> const &A, IObjAllocator<T> const &B) {
	return std::addressof(A) == std::addressof(B);
}
template<class T>
bool operator !=(IObjAllocator<T> const &A, IObjAllocator<T> const &B) {
	return !(A == B);
}

#include "Allocator.h"

template<class T> class TTypedBuffer;

template<class T>
/**
 * @ingroup Utilities
 * @brief Cascade Object Allocator
 *
 * Delegate memory management to an allocator
 **/
class CascadeObjAllocator : public IObjAllocator<T> {
	typedef CascadeObjAllocator _this;

protected:
	IAllocator &_Alloc;

public:
	CascadeObjAllocator(IAllocator &xAlloc = DefaultAllocator()) : _Alloc(xAlloc) {}
	~CascadeObjAllocator(void) override {}

	T* Create(TNew const &xNew) override {
		TTypedBuffer<T> ObjMem(_Alloc);
		auto *Ret = xNew(&ObjMem);
		return ObjMem.Invalidate(), Ret;
	}
	void Destroy(T *Obj) override {
		_DefDestroy(Obj), _Alloc.Dealloc(Obj);
	}
	T* Transfer(T *Obj, IObjAllocator<T> &OAlloc) override {
		return Adopt(OAlloc.Drop(Obj), OAlloc.RAWAllocator());
	}
	T* Adopt(T *Obj, IAllocator &OAlloc) override {
		return (T*)_Alloc.Transfer(Obj, OAlloc);
	}
	T* Drop(T *Obj) override {
		return Obj;
	}
	IAllocator& RAWAllocator(void) const override {
		return _Alloc;
	}
};

template<class T>
IObjAllocator<T>& DefaultObjAllocator(void) {
	static CascadeObjAllocator<T> __IoFU;
	return __IoFU;
}

#define DEFAULT_NEW(cls, ...) DefaultObjAllocator<cls>().Create(RLAMBDANEW(cls, __VA_ARGS__))
#define DEFAULT_DESTROY(cls, obj) DefaultObjAllocator<cls>().Destroy(obj)

#include "Resource.h"

template<class T>
/**
 * @ingroup Utilities
 * @brief External managed object allocator
 **/
class ExtObjAllocator : public IObjAllocator<T> {
	typedef ExtObjAllocator _this;

public:
	~ExtObjAllocator(void) override {}

	T* Create(TNew const &xNew) override;
	void Destroy(T *Obj) {}
	T* Transfer(T *Obj, IObjAllocator<T> &OAlloc) {
		return dynamic_cast<_this*>(std::addressof(OAlloc)) ? Obj : nullptr;
	}
	T* Adopt(T *Obj, IAllocator &OAlloc) override;
	T* Drop(T *Obj) override;
	IAllocator& RAWAllocator(void) const override;
};

#include "Debug/Exception.h"

template<class T>
T* IObjAllocator<T>::Create(TNew const &xNew) {
	FAIL(_T("Abstract function"));
}

template<class T>
void IObjAllocator<T>::Destroy(T *Obj) {
	FAIL(_T("Abstract function"));
}

template<class T>
T* IObjAllocator<T>::Transfer(T *Obj, _this &OAlloc) {
	FAIL(_T("Abstract function"));
}

template<class T>
T* IObjAllocator<T>::Adopt(T *Obj, IAllocator &OAlloc) {
	FAIL(_T("Abstract function"));
}

template<class T>
T* IObjAllocator<T>::Drop(T *Obj) {
	FAIL(_T("Abstract function"));
}

template<class T>
IAllocator& IObjAllocator<T>::RAWAllocator(void) const {
	FAIL(_T("Abstract function"));
}

template<class T>
T* ExtObjAllocator<T>::Create(TNew const &xNew) {
	FAIL(_T("Not supported"));
}

template<class T>
T* ExtObjAllocator<T>::Adopt(T *Obj, IAllocator &OAlloc) {
	FAIL(_T("Not supported"));
}

template<class T>
T* ExtObjAllocator<T>::Drop(T *Obj) {
	FAIL(_T("Not supported"));
}

template<class T>
IAllocator& ExtObjAllocator<T>::RAWAllocator(void) const {
	FAIL(_T("Not supported"));
}

#endif
