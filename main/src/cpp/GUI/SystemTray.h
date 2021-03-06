/*
Copyright (c) 2005 - 2018, Zhenyu Wu; 2012 - 2018, NEC Labs America Inc.
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
 * @addtogroup GUI Graphic User Interface Utilities
 * @file
 * @brief System Tray Helper
 * @author Zhenyu Wu
 * @date Nov 09, 2018: Initial Implementation
 **/

#ifndef ZWUtils_GUISystemTray_H
#define ZWUtils_GUISystemTray_H

 // Project global control
#include "Misc/Global.h"

#ifdef WINDOWS

#include "Misc/TString.h"

#include "Threading/WorkerThread.h"

#include "GUITypes.h"

#include <functional>
#include <Windows.h>

typedef std::function<void(WPARAM State)> FWinStationStateNotify;
MRWorkerThread CreateSystemTray(TString const &Name, TIcon &&Icon, TString const &ToolTip,
								TMenuItems && MenuItems, FWinStationStateNotify const &WTSStateNotify);

void SystemTray_GlobalInit(HINSTANCE hInstance);
void SystemTray_GlobalFInit(void);

#endif

#endif //ZWUtils_GUISystemTray_H