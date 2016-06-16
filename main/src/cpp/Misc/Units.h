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
 * @brief Unit value support
 * @author Zhenyu Wu
 * @date Jan 05, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_Units_H
#define ZWUtils_Units_H

#define __UNIT_ENUM 0

#include "Global.h"
#include "TString.h"

enum class TimeUnit : unsigned long long {
	NSEC = 1,
	HNSEC = NSEC * 100,
	USEC = NSEC * 1000,
	MSEC = USEC * 1000,
	SEC = MSEC * 1000,
	MIN = SEC * 60,
	HR = MIN * 60,
	DAY = HR * 24,
#if __UNIT_ENUM
	__END = 0xFFFFFFFFFFFFFFFF,
	__BEGIN = NSEC,
#endif
};

long long Convert(long long const &Value, TimeUnit From, TimeUnit To);
long long Convert(long long &Value, TimeUnit From, TimeUnit To);
PCTCHAR UnitName(TimeUnit Unit, bool Abbrv = false);
TString Print(long long &Value, TimeUnit From, TimeUnit Resolution = TimeUnit::MSEC, TimeUnit Reserve = TimeUnit::MIN);

#if __UNIT_ENUM
inline TimeUnit operator++(TimeUnit& x) { return x = (TimeUnit)(std::underlying_type<TimeUnit>::type(x) + 1); }
inline TimeUnit operator*(TimeUnit c) { return c; }
inline TimeUnit begin(TimeUnit r) { return TimeUnit::__BEGIN; }
inline TimeUnit end(TimeUnit r)   { return TimeUnit::__END; }
#endif

enum class SizeUnit : unsigned long long {
	BYTE = 1,
	KB = BYTE * 1024,
	MB = KB * 1024,
	GB = MB * 1024,
	TB = GB * 1024,
	PB = TB * 1024,
#if __UNIT_ENUM
	__END = 0xFFFFFFFFFFFFFFFF,
	__BEGIN = BYTE,
#endif
};

long long Convert(long long const &Value, SizeUnit From, SizeUnit To);
long long Convert(long long &Value, SizeUnit From, SizeUnit To);
PCTCHAR UnitName(SizeUnit Unit, bool Abbrv = false);
TString Print(long long &Value, TimeUnit From, SizeUnit Resolution = SizeUnit::BYTE, SizeUnit Reserve = SizeUnit::BYTE);

#if __UNIT_ENUM
inline SizeUnit operator++(SizeUnit& x) { return x = (SizeUnit)(std::underlying_type<SizeUnit>::type(x) + 1); }
inline SizeUnit operator*(SizeUnit c) { return c; }
inline SizeUnit begin(SizeUnit r) { return SizeUnit::__BEGIN; }
inline SizeUnit end(SizeUnit r)   { return SizeUnit::__END; }
#endif

#endif