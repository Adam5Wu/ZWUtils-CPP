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

	__BEGIN = NSEC,
	__END = DAY
};

long long Convert(long long const &Value, TimeUnit const &From, TimeUnit const &To);
long long Convert(long long &Value, TimeUnit const &From, TimeUnit const &To);
PCTCHAR UnitName(TimeUnit const &Unit, bool const &Abbrv = false);
TString ToString(long long const &Value, TimeUnit const &DataUnit, TimeUnit const &HiUnit = TimeUnit::__END,
				 TimeUnit const &LoUnit = TimeUnit::__BEGIN, bool const &Abbrv = false);

inline TimeUnit operator++(TimeUnit& x) {
	switch (x) {
		case TimeUnit::NSEC: return x = TimeUnit::USEC;
		case TimeUnit::USEC: return x = TimeUnit::MSEC;
		case TimeUnit::MSEC: return x = TimeUnit::SEC;
		case TimeUnit::SEC: return x = TimeUnit::MIN;
		case TimeUnit::MIN: return x = TimeUnit::HR;
		case TimeUnit::HR: return x = TimeUnit::DAY;
		case TimeUnit::DAY: return x = TimeUnit::__END;
	}
	return x = TimeUnit::__END;
}
inline TimeUnit operator--(TimeUnit& x) {
	switch (x) {
		case TimeUnit::NSEC: return x = TimeUnit::__BEGIN;
		case TimeUnit::USEC: return x = TimeUnit::NSEC;
		case TimeUnit::MSEC: return x = TimeUnit::USEC;
		case TimeUnit::SEC: return x = TimeUnit::MSEC;
		case TimeUnit::MIN: return x = TimeUnit::SEC;
		case TimeUnit::HR: return x = TimeUnit::MIN;
		case TimeUnit::DAY: return x = TimeUnit::HR;
	}
	return x = TimeUnit::__BEGIN;
}
inline TimeUnit operator*(TimeUnit r)	{ return r; }
inline TimeUnit begin(TimeUnit r)		{ return TimeUnit::__BEGIN; }
inline TimeUnit end(TimeUnit r)			{ return TimeUnit::__END; }

enum class SizeUnit : unsigned long long {
	BYTE = 1,
	KB = BYTE * 1024,
	MB = KB * 1024,
	GB = MB * 1024,
	TB = GB * 1024,
	PB = TB * 1024,

	__BEGIN = BYTE,
	__END = PB
};

long long Convert(long long const &Value, SizeUnit const &From, SizeUnit const &To);
long long Convert(long long &Value, SizeUnit const &From, SizeUnit const &To);
PCTCHAR UnitName(SizeUnit const &Unit, bool const &Abbrv = false);
TString ToString(long long const &Value, SizeUnit const &DataUnit, SizeUnit const &HiUnit = SizeUnit::__END,
				 SizeUnit const &LoUnit = SizeUnit::__BEGIN, bool const &Abbrv = false);

inline SizeUnit operator++(SizeUnit& x) {
	switch (x) {
		case SizeUnit::BYTE: return x = SizeUnit::KB;
		case SizeUnit::KB: return x = SizeUnit::MB;
		case SizeUnit::MB: return x = SizeUnit::GB;
		case SizeUnit::GB: return x = SizeUnit::TB;
		case SizeUnit::TB: return x = SizeUnit::PB;
		case SizeUnit::PB: return x = SizeUnit::__END;
	}
	return x = SizeUnit::__END;
}
inline SizeUnit operator--(SizeUnit& x) {
	switch (x) {
		case SizeUnit::BYTE: return x = SizeUnit::__BEGIN;
		case SizeUnit::KB: return x = SizeUnit::BYTE;
		case SizeUnit::MB: return x = SizeUnit::KB;
		case SizeUnit::GB: return x = SizeUnit::MB;
		case SizeUnit::TB: return x = SizeUnit::GB;
		case SizeUnit::PB: return x = SizeUnit::TB;
	}
	return x = SizeUnit::__BEGIN;
}
inline SizeUnit operator*(SizeUnit r)	{ return r; }
inline SizeUnit begin(SizeUnit r)		{ return SizeUnit::__BEGIN; }
inline SizeUnit end(SizeUnit r)			{ return SizeUnit::__END; }

#endif