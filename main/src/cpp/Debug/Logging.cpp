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

// [Utilities] Logging support

#include "Logging.h"

#include "Memory/Resource.h"

#include "Threading/SyncObjects.h"

#ifdef WINDOWS
#include <Windows.h>
#endif

#include <unordered_map>

TString const& LOGTARGET_CONSOLE(void) {
	static TString const __IoFU(_T("Console"));
	return __IoFU;
}

typedef std::vector<std::pair<TString, FILE*>> TLogTargets;

TSyncObj<TLogTargets>& LOGTARGETS(void) {
	static TSyncObj<TLogTargets> __IoFU(TLogTargets({{LOGTARGET_CONSOLE(), stderr}}));
	return __IoFU;
}

void SETLOGTARGET(TString const &Name, FILE *xTarget, PCTCHAR Message) {
	auto LogTargets(LOGTARGETS().Pickup());
	if (xTarget) {
		if (setvbuf(xTarget, nullptr, _IOLBF, 4096) != 0) {
			LOG(_T("WARNING: Unable to turn off file buffering for log target '%s'"), Name);
		}

		for (auto entry : *LogTargets) {
			if (entry.first.compare(Name) == 0) {
				entry.second = xTarget;
				xTarget = nullptr;
				break;
			}
		}
		if (xTarget) LogTargets->emplace_back(Name, xTarget);
		if (Message) { LOG(_T("===== %s ====="), Message); }
	} else {
		auto Iter = LogTargets->begin();
		while (Iter != LogTargets->end()) {
			if (Iter->first.compare(Name) == 0) {
				if (Message) { LOG(_T("===== %s ====="), Message); }
				LogTargets->erase(Iter);
				break;
			} else Iter++;
		}
	}
}

#ifdef WINDOWS

void __LOG(PCTCHAR Fmt, ...) {
	va_list params;
	va_start(params, Fmt);
	TInitResource<va_list> Params(params, [&](va_list &X) {va_end(X); });
	auto LogTargets(LOGTARGETS().Pickup());
	for (size_t i = 0; i < LogTargets->size(); i++) {
		auto &entry = LogTargets->at(i);
		_vftprintf(entry.second, Fmt, params);
		DEBUG_DO(fflush(entry.second));
	}
}

#endif