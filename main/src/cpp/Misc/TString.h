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
 * @brief Platform and charset independent string utilities
 * @author Zhenyu Wu
 * @date Jan 05, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_TString_H
#define ZWUtils_TString_H

#include "Global.h"

#ifdef WINDOWS
#include <tchar.h>
#else

#ifdef UNICODE
#define _T(x)	L ## x
#define TCHAR	wchar_t
#else
#define _T(x)	x
#define TCHAR	char
#endif

#endif

#define PTCHAR	TCHAR*
#define PCTCHAR	TCHAR const*

#define NullAChar	'\0'
#define NullWChar	L'\0'
#define NullTChar	_T('\0')

#ifdef WINDOWS
#define ANewLine	"\r\n"
#define WNewLine	L"\r\n"
#define TNewLine	_T("\r\n")
#endif

#ifdef LINUX
#define ANewLine	"\n"
#define WNewLine	L"\n"
#define TNewLine	_T("\n")
#endif

#ifdef MACOS
#define ANewLine	"\r"
#define WNewLine	L"\r"
#define TNewLine	_T("\r")
#endif

#define EmptyAText	""
#define EmptyWText	L""
#define EmptyTText	_T("")

#include <string>
#include <sstream>

typedef std::string CString;
typedef std::stringstream CStringStream;

typedef std::wstring WString;
typedef std::wstringstream WStringStream;

#ifdef UNICODE
typedef WString TString;
typedef WStringStream TStringStream;
#else
typedef CString TString;
typedef CStringStream TStringStream;
#endif

CString const& EMPTY_CSTRING(void);
WString const& EMPTY_WSTRING(void);
TString const& EMPTY_TSTRING(void);

#define CStringCast(exp) dynamic_cast<CStringStream&>(CStringStream() << exp).str()
#define WStringCast(exp) dynamic_cast<WStringStream&>(WStringStream() << exp).str()
#define TStringCast(exp) dynamic_cast<TStringStream&>(TStringStream() << exp).str()

//! @ingroup Utilities
//! Convert wide string to a given code page, with soft-fault tolorance
CString WStringtoCString(unsigned int CodePage, WString const &Str, WString &ErrMessage);

//! @ingroup Utilities
//! Convert string of a given code page to wide string
WString CStringtoWString(unsigned int CodePage, CString const &Str);

CString WStringtoUTF8(WString const &Str);
CString WStringtoUTF8(WString const &Str, WString &ErrMessage);
WString UTF8toWString(CString const &Str);

#ifdef WINDOWS
TCHAR const* ACP_LOCALE(void);
#endif

#endif