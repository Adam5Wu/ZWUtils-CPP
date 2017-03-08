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
 * @brief System error diagnostics
 * @author Zhenyu Wu
 * @date Jan 05, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_SysError_H
#define ZWUtils_SysError_H

#include "Misc/Global.h"
#include "Misc/TString.h"

#include "Memory/ObjAllocator.h"

#include "Debug.h"
#include "Logging.h"
#include "Exception.h"

#ifdef WINDOWS
#include <Windows.h>
#endif

//! @ingroup Utilities
//! Release the allocate a buffer for error message
void FreeSysErrorMessage(PCTCHAR MsgBuf);

//! @ingroup Utilities
//! Decode the error code in GetLastError and allocate a buffer for the message (free with FreeErrorMessage)
PCTCHAR DecodeLastSysError(va_list *pArgs = nullptr);

//! @ingroup Utilities
//! Decode the error code in GetLastError using a pre-allocated message buffer
void DecodeLastSysError(PTCHAR Buffer, size_t &BufLen, va_list *pArgs = nullptr);

//! @ingroup Utilities
//! Decode the error code in GetLastError and allocate a string buffer for the message
void DecodeLastSysError(TString &StrBuf, va_list *pArgs = nullptr);

//! @ingroup Utilities
//! Decode the specified system error code and allocate a buffer for the message (free with FreeErrorMessage)
PCTCHAR DecodeSysError(unsigned int ErrCode, va_list *pArgs = nullptr);

//! @ingroup Utilities
//! Decode the specified system error code using a pre-allocated message buffer
void DecodeSysError(unsigned int ErrCode, PTCHAR Buffer, size_t &BufLen, va_list *pArgs = nullptr);

//! @ingroup Utilities
//! Decode the specified system error code and allocate a string buffer for the message
void DecodeSysError(unsigned int ErrCode, TString &StrBuf, va_list *pArgs = nullptr);

#ifdef WINDOWS

//! @ingroup Utilities
//! Decode the specified module error code and allocate a buffer for the message (free with LocalFree)
PCTCHAR DecodeSysError(HMODULE Module, unsigned int ErrCode, va_list *pArgs = nullptr);

//! @ingroup Utilities
//! Decode the specified module error code using a pre-allocated message buffer
void DecodeSysError(HMODULE Module, unsigned int ErrCode, PTCHAR Buffer, size_t &BufLen, va_list *pArgs = nullptr);

//! @ingroup Utilities
//! Decode the specified module error code using a pre-allocated message buffer
void DecodeSysError(HMODULE Module, unsigned int ErrCode, TString &StrBuf, va_list *pArgs = nullptr);

#endif

#define SYSERRMSG_STACK(errcode, ...)								\
TCHAR __SysErrMsg[ERRMSG_BUFLEN];									\
{																	\
	size_t __BufLen = ERRMSG_BUFLEN;								\
	DecodeSysError(errcode, __SysErrMsg, __BufLen,  __VA_ARGS__);	\
}

#define SYSERRMSG_HEAP(errcode, ...)				\
TString __SysErrMsg;								\
DecodeSysError(errcode, __SysErrMsg, __VA_ARGS__);

#ifdef WINDOWS

void __ModuleFormatCtxAndDecodeSysError(HMODULE Module, unsigned int ErrCode,
										PTCHAR CtxBuffer, size_t CtxBufLen, PCTCHAR CtxBufFmt,
										PTCHAR ErrBuffer, size_t &ErrBufLen, ...);

#define __FormatCtxAndDecodeSysError(ErrCode, CtxBuffer, CtxBufLen, CtxBufFmt, ErrBuffer, ErrBufLen, ...) \
	__ModuleFormatCtxAndDecodeSysError(0, ErrCode, CtxBuffer, CtxBufLen, CtxBufFmt, ErrBuffer, ErrBufLen, __VA_ARGS__)

#else

void __FormatCtxAndDecodeSysError(unsigned int ErrCode, PTCHAR CtxBuffer, size_t CtxBufLen, PCTCHAR CtxBufFmt,
								  PTCHAR ErrBuffer, size_t &ErrBufLen, ...);

#endif

//! @ingroup Utilities
//! Log a failure with a system error and context
#define ERRLOG(errcode, ctx, ...) {													\
	SYSERRMSG_STACK(errcode);														\
	LOG(_T("Error %0.8X: %s (") ctx _T(")"), errcode, __SysErrMsg, __VA_ARGS__);	\
}

