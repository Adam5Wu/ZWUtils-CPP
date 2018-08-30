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
 * @brief Timing support
 * @author Zhenyu Wu
 * @date Jan 05, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_Timing_H
#define ZWUtils_Timing_H

 // Project global control 
#include "Global.h"

#include "TString.h"
#include "Types.h"
#include "Units.h"

#include <algorithm>

// Value = Second offset
enum class TimeSystem : unsigned long long {
	GREGORIAN = 0,
	UNIX = 11644473600000,
};

class TimeSpan {
	typedef TimeSpan _this;

protected:
	TimeUnit Unit;
	Cardinal64 Value;

public:
	TimeSpan(long long const &xValue = 0, TimeUnit const &xUnit = TimeUnit::MSEC) :
		Unit(xUnit), Value(xValue) {}

	virtual ~TimeSpan(void) {}

	// Copy assignment operation is supported
	_this& operator=(_this const &xTimeSpan);

	long long GetValue(TimeUnit const &Resolution) const;
	TString toString(TimeUnit const &Unit, bool Abbrv = true, bool OmitPlus = true) const;
	TString toString(bool Abbrv = false, bool OmitPlus = true, TimeUnit const &MaxUnit = TimeUnit::__END,
		TimeUnit const &MinUnit = TimeUnit::__BEGIN) const;

	_this operator-(void) const {
		return { -Value.S64, Unit };
	}

	_this operator+(_this const &xTimeSpan) const;

	_this operator-(_this const &xTimeSpan) const {
		return this->operator+(-xTimeSpan);
	}

	_this& operator+=(_this const &xTimeSpan) {
		return *this = *this + xTimeSpan;
	}

	_this& operator-=(_this const &xTimeSpan) {
		return *this += -xTimeSpan;
	}

	bool Same(_this const &TS) const {
		return Same(TS, std::min(Unit, TS.Unit));
	}

	bool Same(_this const &TS, TimeUnit const &Resolution) const {
		return GetValue(Resolution) == TS.GetValue(Resolution);
	}

	bool MoreThan(_this const &TS) const {
		return MoreThan(TS, std::min(Unit, TS.Unit));
	}

	bool MoreThan(_this const &TS, TimeUnit const &Resolution) const {
		return GetValue(Resolution) > TS.GetValue(Resolution);
	}

	bool LessThan(_this const &TS) const {
		return LessThan(TS, std::min(Unit, TS.Unit));
	}

	bool LessThan(_this const &TS, TimeUnit const &Resolution) const {
		return GetValue(Resolution) < TS.GetValue(Resolution);
	}

	bool operator==(_this const &TS) const {
		return Same(TS);
	}

	bool operator!=(_this const &TS) const {
		return !Same(TS);
	}

	bool operator<(_this const &TS) const {
		return LessThan(TS);
	}

	bool operator>(_this const &TS) const {
		return MoreThan(TS);
	}

	bool operator<=(_this const &TS) const {
		return !MoreThan(TS);
	}

	bool operator>=(_this const &TS) const {
		return !LessThan(TS);
	}

	static TimeSpan const Null;

	operator bool() const {
		return Value.U64 != 0;
	}
};

class TimeStamp {
	typedef TimeStamp _this;

protected:
	static TimeUnit const __UNIT;
	static TimeSystem const __SYSTEM;
	Cardinal64 Value;

	static long long Normalize(long long const &xValue, TimeUnit const &xUnit, TimeSystem const &xSystem);

public:
#ifdef WINDOWS
	TimeStamp(unsigned long long const &xValue = 0, TimeUnit const &xUnit = TimeUnit::HNSEC,
		TimeSystem const &xSystem = TimeSystem::GREGORIAN) :
#endif
#ifdef UNIX
		TimeStamp(unsigned long long const &xValue = 0, TimeUnit const &xUnit = TimeUnit::MSEC,
			TimeSystem const &xSystem = TimeSystem::UNIX) :
#endif
		Value(Normalize(xValue, xUnit, xSystem)) {}

#ifdef WINDOWS
	TimeStamp(FILETIME const &xValue) :
		TimeStamp((unsigned long long)xValue.dwHighDateTime << 32 | xValue.dwLowDateTime) {}
#endif

	virtual ~TimeStamp(void) {}

	// Copy assignment operation is supported
	_this& operator=(_this const &xTimeStamp);

	long long UNIXMS(void) const;
	long long MSWINTS(void) const;

	bool At(_this const &TS) const {
		return Value.equalto(TS.Value);
	}

	bool Before(_this const &TS) const {
		return Value.U64 < TS.Value.U64;
	}

	bool After(_this const &TS) const {
		return Value.U64 > TS.Value.U64;
	}

	bool OnTime(_this const &TS, TimeSpan const &TEarly, TimeSpan const &TLate,
		bool EInc = true, bool LInc = true) const;

	bool operator==(_this const &TS) const {
		return At(TS);
	}

	bool operator!=(_this const &TS) const {
		return !At(TS);
	}

	bool operator<(_this const &TS) const {
		return Before(TS);
	}

	bool operator>(_this const &TS) const {
		return After(TS);
	}

	bool operator<=(_this const &TS) const {
		return !After(TS);
	}

	bool operator>=(_this const &TS) const {
		return !Before(TS);
	}

	_this Offset(TimeSpan const &Ofs) const {
		return { Value.U64 + Ofs.GetValue(__UNIT), __UNIT, __SYSTEM };
	}

	_this operator+(TimeSpan const &Ofs) const {
		return Offset(Ofs);
	}

	_this operator-(TimeSpan const &Ofs) const {
		return Offset(-Ofs);
	}

	_this& operator+=(TimeSpan const &Ofs) {
		return *this = *this + Ofs;
	}

	_this& operator-=(TimeSpan const &Ofs) {
		return *this += -Ofs;
	}

	TimeSpan From(_this const &xTimeStamp) const {
		return { Value.S64 - xTimeStamp.Value.S64, __UNIT };
	}

	TimeSpan To(_this const &xTimeStamp) const {
		return xTimeStamp.From(*this);
	}

	TimeSpan operator-(_this const &xTimeStamp) const {
		return From(xTimeStamp);
	}

	TString toString(TimeUnit const &Unit = TimeUnit::MSEC) const;

	static TimeStamp const Null;

	operator bool() const {
		return Value.U64 != 0;
	}

	static _this Now(TimeSpan const &Offset = TimeSpan::Null);
};



#endif