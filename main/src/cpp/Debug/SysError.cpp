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

// [Utilities] System error diagnostics

#include "SysError.h"

#include "Memory/Resource.h"

#include <stdarg.h>

#ifdef WINDOWS

void __RLNTrim(PTCHAR Buf, size_t &Len) {
	while (Len) {
		TCHAR CVal = Buf[Len - 1];
		if ((CVal != _T('\n')) && (CVal != _T('\r'))) {
			break;
		}
		Buf[--Len] = NullTChar;
	}
}

PCTCHAR _DecodeSysError(HMODULE Module, unsigned int ErrCode, PTCHAR Buffer, size_t &BufLen, va_list *pArgs) {
	DWORD MsgFlags = 0;
	if (pArgs == nullptr)
		MsgFlags |= FORMAT_MESSAGE_IGNORE_INSERTS;
	if (Module == 0)
		MsgFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
	else
		MsgFlags |= FORMAT_MESSAGE_FROM_HMODULE;

	PTCHAR MsgBuf;
	if (Buffer == nullptr) {
		MsgBuf = (PTCHAR)&MsgBuf;
		MsgFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER;
	} else
		MsgBuf = Buffer;

	BufLen = FormatMessage(MsgFlags, Module, ErrCode, 0, MsgBuf, (DWORD)BufLen, pArgs);
	if (!BufLen) SYSFAIL(_T("Unable to decode error code %0.8X"), ErrCode);

	__RLNTrim(MsgBuf, BufLen);
	return MsgBuf;
}

void FreeSysErrorMessage(PCTCHAR MsgBuf) {
	if (LocalFree((HLOCAL)MsgBuf) != nullptr)
		SYSFAIL(_T("Unable to free error message buffer"));
}

PCTCHAR DecodeLastSysError(va_list *pArgs) {
	return DecodeSysError(GetLastError(), pArgs);
}

void DecodeLastSysError(PTCHAR Buffer, size_t &BufLen, va_list *pArgs) {
	DecodeSysError(GetLastError(), Buffer, BufLen, pArgs);
}

void DecodeLastSysError(TString &StrBuf, va_list *pArgs) {
	unsigned int ErrCode = GetLastError();
	DecodeSysError(ErrCode, StrBuf, pArgs);
}

PCTCHAR DecodeSysError(unsigned int ErrCode, va_list *pArgs) {
	return DecodeSysError(0, ErrCode, pArgs);
}

PCTCHAR DecodeSysError(HMODULE Module, unsigned int ErrCode, va_list *pArgs) {
	size_t __BufLen = 0;
	return _DecodeSysError(Module, ErrCode, nullptr, __BufLen, pArgs);
}

void DecodeSysError(unsigned int ErrCode, PTCHAR Buffer, size_t &BufLen, va_list *pArgs) {
	DecodeSysError(0, ErrCode, Buffer, BufLen, pArgs);
}

void DecodeSysError(unsigned int ErrCode, TString &StrBuf, va_list *pArgs) {
	DecodeSysError(0, ErrCode, StrBuf, pArgs);
}

void DecodeSysError(HMODULE Module, unsigned int ErrCode, PTCHAR Buffer, size_t &BufLen, va_list *pArgs) {
	_DecodeSysError(Module, ErrCode, Buffer, BufLen, pArgs);
}

void DecodeSysError(HMODULE Module, unsigned int ErrCode, TString &StrBuf, va_list *pArgs) {
	StrBuf.resize(ERRMSG_BUFLEN);
	size_t __BufLen = ERRMSG_BUFLEN;
	_DecodeSysError(Module, ErrCode, (PTCHAR)StrBuf.data(), __BufLen, pArgs);
	StrBuf.resize(__BufLen);
}

void __ModuleFormatCtxAndDecodeSysError(HMODULE Module, unsigned int ErrCode,
	PTCHAR CtxBuffer, size_t CtxBufLen, PCTCHAR CtxBufFmt,
	PTCHAR ErrBuffer, size_t &ErrBufLen, ...) {
	va_list params;
	va_start(params, ErrCode);
	TInitResource<va_list> Params(params, [](va_list &X) {va_end(X); });
	int __Len = _vsntprintf_s(CtxBuffer, CtxBufLen, _TRUNCATE, CtxBufFmt, params);
	if (__Len >= 0) CtxBuffer[__Len] = NullTChar;
	DecodeSysError(Module, ErrCode, ErrBuffer, ErrBufLen, &params);
}

#define FSystemErrorMessage _T("%s (Error %0.8X: %s)")

TString const& SystemError::ErrorMessage(void) const {
	if (rErrorMsg.empty())
		DecodeSysError(ErrorCode, rErrorMsg);
	return rErrorMsg;
}

TString const& SystemError::Why(void) const {
	if (rWhy.empty()) {
		HEAPSTR_ERRMSGFMT(FSystemErrorMessage, Exception::Why().c_str(), ErrorCode, ErrorMessage().c_str());
		rWhy.assign(std::move(__ErrorMsg));
	}
	return rWhy;
}

#endif