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

// [Utilities] Logging support

#include "Logging.h"

#include "Memory/Resource.h"

#include "Threading/SyncObjects.h"

#ifdef WINDOWS
#include <Windows.h>
#endif

#include <unordered_map>

TString const LOGTARGET_CONSOLE(_T("Console"));

typedef std::vector<std::pair<TString, FILE*>> TLogTargets;

TSyncObj<TLogTargets>& LOGTARGETS(void) {
	static TSyncObj<TLogTargets> __IoFU(TLogTargets({ {LOGTARGET_CONSOLE, stderr} }));
	return __IoFU;
}

#define MESSAGE_LOGTARGET_START	"Start of log"
#define MESSAGE_LOGTARGET_END	"End of log"

void __LOG_WRITE(FILE *Target, PCTCHAR Fmt, va_list params);

void LOG_WRITE(FILE *Target, PCTCHAR Fmt, ...) {
	va_list params;
	va_start(params, Fmt);
	TInitResource<va_list> Params(params, [](va_list &X) {va_end(X); });
	__LOG_WRITE(Target, Fmt, params);
}

void SETLOGTARGET(TString const &Name, FILE *xTarget) {
	auto LogTargets(LOGTARGETS().Pickup());
	if (xTarget) {
		if (setvbuf(xTarget, nullptr, _IOLBF, 4096) != 0) {
			LOG(_T("WARNING: Unable to turn off file buffering for log target '%s'"), Name);
		}
		LOG_WRITE(xTarget, _T(MESSAGE_LOGTARGET_START)TNewLine);

		for (auto entry : *LogTargets) {
			if (entry.first.compare(Name) == 0) {
				LOG_WRITE(entry.second, _T(MESSAGE_LOGTARGET_END)TNewLine);
				entry.second = xTarget;
				xTarget = nullptr;
				break;
			}
		}
		if (xTarget) LogTargets->emplace_back(Name, xTarget);
	} else {
		auto Iter = LogTargets->begin();
		while (Iter != LogTargets->end()) {
			if (Iter->first.compare(Name) == 0) {
				LOG_WRITE(Iter->second, _T(MESSAGE_LOGTARGET_END)TNewLine);
				LogTargets->erase(Iter);
				break;
			} else Iter++;
		}
	}
}

FILE * GETLOGTARGET(TString const &Name) {
	auto LogTargets(LOGTARGETS().Pickup());
	auto Iter = LogTargets->begin();
	while (Iter != LogTargets->end()) {
		if (Iter->first.compare(Name) == 0) {
			return Iter->second;
		}
	}
	return nullptr;
}

#ifdef WINDOWS

// Interest read: http://codesnipers.com/?q=the-secret-family-split-in-windows-code-page-functions
void __LocaleInit(void) {
	static TCHAR const* __InitLocale = nullptr;
	if (!__InitLocale) {
#ifndef UNICODE
		setlocale(LC_ALL, __InitLocale = ACP_LOCALE());
#else
		_wsetlocale(LC_ALL, __InitLocale = ACP_LOCALE());
#endif
	}
}

void __LOG_DO(TString const* Target, PCTCHAR Fmt, ...) {
	va_list params;
	va_start(params, Fmt);
	TInitResource<va_list> Params(params, [](va_list &X) {va_end(X); });
	auto LogTargets(LOGTARGETS().Pickup());
	__LocaleInit();
	for (size_t i = 0; i < LogTargets->size(); i++) {
		auto &entry = LogTargets->at(i);
		if ((!Target && entry.first.at(0) != _T('.')) || (Target && entry.first == *Target)) {
			__LOG_WRITE(entry.second, Fmt, params);
		}
	}
}

void __LOG_WRITE(FILE *Target, PCTCHAR Fmt, va_list params) {
	_vftprintf(Target, Fmt, params);
	DEBUG_DO(fflush(Target));
}

#endif