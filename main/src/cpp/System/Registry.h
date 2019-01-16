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
 * @brief Registry access
 * @author Zhenyu Wu
 * @date Jan 15, 2019: Extracted from application projects
 **/

#ifndef ZWUtils_SysReg_H
#define ZWUtils_SysReg_H

 // Project global control
#include "Misc/Global.h"

#include "Misc/TString.h"

#include "SysTypes.h"

#ifdef WINDOWS

#include <Windows.h>

class TRegistry : public TAllocResource<HKEY>, public TGenericHandle<HKEY> {
public:
	TRegistry(TRegistry const &Base, TString const &Name, REGSAM samDesired = KEY_ALL_ACCESS, bool createIfNeeded = false);

	TRegistry(CONSTRUCTION::HANDOFF_T const &, HKEY const &hKey, TResDealloc const &xDealloc = HandleDealloc_Standard, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(hKey, xDealloc, xAlloc) {}

	INT32 GetInt32(TString const &Name, INT32 Default);
	INT64 GetInt64(TString const &Name, INT64 Default);
	TString GetString(TString const &Name, TString const &Default);

	void SetInt32(TString const &Name, INT32 Value);
	void SetInt64(TString const &Name, INT64 Value);
	void SetString(TString const &Name, TString const &Value);

	static TRegistry Unmanaged(HKEY const &hKey)
	{ return { CONSTRUCTION::HANDOFF, hKey, NullDealloc }; }
};

#endif //WINDOWS

#endif //ZWUtils_SysReg_H