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

// [Utilities] Various extended type support

#include "Types.h"
#include "Debug/Exception.h"

#include <iomanip>

// Cardinal32

size_t Cardinal32::hashcode(void) const {
	return std::hash<unsigned long>()(U32);
}

TString Cardinal32::toString(void) const {
	return TStringCast(std::hex << std::uppercase << std::setfill(_T('0')) << std::setw(8) << U32);
}

TString Cardinal32::toString(unsigned int bcnt) const {
	TStringStream StrBuf;
	StrBuf << std::hex << std::uppercase << std::setfill(_T('0'));
	for (unsigned int idx = 0; idx < std::min(bcnt, (unsigned int)sizeof(U8)); idx++) {
		StrBuf << std::setw(2) << U8[idx];
	}
	return StrBuf.str();
}

bool Cardinal32::equalto(Cardinal const &T) const {
	if (typeid(T) == typeid(Cardinal32)) {
		auto & xT = (Cardinal32 const &)T;
		return equalto(xT);
	}
	if (typeid(T) == typeid(Cardinal64)) {
		auto & xT = (Cardinal64 const &)T;
#ifdef LITTLE_ENDIAN
		return xT.C32B.isZero() && equalto(xT.C32A);
#else
		return xT.C32A.isZero() && equalto(xT.C32B);
#endif
	}
	if (typeid(T) == typeid(Cardinal128)) {
		auto & xT = (Cardinal128 const &)T;
#ifdef LITTLE_ENDIAN
		return xT.C64B.isZero() && xT.C32[1].isZero() && equalto(xT.C32[0]);
#else
		return xT.C64A.isZero() && xT.C32[2].isZero() && equalto(xT.C32[3]);
#endif
	}
	if (typeid(T) == typeid(Cardinal256)) {
		auto & xT = (Cardinal256 const &)T;
#ifdef LITTLE_ENDIAN
		return xT.C128B.isZero() && xT.C64[1].isZero() && xT.C32[1].isZero() && equalto(xT.C32[0]);
#else
		return xT.C128A.isZero() && xT.C64[2].isZero() && xT.C32[6].isZero() && equalto(xT.C32[7]);
#endif
	}
	FAIL(_T("Unsupported cardinal type"));
}

bool Cardinal32::equalto(Cardinal32 const &T) const {
	return U32 == T.U32;
}

bool Cardinal32::isZero(void) const {
	return U32 == 0;
}

Cardinal32 const& Cardinal32::ZERO(void) {
	static Cardinal32 __IoFU(0UL);
	return __IoFU;
}

// Cardinal64

size_t Cardinal64::hashcode(void) const {
	return std::hash<unsigned long long>()(U64);
}

TString Cardinal64::toString(void) const {
	return TStringCast(std::hex << std::uppercase << std::setfill(_T('0')) << std::setw(16) << U64);
}

TString Cardinal64::toString(unsigned int bcnt) const {
	TStringStream StrBuf;
	StrBuf << std::hex << std::uppercase << std::setfill(_T('0'));
	for (unsigned int idx = 0; idx < std::min(bcnt, (unsigned int)sizeof(U8)); idx++) {
		StrBuf << std::setw(2) << U8[idx];
	}
	return StrBuf.str();
}

bool Cardinal64::equalto(Cardinal const &T) const {
	if (typeid(T) == typeid(Cardinal32)) {
		auto & xT = (Cardinal32 const &)T;
#ifdef LITTLE_ENDIAN
		return C32B.isZero() && xT.equalto(C32A);
#else
		return C32A.isZero() && xT.equalto(C32B);
#endif
	}
	if (typeid(T) == typeid(Cardinal64)) {
		auto & xT = (Cardinal64 const &)T;
		return equalto(xT);
	}
	if (typeid(T) == typeid(Cardinal128)) {
		auto & xT = (Cardinal128 const &)T;
#ifdef LITTLE_ENDIAN
		return xT.C64B.isZero() && equalto(xT.C64A);
#else
		return xT.C64A.isZero() && equalto(xT.C64B);
#endif
	}
	if (typeid(T) == typeid(Cardinal256)) {
		auto & xT = (Cardinal256 const &)T;
#ifdef LITTLE_ENDIAN
		return xT.C128B.isZero() && xT.C64[1].isZero() && equalto(xT.C64[0]);
#else
		return xT.C128A.isZero() && xT.C64[2].isZero() && equalto(xT.C64[3]);
#endif
	}
	FAIL(_T("Unsupported cardinal type"));
}

bool Cardinal64::equalto(Cardinal64 const &T) const {
	return U64 == T.U64;
}

bool Cardinal64::isZero(void) const {
	return U64 == 0;
}

Cardinal64 const& Cardinal64::ZERO(void) {
	static Cardinal64 __IoFU(0ULL);
	return __IoFU;
}

// Cardinal128

