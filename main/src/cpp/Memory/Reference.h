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
 * @brief Reference Template
 * @author Zhenyu Wu
 * @date Jul 29, 2015: Refactored from ManagedRef
 **/

#ifndef ZWUtils_Reference_H
#define ZWUtils_Reference_H

 // Project global control
#include "Misc/Global.h"

#include "Debug/Exception.h"

template<class T>
class Reference {
	typedef Reference _this;

protected:
	Reference() {}

	// Note: For performance reasons, we do not have a virtual destructor
	// Hence we prevent arbitrary destruction of this instance
	~Reference(void) {}

	virtual T* _ObjPointer(void) const {
		FAIL(_T("Abstract Function"));
	}

	virtual T* _ObjExchange(T *xObj) {
		FAIL(_T("Abstract Function"));
	}

public:
	// Disable copy and move constructions for generial references
	Reference(_this const &) = delete;
	Reference(_this &&) = delete;

	// Assignment operations are also disabled
	_this& operator=(_this const &) = delete;
	_this& operator=(_this &&) = delete;

	_this* operator~(void) {
		return this;
	}
	_this const* operator~(void) const {
		return this;
	}

	T* operator&(void) const {
		return _ObjPointer();
	}
	T& operator*(void) const {
		return *_ObjPointer();
	}
	T* operator->(void) const {
		return _ObjPointer();
	}
	bool operator==(_this const &xRef) const {
		return **this == *xRef;
	}

	virtual T* Assign(T *xObj) {
		return _ObjExchange(xObj);
	}

	virtual T* Drop(void) {
		return _ObjExchange(nullptr);
	}

	virtual T* operator=(T *xObj) {
		return Assign(xObj), xObj;
	}

	virtual bool Empty(void) const {
		return _ObjPointer() == nullptr;
	}

	virtual void Clear(void) {
		Assign(nullptr);
	}
};

#endif