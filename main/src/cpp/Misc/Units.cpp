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

// [Utilities] Platform and charset independent string utilities

#include "Units.h"

long long _Convert(long long &Value, unsigned long long const &FromBase, unsigned long long const &ToBase) {
	unsigned long long Factor;
	unsigned long long Rem;
	if (FromBase >= ToBase) {
		Factor = FromBase / ToBase;
		Rem = 0;
		Value *= Factor;
	} else {
		Factor = ToBase / FromBase;
		Rem = Value % Factor;
		Value /= Factor;
	}
	return Rem;
}

long long Convert(long long &Value, TimeUnit const &From, TimeUnit const &To) {
	return _Convert(Value, (unsigned long long) From, (unsigned long long) To);
}

long long Convert(long long const &Value, TimeUnit const &From, TimeUnit const &To) {
	long long iValue = Value;
	return _Convert(iValue, (unsigned long long) From, (unsigned long long) To), iValue;
}

PCTCHAR UnitName(TimeUnit const &Unit, bool const &Abbrv) {
	switch (Unit) {
		case TimeUnit::NSEC: return Abbrv ? _T("ns") : _T("nanosecond");
		case TimeUnit::HNSEC: return Abbrv ? _T("hns") : _T("hundred-nanosecond");
		case TimeUnit::USEC: return Abbrv ? _T("us") : _T("microsecond");
		case TimeUnit::MSEC: return Abbrv ? _T("ms") : _T("millisecond");
		case TimeUnit::SEC: return Abbrv ? _T("s") : _T("second");
		case TimeUnit::MIN: return Abbrv ? _T("m") : _T("minute");
		case TimeUnit::HR: return Abbrv ? _T("h") : _T("hour");
		case TimeUnit::DAY: return Abbrv ? _T("d") : _T("day");
		default: return Abbrv ? _T("?") : _T("(unknown time-unit)");
	}
}

template <class T>
TString _ToString(long long const &Value, T const &DataUnit, T const &HiUnit, T const &LoUnit, bool const &Abbrv) {
	long long Rem = Value;
	long long RValue = 0;
	auto CurRes = (HiUnit > LoUnit) ? HiUnit : LoUnit;
	TString Ret;
	while (CurRes >= LoUnit) {
		if ((RValue = Rem) == 0) {
			if (!Ret.empty()) break;
		}
		Rem = Convert(RValue, DataUnit, CurRes);
		if ((RValue != 0) || (CurRes <= LoUnit)) {
			Ret.append(Ret.empty() ? 0 : 1, _T(' '))
				.append(TStringCast(RValue))
				.append(Abbrv ? 0 : 1, _T(' '))
				.append(UnitName(CurRes, Abbrv))
				.append(!Abbrv && (RValue > 1) ? 1 : 0, _T('s'));
		}
		--CurRes;
	}
	return Ret.append(Rem != 0 ? 1 : 0, _T('+'));
}

TString ToString(long long const &Value, TimeUnit const &DataUnit, TimeUnit const &HiUnit, TimeUnit const &LoUnit, bool const &Abbrv) {
	return _ToString(Value, DataUnit, HiUnit, LoUnit, Abbrv);
}


long long Convert(long long &Value, SizeUnit const &From, SizeUnit const &To) {
	return _Convert(Value, (unsigned long long) From, (unsigned long long) To);
}

long long Convert(long long const &Value, SizeUnit const &From, SizeUnit const &To) {
	long long iValue = Value;
	return _Convert(iValue, (unsigned long long) From, (unsigned long long) To), iValue;
}

PCTCHAR UnitName(SizeUnit const &Unit, bool const &Abbrv) {
	switch (Unit) {
		case SizeUnit::BYTE: return Abbrv ? _T("B") : _T("byte");
		case SizeUnit::KB: return Abbrv ? _T("KB") : _T("kilobyte");
		case SizeUnit::MB: return Abbrv ? _T("MB") : _T("megabyte");
		case SizeUnit::GB: return Abbrv ? _T("GB") : _T("gigabyte");
		case SizeUnit::TB: return Abbrv ? _T("TB") : _T("terabyte");
		case SizeUnit::PB: return Abbrv ? _T("PB") : _T("petabyte");
		default: return Abbrv ? _T("?") : _T("(unknown size-unit)");
	}
}

TString ToString(long long const &Value, SizeUnit const &DataUnit, SizeUnit const &HiUnit, SizeUnit const &LoUnit, bool const &Abbrv) {
	return _ToString(Value, DataUnit, HiUnit, LoUnit, Abbrv);
}