#define ERRLOGS(errcode, ctx, ...) {												\
	SYSERRMSG_STACK(errcode);														\
	LOGS(_T("Error %0.8X: %s (") ctx _T(")"), errcode, __SysErrMsg, __VA_ARGS__);	\
}

//! @ingroup Utilities
//! Log a failure with an argumented system error and context
#define ERRLOGA(errcode, ctx, ...) {									\
	TCHAR __CtxErrMsg[ERRMSG_BUFLEN];									\
	TCHAR __SysErrMsg[ERRMSG_BUFLEN];									\
	__FormatCtxAndDecodeError(errcode, __CtxErrMsg, ERRMSG_BUFLEN, ctx, \
							  __SysErrMsg, ERRMSG_BUFLEN, __VA_ARGS__);	\
	LOG(_T("Error %0.8X: %s (%s)"), errcode, __SysErrMsg, __CtxErrMsg);	\
}

#define ERRLOGSA(errcode, ctx, ...) {										\
	TCHAR __CtxErrMsg[ERRMSG_BUFLEN];										\
	TCHAR __SysErrMsg[ERRMSG_BUFLEN];										\
	__FormatCtxAndDecodeError(errcode, __CtxErrMsg, ERRMSG_BUFLEN, ctx,		\
							  __SysErrMsg, ERRMSG_BUFLEN, __VA_ARGS__);		\
	LOGS(_T("Error %0.8X: %s (%s)"), errcode, __SysErrMsg, __CtxErrMsg);	\
}

#ifdef WINDOWS

//! @ingroup Utilities
//! Log a failed system call
#define SYSERRLOG(ctx, ...) {			\
	DWORD ErrCode = GetLastError();		\
	ERRLOG(ErrCode, ctx, __VA_ARGS__);	\
	SetLastError(ErrCode);				\
}

#define SYSERRLOGA(ctx, ...) {			\
	DWORD ErrCode = GetLastError();		\
	ERRLOGA(ErrCode, ctx, __VA_ARGS__);	\
	SetLastError(ErrCode);				\
}

#define SYSERRLOGS(ctx, ...) {			\
	DWORD ErrCode = GetLastError();		\
	ERRLOGS(ErrCode, ctx, __VA_ARGS__);	\
	SetLastError(ErrCode);				\
}

#define SYSERRLOGSA(ctx, ...) {				\
	DWORD ErrCode = GetLastError();			\
	ERRLOGSA(ErrCode, ctx, __VA_ARGS__);	\
	SetLastError(ErrCode);					\
}

#endif

/**
 * @ingroup Utilities
 * @brief System error exception class
 *
 * Contains useful information about the system error: ErrorCode, ErrorMessage
 **/
class SystemError : public Exception {
	typedef SystemError _this;
	friend class IObjAllocator < _this > ;

protected:
	TString const rErrorMsg = EmptyWText;

	template<typename... Params>
	SystemError(unsigned int xErrorCode, TString &&xSource, PCTCHAR ReasonFmt, Params&&... xParams) :
		Exception(std::move(xSource), ReasonFmt, xParams...), ErrorCode(xErrorCode) {}

public:
	unsigned int const ErrorCode = 0;

	virtual TString const& ErrorMessage(void) const;
	TString const& Why(void) const override;

	template<typename... Params>
	static SystemError* Create(unsigned int xErrorCode, TString &&xSource, PCTCHAR ReasonFmt, Params&&... xParams) {
		return DefaultObjAllocator<SystemError>().Create(
			RLAMBDANEW(SystemError, xErrorCode, std::move(xSource), ReasonFmt, xParams...));
	}
};

//! @ingroup Utilities
//! Raise a system error exception with a formatted string message
#define SYSERRFAILS(src, errcode, fmt, ...)	throw SystemError::Create(src, errcode, fmt VAWRAP(__VA_ARGS__));
#define SYSERRFAIL(errcode, fmt, ...) {								\
	SOURCEMARK														\
	SYSERRFAILS(errcode, std::move(__SrcMark), fmt, __VA_ARGS__);	\
}

#ifdef WINDOWS
#define SYSFAIL(ctx, ...)	SYSERRFAIL(GetLastError(), ctx, __VA_ARGS__)
#endif

#endif
