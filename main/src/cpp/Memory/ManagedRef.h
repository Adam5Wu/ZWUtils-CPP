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
 * @brief Managed Object Reference Container
 * @author Zhenyu Wu
 * @date Jan 07, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_ManagedRef_H
#define ZWUtils_ManagedRef_H

 // Project global control 
#include "Misc/Global.h"

#include "Misc/TString.h"
#include "Misc/Types.h"

#include "ObjAllocator.h"
#include "Reference.h"

template<class T>
class ManagedRef final : public Reference<T> {
	typedef ManagedRef _this;

private:
	TInterlockedOrdinal<T*> _Obj = nullptr;
	IObjAllocator<T> &_Alloc;

protected:
	static T* _RefObj(T *xObj);
	static T* _RelObj(T *xObj);
	static T* _DupObj(T *xObj, bool ForceClone, IObjAllocator<T> &xAlloc);

	T* _ObjPointer(void) const override {
		return ~_Obj;
	}

	T* _ObjExchange(T *xObj) override {
		return _Alloc.Destroy(_RelObj(_Obj.Exchange(xObj))), nullptr;
	}

public:
	ManagedRef(IObjAllocator<T> &xAlloc = DefaultObjAllocator<T>()) :
		_Alloc(xAlloc) {}

	template<typename... Params>
	ManagedRef(IObjAllocator<T> &xAlloc, Params&&... xParams) :
		_Alloc(xAlloc) {
		_Obj = _RefObj(xAlloc.Create(RLAMBDANEW(T, std::forward<Params>(xParams)...)));
	}

	ManagedRef(CONSTRUCTION::EMPLACE_T const&, IObjAllocator<T> &xAlloc = DefaultObjAllocator<T>()) :
		_Obj(_RefObj(xAlloc.Create())), _Alloc(xAlloc) {}

	template<typename... Params>
	ManagedRef(CONSTRUCTION::EMPLACE_T const&, Params&&... xParams) :
		ManagedRef(DefaultObjAllocator<T>(), std::forward<Params>(xParams)...) {}

	ManagedRef(T *xObj, IObjAllocator<T> &xAlloc = DefaultObjAllocator<T>()) :
		_Obj(_DupObj(xObj, false, xAlloc)), _Alloc(xAlloc) {}

	ManagedRef(T *xObj, CONSTRUCTION::CLONE_T const&, IObjAllocator<T> &xAlloc = DefaultObjAllocator<T>()) :
		_Obj(_DupObj(xObj, true, xAlloc)), _Alloc(xAlloc) {}

	ManagedRef(T *xObj, CONSTRUCTION::HANDOFF_T const&, IObjAllocator<T> &xAlloc = DefaultObjAllocator<T>()) :
		_Obj(_RefObj(xObj)), _Alloc(xAlloc) {}

	// Copy constructor
	ManagedRef(_this const &xMR) : _Obj(_DupObj(&xMR, false, xMR._Alloc)), _Alloc(xMR._Alloc) {}
	// Move constructor
	ManagedRef(_this &&xMR) noexcept : _Obj(xMR.Drop()), _Alloc(xMR._Alloc) {}

	// Note: For performance reasons, we do not have a virtual destructor
	// Hence we seal this class and do not allow further derivation
	~ManagedRef(void) { _Alloc.Destroy(_RelObj(_Obj.Exchange(nullptr))); }

	_this& operator=(_this const &xMR);
	_this& operator=(_this &&xMR);

	T* Drop(void) override {
		return _Obj.Exchange(nullptr);
	}
};

#include "ManagedObj.h"
#include "Debug/Exception.h"

template<class T>
T* ManagedRef<T>::_RefObj(T *xObj) {
	if (auto MRef = ManagedObj::Cast(xObj))
		MRef->_AddRef();
	return xObj;
}

template<class T>
T* ManagedRef<T>::_RelObj(T *xObj) {
	if (auto MRef = ManagedObj::Cast(xObj))
		return MRef->_RemoveRef() ? xObj : nullptr;
	return xObj;
}

template<class T>
T* ManagedRef<T>::_DupObj(T *xObj, bool ForceClone, IObjAllocator<T> &xAlloc) {
	if (xObj == nullptr) return nullptr;
	if (!ForceClone) {
		if (auto MRef = ManagedObj::Cast(xObj))
			return MRef->_AddRef(), xObj;
	}
	return Cloneable::Clone(xObj, xAlloc);
}

template<class T>
ManagedRef<T>& ManagedRef<T>:: operator=(ManagedRef const &xMR) {
	if (_Alloc != xMR._Alloc) FAIL(_T("Incompatible allocator"));
	return Assign(&xMR), *this;
}

template<class T>
ManagedRef<T>& ManagedRef<T>:: operator=(ManagedRef &&xMR) {
	T* TransObj = _Alloc.Transfer(&xMR, xMR._Alloc);
	if (!TransObj) FAIL(_T("Incompatible allocator"));
	return Assign(TransObj), xMR.Drop(), *this;
}

#endif