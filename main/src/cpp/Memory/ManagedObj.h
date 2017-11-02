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
 * @brief Managed Object Helpers
 * @author Zhenyu Wu
 * @date Jan 07, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_ManagedObj_H
#define ZWUtils_ManagedObj_H

#define __MANAGEDOBJ_LITE

 // Project global control 
#include "Misc/Global.h"

#include "Misc/TString.h"
#include "Misc/Types.h"

#include "ObjAllocator.h"

class Cloneable : public TCastable<Cloneable> {
	typedef Cloneable _this;

protected:
	virtual _this* MakeClone(IObjAllocator<void> &_Alloc) const;

public:
	template<class T>
	static T* Clone(T const *xObj, IObjAllocator<T> &xAlloc = DefaultObjAllocator<T>());
};

class ManagedObj : public TCastable<ManagedObj> {
	typedef ManagedObj _this;

private:
	typedef TInterlockedOrdinal32<long> TSyncCounter;
	TSyncCounter _RefCount;

protected:
	ManagedObj(long xRefCount = 0) : _RefCount(xRefCount) {}

#ifdef __MANAGEDOBJ_LITE
	// Disable virtual destructor for performance reasons
#else
	virtual ~ManagedObj(void);
#endif

public:
	void _AddRef(void) {
		++_RefCount;
	}
	bool _RemoveRef(void) {
		return --_RefCount == 0;
	}
	long RefCount(void) const {
		return ~_RefCount;
	}

	virtual TString toString(void) const {
		return TStringCast(_T("MObj@") << (void*)this << _T('(') << RefCount() << _T(')'));
	}
};

template<class TObject>
class ManagedObjAdapter final : public TObject, public ManagedObj {
	ENFORCE_POLYMORPHIC(TObject);
	typedef ManagedObjAdapter _this;
	friend IObjAllocator<_this>;

private:
	MEMBERFUNC_PROBE(toString);

	template<typename X = TObject>
	auto _toString(bool Debug = false) const -> decltype(std::enable_if<Has_toString<X>::value, TString>::type()) {
		return Debug ? TStringCast(X::toString() << _T("#MObj(") << RefCount() << _T(')')) : X::toString();
	}

	template<typename X = TObject, typename = void>
	auto _toString(void) const -> decltype(std::enable_if<!Has_toString<X>::value, TString>::type()) {
		return TStringCast(_T('#') << ManagedObj::toString());
	}

public:
	template<typename... Params>
	ManagedObjAdapter(Params&&... xParams) : TObject(std::forward<Params>(xParams)...) {}

	// Disable copy or move construction
	ManagedObjAdapter(_this const &) = delete;
	ManagedObjAdapter(_this &&) = delete;

	// Disable copy or move assignment
	_this& operator=(_this const &) = delete;
	_this& operator=(_this &&) = delete;

	TString toString(void) const override {
		return _toString();
	}

	template<typename X = TObject>
	TString toString(bool Debug) const {
		return _toString(Debug);
	}

	typedef IObjAllocator<_this> TAlloc;

	template<typename... Params>
	static TObject* Create(TAlloc &xAlloc = DefaultObjAllocator<_this>(), Params&&... xParams) {
		return xAlloc.Create(RLAMBDANEW(_this, std::forward<Params>(xParams)...));
	}

	template<
		typename X, typename... Params,
		typename = std::enable_if<!std::is_assignable<X, TAlloc&>::value>::type
	>
		static TObject* Create(X &&xParam, Params&&... xParams) {
		return Create(DefaultObjAllocator<_this>(), xParam, std::forward<Params>(xParams)...);
	}

};

#include "Debug/Exception.h"

template<class T>
static T* Cloneable::Clone(T const *xObj, IObjAllocator<T> &xAlloc) {
	if (auto Ref = Cast(xObj)) return dynamic_cast<T*>(Ref->MakeClone((IObjAllocator<void> &)xAlloc));
	else FAIL(_T("Not a Cloneable derivative"));
}

#endif