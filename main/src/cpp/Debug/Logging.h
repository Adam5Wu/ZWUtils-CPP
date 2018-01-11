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

TString const& LOGTARGET_CONSOLE(void);

//! Add or remove a debug log target
void SETLOGTARGET(TString const &Name, FILE *xTarget, PCTCHAR Message = nullptr);
FILE * GETLOGTARGET(TString const &Name);

//! Print a formatted debug string message to a log target
void __LOG_DO(TString const* Target, PCTCHAR Fmt, ...);

#define __LOG(...)	__LOG_DO(nullptr VAWRAP(__VA_ARGS__))

//-------------- LOGGING

#ifndef NLOG
#define LOG_DO(x)	x
#else
#define LOG_DO(x)	;
#endif

#define _LOG(fmt, ...)																	\
LOG_DO({																				\
	TString __TS = __TimeStamp();														\
	__LOG(_T("[%s] %s | ") fmt TNewLine, __PTID(), __TS.c_str() VAWRAP(__VA_ARGS__));	\
})

#define _LOGS(fmt, ...)												\
LOG_DO({															\
	SOURCEMARK														\
	_LOG(_T("@<%s> ") fmt, __SrcMark.c_str() VAWRAP(__VA_ARGS__));	\
})

#define __LOGPFX__

//-------------- DEBUG LOGGING

#define LOG(s, ...)		DEBUG_DO(_LOG(__LOGPFX__ s, __VA_ARGS__))
#define LOGS(s, ...)	DEBUG_DO(_LOGS(__LOGPFX__ s, __VA_ARGS__))
#define LOGV(s, ...)	DEBUGV_DO(_LOG(__LOGPFX__ s, __VA_ARGS__))
#define LOGSV(s, ...)	DEBUGV_DO(_LOGS(__LOGPFX__ s, __VA_ARGS__))
#define LOGVV(s, ...)	DEBUGVV_DO(_LOG(__LOGPFX__ s, __VA_ARGS__))
#define LOGSVV(s, ...)	DEBUGVV_DO(_LOGS(__LOGPFX__ s, __VA_ARGS__))

#endif