size_t Cardinal128::hashcode(void) const {
	return std::hash<unsigned long long>()(U64A) ^ std::hash<unsigned long long>()(U64B);
}

TString Cardinal128::toString(void) const {
#ifdef LITTLE_ENDIAN
	return TStringCast(std::hex << std::uppercase << std::setfill(_T('0'))
		<< std::setw(16) << U64B << std::setw(16) << U64A);
#endif

#ifdef BIG_ENDIAN
	return TStringCast(std::hex << std::uppercase << std::setfill(_T('0'))
		<< std::setw(16) << U64A << std::setw(16) << U64B);
#endif
}

TString Cardinal128::toString(unsigned int bcnt) const {
	TStringStream StrBuf;
	StrBuf << std::hex << std::uppercase << std::setfill(_T('0'));
	for (unsigned int idx = 0; idx < std::min(bcnt, (unsigned int)sizeof(U8)); idx++) {
		StrBuf << std::setw(2) << U8[idx];
	}
	return StrBuf.str();
}

bool Cardinal128::equalto(Cardinal const &T) const {
	if (typeid(T) == typeid(Cardinal32)) {
		auto & xT = (Cardinal32 const &)T;
#ifdef LITTLE_ENDIAN
		return C64B.isZero() && C32[1].isZero() && xT.equalto(C32[0]);
#else
		return C64A.isZero() && C32[2].isZero() && xT.equalto(C32[3]);
#endif
	}
	if (typeid(T) == typeid(Cardinal64)) {
		auto & xT = (Cardinal64 const &)T;
#ifdef LITTLE_ENDIAN
		return C64B.isZero() && xT.equalto(C64A);
#else
		return C64A.isZero() && xT.equalto(C64B);
#endif
	}
	if (typeid(T) == typeid(Cardinal128)) {
		auto & xT = (Cardinal128 const &)T;
		return equalto(xT);
	}
	if (typeid(T) == typeid(Cardinal256)) {
		auto & xT = (Cardinal256 const &)T;
#ifdef LITTLE_ENDIAN
		return xT.C128B.isZero() && equalto(xT.C128A);
#else
		return xT.C128A.isZero() && equalto(xT.C128B);
#endif
	}
	FAIL(_T("Unsupported cardinal type"));
}

bool Cardinal128::equalto(Cardinal128 const &T) const {
	return ((U64A ^ T.U64A) | (U64B ^ T.U64B)) == 0;
}

GUID Cardinal128::toGUID(void) const {
	return *(LPGUID)U8;
}

void Cardinal128::loadGUID(GUID const &V) {
	memcpy(U8, &V, sizeof(GUID));
}

bool Cardinal128::isZero(void) const {
	return (U64A | U64B) == 0;
}

Cardinal128 const& Cardinal128::ZERO(void) {
	static Cardinal128 __IoFU(0ULL, 0ULL);
	return __IoFU;
}

// Cardinal256

size_t Cardinal256::hashcode(void) const {
	return std::hash<unsigned long long>()(U64[0]) ^ std::hash<unsigned long long>()(U64[1]) ^
		std::hash<unsigned long long>()(U64[2]) ^ std::hash<unsigned long long>()(U64[3]);
}

TString Cardinal256::toString(void) const {
#ifdef LITTLE_ENDIAN
	return TStringCast(std::hex << std::uppercase << std::setfill(_T('0'))
		<< std::setw(16) << U64[3] << std::setw(16) << U64[2]
		<< std::setw(16) << U64[1] << std::setw(16) << U64[0]);
#endif

#ifdef BIG_ENDIAN
	return TStringCast(std::hex << std::uppercase << std::setfill(_T('0'))
		<< std::setw(16) << U64[0] << std::setw(16) << U64[1]
		<< std::setw(16) << U64[2] << std::setw(16) << U64[3]);
#endif
}

TString Cardinal256::toString(unsigned int bcnt) const {
	TStringStream StrBuf;
	StrBuf << std::hex << std::uppercase << std::setfill(_T('0'));
	for (unsigned int idx = 0; idx < std::min(bcnt, (unsigned int)sizeof(U8)); idx++) {
		StrBuf << std::setw(2) << U8[idx];
	}
	return StrBuf.str();
}

bool Cardinal256::equalto(Cardinal const &T) const {
	if (typeid(T) == typeid(Cardinal32)) {
		auto & xT = (Cardinal32 const &)T;
#ifdef LITTLE_ENDIAN
		return C128B.isZero() && C64[1].isZero() && C32[1].isZero() && xT.equalto(C32[0]);
#else
		return C128A.isZero() && C64[2].isZero() && C32[6].isZero() && xT.equalto(C32[7]);
#endif
	}
	if (typeid(T) == typeid(Cardinal64)) {
		auto & xT = (Cardinal64 const &)T;
#ifdef LITTLE_ENDIAN
		return C128B.isZero() && C64[1].isZero() && xT.equalto(C64[0]);
#else
		return C128A.isZero() && C64[2].isZero() && xT.equalto(C64[3]);
#endif
	}
	if (typeid(T) == typeid(Cardinal128)) {
		auto & xT = (Cardinal128 const &)T;
#ifdef LITTLE_ENDIAN
		return C128B.isZero() && xT.equalto(C128A);
#else
		return C128A.isZero() && xT.equalto(C128B);
#endif
	}
	if (typeid(T) == typeid(Cardinal256)) {
		auto & xT = (Cardinal256 const &)T;
		return equalto(xT);
	}
	FAIL(_T("Unsupported cardinal type"));
}

