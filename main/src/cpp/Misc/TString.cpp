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

// [Utilities] Platform and charset independent string utilities

#include "TString.h"

#include "Debug\Exception.h"
#include "Debug\SysError.h"

#ifdef WINDOWS
#include <Windows.h>
#endif

CString const& EMPTY_CSTRING(void) {
	static CString const __IoFU(EmptyAText);
	return __IoFU;
}

WString const& EMPTY_WSTRING(void) {
	static WString const __IoFU(EmptyWText);
	return __IoFU;
}

TString const& EMPTY_TSTRING(void) {
	static TString const __IoFU(EmptyTText);
	return __IoFU;
}

CString WStringtoCString(unsigned int CodePage, WString const &Str, TString &ErrMessage) {
	ErrMessage.clear();

	if (Str.length() == 0)
		return CString(EMPTY_CSTRING());

#ifdef WINDOWS
	DWORD dwConversionFlags = 0;
#if (WINVER >= 0x0600)
	// Only applicable to UTF-8 and GB18030
	if ((CodePage == CP_UTF8) || (CodePage == 54936))
		dwConversionFlags = WC_ERR_INVALID_CHARS;
#endif
	BOOL IsDefaultCharUsed = FALSE;
	LPBOOL UsedDefaultChar = &IsDefaultCharUsed;
	// Not supported by UTF-7 or UTF-8
	if ((CodePage == CP_UTF7) || (CodePage == CP_UTF8))
		UsedDefaultChar = NULL;

	// Get size of destination UTF-8 buffer, in CHAR's (= bytes)
	int cbCP = WideCharToMultiByte(
		CodePage,				// convert to UTF-8
		dwConversionFlags,		// specify conversion behavior
		Str.data(),				// source UTF-16 string
		(int)Str.length(),		// null terminated string
		NULL,					// unused - no conversion required in this step
		0,						// request buffer size
		NULL,					// use system default char
		UsedDefaultChar			// check if default char is used
		);
	if (cbCP == 0) {
		DWORD ErrCode = GetLastError();
#if (WINVER >= 0x0600)
		// Only applicable to UTF-8 and GB18030
		if (((CodePage == CP_UTF8) || (CodePage == 54936)) && (ErrCode == ERROR_NO_UNICODE_TRANSLATION)) {
			DecodeSysError(ErrCode, ErrMessage);
			// Retry with less strict conversion behavior
			cbCP = WideCharToMultiByte(
				CodePage,				// convert to UTF-8
				0,						// specify conversion behavior
				Str.data(),				// source UTF-16 string
				(int)Str.length(),		// null terminated string
				NULL,					// unused - no conversion required in this step
				0,						// request buffer size
				NULL,					// use system default char
				UsedDefaultChar			// check if default char is used
				);
		}
		if (cbCP == 0)
#endif
			SYSFAIL(_T("Failed to test convert unicode string"));
	}
	if ((UsedDefaultChar != NULL) && (IsDefaultCharUsed)) {
		ErrMessage.assign(_T("Unicode string contains incompatible codepoint for code page #%d, replaced with default char"), CodePage);
	}

	//
	// Allocate destination buffer for UTF-8 string
	//
	CString Ret(cbCP, NullAChar);

	//
	// Do the conversion from UTF-16 to UTF-8
	//
	int result = WideCharToMultiByte(
		CodePage,				// convert to UTF-8
		0,						// specify conversion behavior
		Str.data(),				// source UTF-16 string
		(int)Str.length(),		// null terminated string
		&Ret.front(),			// destination buffer
		cbCP,					// destination buffer size, in bytes
		NULL, NULL				// unused
		);
	if (result == 0)
		SYSFAIL(_T("Failed to convert unicode string"));

	// Return resulting UTF-8 string
	return std::move(Ret);
#endif
}

WString CStringtoWString(UINT CodePage, CString const &Str) {
	if (Str.length() == 0)
		return TString(EMPTY_TSTRING());

#ifdef WINDOWS
	//
	// Get size of destination UTF-16 buffer, in WCHAR's
	//
	int cchUTF16 = MultiByteToWideChar(
		CodePage,				// convert from UTF-8
		MB_ERR_INVALID_CHARS,	// error on invalid chars
		Str.data(),				// source UTF-8 string
		(int)Str.length(),		// total length of source UTF-8 string,
		NULL,					// unused - no conversion done in this step
		0						// request size of destination buffer, in WCHAR's
		);
	if (cchUTF16 == 0)
		SYSFAIL(_T("Unable to test convert codepage #%u string"), CodePage);

	//
	// Allocate destination buffer to store UTF-16 string
	//
	TString Ret(cchUTF16, NullWChar);

	//
	// Do the conversion from UTF-8 to UTF-16
	//
	int result = ::MultiByteToWideChar(
		CodePage,				// convert from UTF-8
		MB_ERR_INVALID_CHARS,	// error on invalid chars
		Str.data(),				// source UTF-8 string
		(int)Str.length(),		// total length of source UTF-8 string,
		&Ret.front(),			// destination buffer
		cchUTF16				// size of destination buffer, in WCHAR's
		);
	if (result == 0)
		SYSFAIL(_T("Failed to convert codepage #%u string"), CodePage);

	// Return resulting UTF-8 string
	return std::move(Ret);
#endif
}

CString WStringtoUTF8(WString const &Str) {
	TString ErrMessage;
	CString Ret = WStringtoCString(CP_UTF8, Str, ErrMessage);
	if (!ErrMessage.empty())
		FAIL(_T("Unsafe conversion from unicode to UTF-8 - %s"), ErrMessage.c_str());
	return std::move(Ret);
}

CString WStringtoUTF8(WString const &Str, TString &ErrMessage) {
	return std::move(WStringtoCString(CP_UTF8, Str, ErrMessage));
}

WString UTF8toWString(CString const &Str) {
	return std::move(CStringtoWString(CP_UTF8, Str));
}

#ifdef WINDOWS

TCHAR const* ACP_LOCALE(void) {
	static TString __IoFU(TStringCast('.' << GetACP()));
	return __IoFU.c_str();
}

#endif