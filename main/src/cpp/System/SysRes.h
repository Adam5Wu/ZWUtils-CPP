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
 * @brief Resource resolution
 * @author Zhenyu Wu
 * @date Jan 15, 2019: Extracted from application projects
 **/

#ifndef ZWUtils_SysRes_H
#define ZWUtils_SysRes_H

 // Project global control
#include "Misc/Global.h"

#include "Memory/Resource.h"

#include "SysTypes.h"

#ifdef WINDOWS

#include <Windows.h>

struct LANGANDCODEPAGE {
	WORD wLanguage;
	WORD wCodePage;
};

#define VERSIONSTRING_COMPANYNAME	"CompanyName"
#define VERSIONSTRING_FILEDESC		"FileDescription"
#define VERSIONSTRING_FILEVER		"FileVersion"
#define VERSIONSTRING_INTNAME		"InternalName"
#define VERSIONSTRING_LEAGALCPR		"LegalCopyright"
#define VERSIONSTRING_ORGFILENAME	"OriginalFilename"
#define VERSIONSTRING_PRODUCTNAME	"ProductName"
#define VERSIONSTRING_PRODUCTVER	"ProductVersion"

TModule LoadResourceModule(TString const &ModuleName);

TDynBuffer GetVersionResource(TString const &ResKey, TString const &ModuleName);

TDynBuffer LoadVersionResource(TModule const &Module);
TDynBuffer GetVersionResource(TString const &ResKey, TModule const &Module);

TDynBuffer GetVersionResource(TString const &ResKey, TDynBuffer const &VerResData);

TTypedBuffer<LANGANDCODEPAGE> GetVersionTranslations(TDynBuffer const &ResData);

TString ExtractVersionString(TDynBuffer const &ResData, LANGANDCODEPAGE const& LCData, TString const &ResKey);

#endif //WINDOWS

#endif //ZWUtils_SysRes_H