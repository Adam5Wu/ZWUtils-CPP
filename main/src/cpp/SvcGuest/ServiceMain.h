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
 * @addtogroup AdvUtils Advanced Supporting Utilities
 * @file
 * @brief Generic System Service Interface
 * @author Zhenyu Wu
 * @date Sep 26, 2018: Refactored business logic independent code from AgentLib-NG
 **/

#ifndef ZWUtils_ServiceMain_H
#define ZWUtils_ServiceMain_H

#include "Misc/Global.h"

#include "Misc/Types.h"
#include "Misc/TString.h"
#include "Threading/SyncElements.h"

#include <functional>

extern "C" {

	extern PCTCHAR const SERVICE_NAME;
	extern PCTCHAR const SERVICE_DISPNAME;
	extern PCTCHAR const SERVICE_DESC;
	extern PCTCHAR const SERVICE_GROUP;
	extern PCTCHAR const SERVICE_DEPENDS;
	extern PCTCHAR const SERVICE_USER;
	extern PCTCHAR const SERVICE_PRIVILEGES;
	extern PCTCHAR const SERVICE_PROGRAMPATH;

	extern int CONFIG_ServiceStatusQueryInterval;
	extern int CONFIG_ServiceTerminationGraceTime;

	extern void ServiceMain_LoadModules(void);
	extern void ServiceMain_UnloadModules(void);

}

typedef std::function<void(void)> ServiceEvent;
void Module_ServiceMain_AddModule(TString const &Name,
								  ServiceEvent const &Start,
								  ServiceEvent const &Stop,
								  ServiceEvent const &Status);

UINT32 ServiceMain_DebugRun(UINT32 dwArgc, PCTCHAR *lpszArgv);

void ServiceMain_ServiceStopNotify(void);
THandleWaitable &ServiceMain_TermSignal(void);
TString Service_GetDataDir(void);

#endif //ZWUtils_ServiceMain_H
