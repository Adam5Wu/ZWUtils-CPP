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
 * @brief Basic debug support
 * @author Zhenyu Wu
 * @date Jan 06, 2015: Refactored from legacy ZWUtils
 * @date Feb 12, 2015: Refactored from Logging
 * @date Jan 26, 2016: Initial Public Release
 **/

#ifndef ZWUtils_Debug_H
#define ZWUtils_Debug_H

#include "Misc/Global.h"
#include "Misc/Types.h"
#include "Misc/TString.h"
#include "Misc/Timing.h"

#ifdef WINDOWS
#include <Windows.h>
#endif

#define NDEBUG_DO(x)		x
#define NDEBUG_DO_OR(x,y)	x
#define DEBUG_DO(x)			x
#define DEBUG_DO_OR(x,y)	x
#define DEBUGV_DO(x)		DEBUG_DO(x)
#define DEBUGV_DO_OR(x,y)	x
#define DEBUGVV_DO(x)		DEBUGV_DO(x)
#define DEBUGVV_DO_OR(x,y)	x

#ifdef NDEBUG
#undef DEBUG_DO
#define DEBUG_DO(x)			;
#undef DEBUG_DO_OR
#define DEBUG_DO_OR(x,y)	y
#undef DEBUGV_DO_OR
#define DEBUGV_DO_OR(x,y)	y
#undef DEBUGVV_DO_OR
#define DEBUGVV_DO_OR(x,y)	y
#else
#undef NDEBUG_DO
#define NDEBUG_DO(x)		;
#undef NDEBUG_DO_OR
#define NDEBUG_DO_OR(x,y)	y
#endif

#ifndef DBGV
#undef DEBUGV_DO
#define DEBUGV_DO(x)		;
#undef DEBUGV_DO_OR
#define DEBUGV_DO_OR(x,y)	y
#undef DEBUGVV_DO_OR
#define DEBUGVV_DO_OR(x,y)	y
#endif

#ifndef DBGVV
#undef DEBUGVV_DO
#define DEBUGVV_DO(s)		;
#undef DEBUGVV_DO_OR
#define DEBUGVV_DO_OR(x,y)	y
#endif

#define ERRMSG_BUFLEN	1024

#ifdef WINDOWS

#define BUFFMT(buf,len,fmt,...) 										\
{ENFORCE_TYPE(decltype(buf), PTCHAR);}									\
int __Len = _sntprintf_s(buf, len, _TRUNCATE, fmt VAWRAP(__VA_ARGS__));	\
if (__Len >= 0) (buf)[__Len] = NullTChar;								\

#endif

#define STACK_MSGFMT(len,fmt,...)			\
TCHAR __ErrorMsg[len];						\
{BUFFMT(__ErrorMsg, len, fmt, __VA_ARGS__)}

#define STACK_ERRMSGFMT(fmt,...)	STACK_MSGFMT(ERRMSG_BUFLEN, fmt, __VA_ARGS__)

#define STR_MSGFMT(pstr,len,fmt,...) { 					\
ENFORCE_TYPE(decltype(pstr), TString*);					\
(pstr)->resize(len);									\
BUFFMT((PTCHAR)(pstr)->data(), len, fmt, __VA_ARGS__);	\
(pstr)->resize(__Len >= 0 ? __Len : 0);					\
}

#define STR_ERRMSGFMT(pstr,fmt,...)	STR_MSGFMT(pstr, ERRMSG_BUFLEN, fmt,__VA_ARGS__)

#define HEAPSTR_ERRMSGFMT(fmt,...)				\
TString __ErrorMsg;								\
STR_ERRMSGFMT(&__ErrorMsg, fmt, __VA_ARGS__)

PCTCHAR __RelPath(PCTCHAR Path);

//! Get the relative path of source file
#define __REL_FILE__	__RelPath(_T(__FILE__))

//! Allocate buffer and print current source information
#define SOURCEMARK														\
TString __SrcMark;														\
{																		\
	PCTCHAR __RelFile = __REL_FILE__;									\
	/* '(' + LineNum + ')' + \0 */										\
	size_t __Size = _tcslen(__RelFile) + 10 + 1 + 1 + 1;				\
	STR_MSGFMT(&__SrcMark, __Size, _T("%s(%d)"), __RelFile, __LINE__);	\
}

inline TString __TimeStamp(void) { return TimeStamp::Now().toString(); }
PCTCHAR __PTID(void);

#endif