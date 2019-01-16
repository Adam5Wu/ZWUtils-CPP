/*
Copyright (c) 2005 - 2019, Zhenyu Wu; 2012 - 2019, NEC Labs America Inc.
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
 * @addtogroup System System Level Function Interfaces
 * @file
 * @brief Basic Types
 * @author Zhenyu Wu
 * @date Jan 15, 2019: Refactored from Memory/Resource.h
 **/

#ifndef ZWUtils_SysTypes_H
#define ZWUtils_SysTypes_H

 // Project global control
#include "Misc/Global.h"

#include "Memory/Resource.h"

#include <Windows.h>

#ifdef WINDOWS

template<typename H>
class TGenericHandle {
public:
	static H ValidateHandle(H const &Ref);
	static void HandleDealloc_Standard(H &Res);
	static void HandleDealloc_BestEffort(H &Res);
};

class THandle : public TAllocResource<HANDLE>, public TGenericHandle<HANDLE> {
	typedef THandle _this;

public:
	THandle(void) : THandle(Unmanaged(INVALID_HANDLE_VALUE)) {}
	THandle(TResAlloc const &xAlloc, TResDealloc const &xDealloc = HandleDealloc_Standard) :
		TAllocResource(xAlloc, xDealloc) {}
	THandle(CONSTRUCTION::HANDOFF_T const&, HANDLE const &xResRef, TResDealloc const &xDealloc = HandleDealloc_Standard, TResAlloc const &xAlloc = NoAlloc) :
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

	static _this Unmanaged(HANDLE const &xHandle)
	{ return _this(CONSTRUCTION::VALIDATED, xHandle, NullDealloc); }
};

class TModule : public TAllocResource<HMODULE> {
	typedef TModule _this;

public:
	static HMODULE ValidateHandle(HMODULE const &Ref);
	static void HandleDealloc_Standard(HMODULE &Res);
	static void HandleDealloc_BestEffort(HMODULE &Res);

	TModule(void) : TModule(Unmanaged(NULL)) {}
	TModule(TResAlloc const &xAlloc, TResDealloc const &xDealloc = HandleDealloc_Standard) :
		TAllocResource(xAlloc, xDealloc) {}
	TModule(CONSTRUCTION::HANDOFF_T const&, HMODULE const &xResRef, TResDealloc const &xDealloc = HandleDealloc_Standard, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(ValidateHandle(xResRef), xDealloc, xAlloc) {}
	TModule(CONSTRUCTION::VALIDATED_T const&, HMODULE const &xResRef, TResDealloc const &xDealloc = HandleDealloc_Standard, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(xResRef, xDealloc, xAlloc) {}

	static _this Unmanaged(HMODULE const &xModule)
	{ return _this(CONSTRUCTION::VALIDATED, xModule, NullDealloc); }

	static TModule GetLoaded(TString const &Name)
	{ return Unmanaged(GetModuleHandle(Name.c_str())); }
};

#include "Debug/SysError.h"

template<typename H>
H TGenericHandle<H>::ValidateHandle(H const &Ref) {
	if (Ref == INVALID_HANDLE_VALUE)
		FAIL(_T("Cannot assign invalid handle"));
	return Ref;
}

template<typename H>
void TGenericHandle<H>::HandleDealloc_Standard(H &Res) {
	if (!CloseHandle(Res))
		SYSFAIL(_T("Failed to release handle"));
}

template<typename H>
void TGenericHandle<H>::HandleDealloc_BestEffort(H &Res) {
	if (!CloseHandle(Res))
		SYSERRLOG(_T("Failed to release handle"));
}


#endif //WINDOWS

#endif //ZWUtils_SysTypes_H