bool Cardinal256::equalto(Cardinal256 const &T) const {
	return ((U64[0] ^ T.U64[0]) | (U64[1] ^ T.U64[1]) | (U64[2] ^ T.U64[2]) | (U64[3] ^ T.U64[3])) == 0;
}

bool Cardinal256::isZero(void) const {
	return (U64[0] | U64[1] | U64[2] | U64[3]) == 0;
}

Cardinal256 const& Cardinal256::ZERO(void) {
	static Cardinal256 __IoFU(0ULL, 0ULL, 0ULL, 0ULL);
	return __IoFU;
}

TString UUIDToString(UUID const &Val) {
	TString Ret(40, NullTChar);
	Cardinal128 Value(Val);
	int result = _sntprintf_s((PTCHAR)Ret.data(), 36 + 1, 36 + 1,
		_T("%08X-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X"),
		Value.U32[0], Value.U16[2], Value.U16[3],
		Value.U8[8], Value.U8[9], Value.U8[10], Value.U8[11],
		Value.U8[12], Value.U8[13], Value.U8[14], Value.U8[15]);
	if (result < 0)
		FAIL(_T("Converting from GUID to string failed with error code %d"), errno);
	return Ret;
}

UUID UUIDFromString(TString const &Str) {
	Cardinal128 Value;

	// Work around VC++ missing support for "hhX"
	int tail8[8];
	int result = _stscanf_s(Str.c_str(), _T("%8X-%4hX-%4hX-%2X%2X-%2X%2X%2X%2X%2X%2X"),
		&Value.U32[0], &Value.U16[2], &Value.U16[3],
		&tail8[0], &tail8[1], &tail8[2], &tail8[3],
		&tail8[4], &tail8[5], &tail8[6], &tail8[7]);
	if (result < 0)
		FAIL(_T("Converting from string to GUID failed with error code %d"), errno);
	if (result < 11)
		FAIL(_T("Converting from string to GUID failed strating from %d"), result + 1);
	// Work around VC++ missing support for "hhX"
	for (int i = 0; i < 8; i++)
		Value.U8[8 + i] = tail8[i];
	return Value.toGUID();
}

UUID const UUID_NULL = { 0 };

TString HexInspect(void* Buf, size_t Len) {
	TString HexBuf;
	unsigned char* ByteBuf = (unsigned char*)Buf;
	for (size_t i = 0; i < Len; i++) {
		HexBuf.append(TStringCast(_T(' ') << std::hex << std::uppercase << std::setfill(_T('0'))
			<< std::setw(2) << ByteBuf[i]));
		if ((i & 15) == 15) HexBuf.append(_T("\n"));
		else if ((i & 7) == 7) HexBuf.append(_T("\t"));
	}
	if ((Len & 15) != 15) HexBuf.append(_T("\n"));
	return HexBuf;
}

UINT32 CountBits32(UINT32 Mask) {
	UINT32 Ret = Mask - ((Mask >> 1) & 0x55555555);
	Ret = (Ret & 0x33333333) + ((Ret >> 2) & 0x33333333);
	Ret = (Ret + (Ret >> 4)) & 0x0f0f0f0f;
	return (Ret * 0x01010101) >> 24;
}

UINT32 CountBits64(UINT64 Mask) {
	UINT64 Ret = Mask - ((Mask >> 1) & 0x5555555555555555);
	Ret = (Ret & 0x3333333333333333) + ((Ret >> 2) & 0x3333333333333333);
	Ret = (Ret + (Ret >> 4)) & 0x0f0f0f0f0f0f0f0f;
	return (Ret * 0x0101010101010101) >> 56;
}

// --- Clonable
Cloneable* Cloneable::MakeClone(IObjAllocator<void> &_Alloc) const {
	FAIL(_T("Abstract function"));
}

// --- CONSTURCTION

struct CONSTRUCTION::EMPLACE_T const CONSTRUCTION::EMPLACE;
struct CONSTRUCTION::HANDOFF_T const CONSTRUCTION::HANDOFF;
struct CONSTRUCTION::CLONE_T const CONSTRUCTION::CLONE;
struct CONSTRUCTION::DEFER_T const CONSTRUCTION::DEFER;
struct CONSTRUCTION::VALIDATED_T const CONSTRUCTION::VALIDATED;
