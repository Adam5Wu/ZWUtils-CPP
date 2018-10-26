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
 * @brief Logging support
 * @author Zhenyu Wu
 * @date Jan 06, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_Logging_H
#define ZWUtils_Logging_H

 // Project global control 
#include "Misc/Global.h"

#include "Misc/TString.h"

#include "Debug.h"

extern TString const LOGTARGET_CONSOLE;

//! Add or remove a debug log target
void SETLOGTARGET(TString const &Name, FILE *xTarget);
FILE * GETLOGTARGET(TString const &Name);

//! Print a formatted debug string message to a log target
void __LOG_DO(TString const* Target, PCTCHAR Fmt, ...);

#define __LOG(...)		__LOG_DO(nullptr, __VA_ARGS__)
#define __TLOG(t, ...)	__LOG_DO(t, __VA_ARGS__)

//-------------- LOGGING

#ifndef NLOG
#define LOG_DO(x)	x
#else
#define LOG_DO(x)	;
#endif

#define _LOG(fmt, ...)												\
LOG_DO({															\
	TString __TS = __TimeStamp();									\
	__LOG(_T("[%s] %s | ") fmt TNewLine,							\
		__PTID(), __TS.c_str() __VAWRAP(__VA_ARGS__));				\
})

#define _LOGS(fmt, ...)												\
LOG_DO({															\
	SOURCEMARK														\
	TString __TS = __TimeStamp();									\
	__LOG(_T("@<%s>") TNewLine _T("[%s] %s | ") fmt TNewLine,		\
		__SrcMark.c_str(), __PTID(), __TS.c_str()					\
		__VAWRAP(__VA_ARGS__));										\
})

#define _TLOG(t,fmt, ...)											\
LOG_DO({															\
	TString __TS = __TimeStamp();									\
	__TLOG(t, _T("[%s] %s | ") fmt TNewLine,						\
		__PTID(), __TS.c_str() __VAWRAP(__VA_ARGS__));				\
})

#define _TLOGS(t,fmt, ...)											\
LOG_DO({															\
	SOURCEMARK														\
	TString __TS = __TimeStamp();									\
	__TLOG(t, _T("@<%s>") TNewLine _T("[%s] %s | ") fmt TNewLine,	\
		__SrcMark.c_str(), __PTID(), __TS.c_str()					\
		__VAWRAP(__VA_ARGS__));										\
})

#ifndef __LOGPFX__
#define __LOGPFX__
#endif

//-------------- DEBUG LOGGING

#define LOG(s, ...)		DEBUG_DO(_LOG(__LOGPFX__ s, __VA_ARGS__))
#define LOGS(s, ...)	DEBUG_DO(_LOGS(__LOGPFX__ s, __VA_ARGS__))
#define LOGV(s, ...)	DEBUGV_DO(_LOG(__LOGPFX__ s, __VA_ARGS__))
#define LOGSV(s, ...)	DEBUGV_DO(_LOGS(__LOGPFX__ s, __VA_ARGS__))
#define LOGVV(s, ...)	DEBUGVV_DO(_LOG(__LOGPFX__ s, __VA_ARGS__))
#define LOGSVV(s, ...)	DEBUGVV_DO(_LOGS(__LOGPFX__ s, __VA_ARGS__))

#define TLOG(t,s, ...)		DEBUG_DO(_TLOG(t, __LOGPFX__ s, __VA_ARGS__))
#define TLOGS(t,s, ...)		DEBUG_DO(_TLOGS(t, __LOGPFX__ s, __VA_ARGS__))
#define TLOGV(t,s, ...)		DEBUGV_DO(_TLOG(t, __LOGPFX__ s, __VA_ARGS__))
#define TLOGSV(t,s, ...)	DEBUGV_DO(_TLOGS(t, __LOGPFX__ s, __VA_ARGS__))
#define TLOGVV(t,s, ...)	DEBUGVV_DO(_TLOG(t, __LOGPFX__ s, __VA_ARGS__))
#define TLOGSVV(t,s, ...)	DEBUGVV_DO(_TLOGS(t, __LOGPFX__ s, __VA_ARGS__))

#ifdef DEFAULT_LOG_WITH_SOURCE
#undef LOG
#define LOG LOGS
#undef LOGV
#define LOGV LOGSV
#undef LOGVV
#define LOGVV LOGSVV

#undef TLOG
#define TLOG TLOGS
#undef TLOGV
#define TLOGV TLOGSV
#undef TLOGVV
#define TLOGVV TLOGSVV
#endif

#endif