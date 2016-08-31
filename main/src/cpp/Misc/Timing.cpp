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

// [Utilities] Timing support

#include "Timing.h"

#include "Debug/Exception.h"

#ifdef WINDOWS
#include <Windows.h>
#endif

#include <iomanip>

TimeSpan const TimeSpan::Null = {0, TimeUnit::HNSEC};

TimeSpan& TimeSpan::operator=(_this const &xTimeSpan) {
	*const_cast<TimeUnit*>(&Unit) = xTimeSpan.Unit;
	const_cast<Cardinal64*>(&Value)->S64 = xTimeSpan.Value.S64;
	return *this;
}

TimeSpan TimeSpan::operator+(_this const &xTimeSpan) const {
	TimeUnit MergeUnit = Unit < xTimeSpan.Unit ? Unit : xTimeSpan.Unit;
	long long MergeValue = GetValue(MergeUnit) + xTimeSpan.GetValue(MergeUnit);
	return TimeSpan(MergeValue, MergeUnit);
}

long long TimeSpan::GetValue(TimeUnit const &Resolution) const {
	long long RValue = Value.S64;
	Convert(RValue, Unit, Resolution);
	return RValue;
}

TString TimeSpan::toString(TimeUnit const &Unit, bool Abbrv) const {
	return toString(Abbrv, Unit, Unit);
}

TString TimeSpan::toString(bool Abbrv, TimeUnit const &HiUnit, TimeUnit const &LoUnit) const {
	return ToString(Value.S64, Unit, Abbrv, HiUnit, LoUnit);
}

// Choose the smallest possible granularity
TimeUnit const TimeStamp::__UNIT = TimeUnit::HNSEC;
// Choose the eariliest possible start time
TimeSystem const TimeStamp::__SYSTEM = TimeSystem::GREGORIAN;

TimeStamp& TimeStamp::operator=(_this const &xTimeStamp) {
	const_cast<Cardinal64*>(&Value)->U64 = xTimeStamp.Value.U64;
	return *this;
}

long long TimeStamp::Normalize(long long const &xValue, TimeUnit const &xUnit, TimeSystem const &xSystem) {
	long long RValue = xValue;
	Convert(RValue, xUnit, __UNIT);
	long long Offset = (unsigned long long) xSystem - (unsigned long long) __SYSTEM;
	Convert(Offset, TimeUnit::MSEC, __UNIT);
	return RValue + Offset;
}

long long TimeStamp::UNIXMS(void) const {
	long long RValue = Value.U64;
	Convert(RValue, __UNIT, TimeUnit::MSEC);
	long long Offset = (unsigned long long) __SYSTEM - (unsigned long long) TimeSystem::UNIX;
	return RValue + Offset;
}

long long TimeStamp::MSWINTS(void) const {
	long long RValue = Value.U64;
	Convert(RValue, __UNIT, TimeUnit::HNSEC);
	long long Offset = (unsigned long long) __SYSTEM - (unsigned long long) TimeSystem::GREGORIAN;
	return RValue + Offset;
}

TString TimeStamp::toString(TimeUnit const &Resolution) const {
#ifdef WINDOWS
	SYSTEMTIME SystemTime;
	FILETIME FileTime;
	((Cardinal64&)FileTime).U64 = MSWINTS();
	FileTimeToSystemTime(&FileTime, &SystemTime);

	TStringStream StrBuf;
	StrBuf << std::setw(4) << SystemTime.wYear
		<< _T('/') << std::setw(2) << std::setfill(_T('0')) << SystemTime.wMonth
		<< _T('/') << std::setw(2) << std::setfill(_T('0')) << SystemTime.wDay;
	switch (Resolution) {
		case TimeUnit::DAY: break;
		case TimeUnit::HR:
			SystemTime.wMinute = 0;
		case TimeUnit::MIN:
		case TimeUnit::SEC:
		case TimeUnit::MSEC:
			StrBuf << _T(' ') << std::setw(2) << std::setfill(_T('0')) << SystemTime.wHour;
			StrBuf << _T(':') << std::setw(2) << std::setfill(_T('0')) << SystemTime.wMinute;
			if ((Resolution == TimeUnit::HR) || (Resolution == TimeUnit::MIN)) break;
			StrBuf << _T(':') << std::setw(2) << std::setfill(_T('0')) << SystemTime.wSecond;
			if (Resolution == TimeUnit::SEC) break;
			StrBuf << _T('.') << std::setw(3) << std::setfill(_T('0')) << SystemTime.wMilliseconds;
			break;
		default:
			FAIL(_T("Unrecognized time-unit resolution"));
	}
	return StrBuf.str();
#endif
}

bool TimeStamp::Before(TimeStamp const &TS) const {
	return TS.Value.U64 > Value.U64;
}

bool TimeStamp::After(TimeStamp const &TS) const {
	return TS.Value.U64 < Value.U64;
}

TimeStamp TimeStamp::Offset(TimeSpan const &Offset) const {
	return{Value.U64 + Offset.GetValue(__UNIT), __UNIT, __SYSTEM};
}

TimeSpan TimeStamp::From(TimeStamp const &xTimeStamp) const {
	return{Value.S64 - xTimeStamp.Value.S64, __UNIT};
}

TimeStamp TimeStamp::Now(TimeSpan const &Offset) {
#ifdef WINDOWS
	Cardinal64 FileTime;
	GetSystemTimeAsFileTime((FILETIME*)&FileTime);
	// NOTE: Windows FileTime is already in the native time unit and system
	return{FileTime.U64 + Offset.GetValue(__UNIT), __UNIT, __SYSTEM};
#endif
}