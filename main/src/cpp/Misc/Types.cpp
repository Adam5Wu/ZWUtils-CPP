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

#define __GEN_HASHCOLLAPSE(v,bcnt)										\
	__ARC_CARDINAL iRet(v);												\
	__ARC_CARDINAL Ret(0L);												\
	unsigned int icnt = std::min(bcnt, (unsigned int)sizeof(iRet.U8));	\
	for (unsigned int i = 0; i < sizeof(U8); i++)						\
		Ret.U8[i % icnt] ^= iRet.U8[i];									\
	return (size_t)Ret

// Cardinal32

size_t Cardinal32::hashcode(unsigned int bcnt) const {
	size_t tRet = std::hash<unsigned long>()(U32);
	if (bcnt == 0) return tRet;
	__GEN_HASHCOLLAPSE(tRet, bcnt);
}

TString Cardinal32::toString(unsigned int bcnt) const {
	TStringStream StrBuf;
	unsigned int start = 0;
	if (bcnt == -1) {
		bcnt = 0;
		while (U8[start++] == 0);
		if (--start) StrBuf << _T('~');
	}
	StrBuf << std::hex << std::uppercase << std::setfill(_T('0'));
	for (unsigned int idx = start; idx < std::min(bcnt ? bcnt : 4, (unsigned int)sizeof(U8)); idx++) {
		StrBuf << std::setw(2) << U8[idx];
	}
	return StrBuf.str();
}

static char HexCharToVal(TCHAR X) {
	int V = X - _T('0');
	if (V < 0) FAIL(_T("Invalid symbol '%c'"), X);
	if (V > 9) {
		V -= _T('A') - _T('0');
		if (V < 0) FAIL(_T("Invalid symbol '%c'"), X);
		if (V > 5) {
			V -= _T('a') - _T('A');
			if (V < 0) FAIL(_T("Invalid symbol '%c'"), X);
		}
		V += 10;
	}
	return (char)V;
}

size_t Cardinal32::fromString(TString const &_S) {
	Cardinal32 iVal(0UL);
	size_t pos = 0;
	for (auto iter = _S.crbegin(); iter != _S.crend(); iter++) {
		char X = HexCharToVal(*iter);
		if (pos & 1) X <<= 4;
#ifdef LITTLE_ENDIAN
		iVal.U8[3 - pos / 2] |= X;
#else
		iVal.U8[pos / 2] |= X;
#endif
		if (++pos >= 8) break;
	}
	operator=(iVal);
	return pos / 2;
}

bool Cardinal32::equalto(Cardinal const &T) const {
	if (typeid(T) == typeid(Cardinal32)) {
		auto & xT = (Cardinal32 const &)T;
		return equalto(xT);
	}
	if (typeid(T) == typeid(Cardinal64)) {
		auto & xT = (Cardinal64 const &)T;
#ifdef LITTLE_ENDIAN
		return xT.U32B == 0 && U32 == xT.U32A;
#else
		return xT.U32A == 0 && U32 == xT.U32B;
#endif
	}
	if (typeid(T) == typeid(Cardinal128)) {
		auto & xT = (Cardinal128 const &)T;
#ifdef LITTLE_ENDIAN
		return xT.U64B == 0 && xT.U32[1] == 0 && U32 == xT.U32[0];
#else
		return xT.U64A == 0 && xT.U32[2] == 0 && U32 == xT.U32[3];
#endif
	}
	if (typeid(T) == typeid(Cardinal256)) {
		auto & xT = (Cardinal256 const &)T;
#ifdef LITTLE_ENDIAN
		return (xT.U64[3] | xT.U64[2] | xT.U64[1]) == 0 && xT.U32[1] == 0 && U32 == xT.U32[0];
#else
		return (xT.U64[0] | xT.U64[1] | xT.U64[2]) == 0 && xT.U32[6] == 0 && U32 == xT.U32[7];
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

size_t Cardinal64::hashcode(unsigned int bcnt) const {
	size_t tRet = std::hash<unsigned long long>()(U64);
	if (bcnt == 0) return tRet;
	__GEN_HASHCOLLAPSE(tRet, bcnt);
}

TString Cardinal64::toString(unsigned int bcnt) const {
	TStringStream StrBuf;
	unsigned int start = 0;
	if (bcnt == -1) {
		bcnt = 0;
		while (U8[start++] == 0);
		if (--start) StrBuf << _T('~');
	}
	StrBuf << std::hex << std::uppercase << std::setfill(_T('0'));
	for (unsigned int idx = start; idx < std::min(bcnt ? bcnt : 8, (unsigned int)sizeof(U8)); idx++) {
		StrBuf << std::setw(2) << U8[idx];
	}
	return StrBuf.str();
}

size_t Cardinal64::fromString(TString const &_S) {
	Cardinal64 iVal(0ULL);
	size_t pos = 0;
	for (auto iter = _S.crbegin(); iter != _S.crend(); iter++) {
		char X = HexCharToVal(*iter);
		if (pos & 1) X <<= 4;
#ifdef LITTLE_ENDIAN
		iVal.U8[7 - pos / 2] |= X;
#else
		iVal.U8[pos / 2] |= X;
#endif
		if (++pos >= 16) break;
	}
	operator=(iVal);
	return pos / 2;
}

bool Cardinal64::equalto(Cardinal const &T) const {
	if (typeid(T) == typeid(Cardinal32)) {
		auto & xT = (Cardinal32 const &)T;
#ifdef LITTLE_ENDIAN
		return U32B == 0 && U32A == xT.U32;
#else
		return U32A == 0 && U32B == xT.U32;
#endif
	}
	if (typeid(T) == typeid(Cardinal64)) {
		auto & xT = (Cardinal64 const &)T;
		return equalto(xT);
	}
	if (typeid(T) == typeid(Cardinal128)) {
		auto & xT = (Cardinal128 const &)T;
#ifdef LITTLE_ENDIAN
		return xT.U64B == 0 && U64 == xT.U64A;
#else
		return xT.U64A == 0 && U64 == xT.U64B;
#endif
	}
	if (typeid(T) == typeid(Cardinal256)) {
		auto & xT = (Cardinal256 const &)T;
#ifdef LITTLE_ENDIAN
		return (xT.U64[3] | xT.U64[2] | xT.U64[1]) == 0 && U64 == xT.U64[0];
#else
		return (xT.U64[0] | xT.U64[1] | xT.U64[2]) == 0 && U64 == xT.U64[3];
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

size_t Cardinal128::hashcode(unsigned int bcnt) const {
	size_t tRet = std::hash<unsigned long long>()(U64A) ^ std::hash<unsigned long long>()(U64B);
	if (bcnt == 0) return tRet;
	__GEN_HASHCOLLAPSE(tRet, bcnt);
}

TString Cardinal128::toString(unsigned int bcnt) const {
	TStringStream StrBuf;
	unsigned int start = 0;
	if (bcnt == -1) {
		bcnt = 0;
		while (U8[start++] == 0);
		if (--start) StrBuf << _T('~');
	}
	StrBuf << std::hex << std::uppercase << std::setfill(_T('0'));
	for (unsigned int idx = start; idx < std::min(bcnt ? bcnt : 16, (unsigned int)sizeof(U8)); idx++) {
		StrBuf << std::setw(2) << U8[idx];
	}
	return StrBuf.str();
}

size_t Cardinal128::fromString(TString const &_S) {
	Cardinal128 iVal(0ULL, 0ULL);
	size_t pos = 0;
	for (auto iter = _S.crbegin(); iter != _S.crend(); iter++) {
		char X = HexCharToVal(*iter);
		if (pos & 1) X <<= 4;
#ifdef LITTLE_ENDIAN
		iVal.U8[15 - pos / 2] |= X;
#else
		iVal.U8[pos / 2] |= X;
#endif
		if (++pos >= 32) break;
	}
	operator=(iVal);
	return pos / 2;
}

bool Cardinal128::equalto(Cardinal const &T) const {
	if (typeid(T) == typeid(Cardinal32)) {
		auto & xT = (Cardinal32 const &)T;
#ifdef LITTLE_ENDIAN
		return U64B == 0 && U32[1] == 0 && U32[0] == xT.U32;
#else
		return U64A == 0 && U32[2] == 0 && U32[3] == xT.U32;
#endif
	}
	if (typeid(T) == typeid(Cardinal64)) {
		auto & xT = (Cardinal64 const &)T;
#ifdef LITTLE_ENDIAN
		return U64B == 0 && U64A == xT.U64;
#else
		return U64A == 0 && U64B == xT.U64;
#endif
	}
	if (typeid(T) == typeid(Cardinal128)) {
		auto & xT = (Cardinal128 const &)T;
		return equalto(xT);
	}
	if (typeid(T) == typeid(Cardinal256)) {
		auto & xT = (Cardinal256 const &)T;
#ifdef LITTLE_ENDIAN
		return (xT.U64[3] | xT.U64[2]) == 0 && U64A == xT.U64[1] && U64B == xT.U64[0];
#else
		return (xT.U64[0] | xT.U64[1]) == 0 && U64A == xT.U64[2] && U64B == xT.U64[3];
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

size_t Cardinal256::hashcode(unsigned int bcnt) const {
	size_t tRet = std::hash<unsigned long long>()(U64[0]) ^ std::hash<unsigned long long>()(U64[1]) ^
		std::hash<unsigned long long>()(U64[2]) ^ std::hash<unsigned long long>()(U64[3]);
	if (bcnt == 0) return tRet;
	__GEN_HASHCOLLAPSE(tRet, bcnt);
}

TString Cardinal256::toString(unsigned int bcnt) const {
	TStringStream StrBuf;
	unsigned int start = 0;
	if (bcnt == -1) {
		bcnt = 0;
		while (U8[start++] == 0);
		if (--start) StrBuf << _T('~');
	}
	StrBuf << std::hex << std::uppercase << std::setfill(_T('0'));
	for (unsigned int idx = start; idx < std::min(bcnt ? bcnt : 32, (unsigned int)sizeof(U8)); idx++) {
		StrBuf << std::setw(2) << U8[idx];
	}
	return StrBuf.str();
}

size_t Cardinal256::fromString(TString const &_S) {
	Cardinal256 iVal(0ULL, 0ULL, 0ULL, 0ULL);
	size_t pos = 0;
	for (auto iter = _S.crbegin(); iter != _S.crend(); iter++) {
		char X = HexCharToVal(*iter);
		if (pos & 1) X <<= 4;
#ifdef LITTLE_ENDIAN
		iVal.U8[31 - pos / 2] |= X;
#else
		iVal.U8[pos / 2] |= X;
#endif
		if (++pos >= 64) break;
	}
	operator=(iVal);
	return pos / 2;
}

bool Cardinal256::equalto(Cardinal const &T) const {
	if (typeid(T) == typeid(Cardinal32)) {
		auto & xT = (Cardinal32 const &)T;
#ifdef LITTLE_ENDIAN
		return (U64[3] | U64[2] | U64[1]) == 0 && U32[1] == 0 && U32[0] == xT.U32;
#else
		return (U64[0] | U64[1] | U64[2]) == 0 && U32[6] == 0 && U32[7] == xT.U32;
#endif
	}
	if (typeid(T) == typeid(Cardinal64)) {
		auto & xT = (Cardinal64 const &)T;
#ifdef LITTLE_ENDIAN
		return (U64[3] | U64[2] | U64[1]) == 0 && U64[0] == xT.U64;
#else
		return (U64[0] | U64[1] | U64[2]) == 0 && U64[3] == xT.U64;
#endif
	}
	if (typeid(T) == typeid(Cardinal128)) {
		auto & xT = (Cardinal128 const &)T;
#ifdef LITTLE_ENDIAN
		return (U64[3] | U64[2]) == 0 && U64[1] == xT.U64A && U64[0] == xT.U64B;
#else
		return (U64[0] | U64[1]) == 0 && U64[2] == xT.U64A && U64[3] == xT.U64B;
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

WString UUIDToWString(UUID const &Val) {
	WString Ret(40, NullWChar);
	Cardinal128 Value(Val);
	int result = _snwprintf_s((PWCHAR)Ret.data(), 36 + 1, 36 + 1,
							  _T("%08X-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X"),
							  Value.U32[0], Value.U16[2], Value.U16[3],
							  Value.U8[8], Value.U8[9], Value.U8[10], Value.U8[11],
							  Value.U8[12], Value.U8[13], Value.U8[14], Value.U8[15]);
	if (result < 0)
		FAIL(_T("Converting from GUID to string failed with error code %d"), errno);
	return Ret;
}

UUID UUIDFromWString(WString const &Str) {
	Cardinal128 Value;

	// Work around VC++ missing support for "hhX"
	int tail8[8];
	int result = swscanf_s(Str.c_str(), _T("%8X-%4hX-%4hX-%2X%2X-%2X%2X%2X%2X%2X%2X"),
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

CString UUIDToCString(UUID const &Val) {
	CString Ret(40, NullAChar);
	Cardinal128 Value(Val);
	int result = _snprintf_s((PCHAR)Ret.data(), 36 + 1, 36 + 1,
							 "%08X-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X",
							 Value.U32[0], Value.U16[2], Value.U16[3],
							 Value.U8[8], Value.U8[9], Value.U8[10], Value.U8[11],
							 Value.U8[12], Value.U8[13], Value.U8[14], Value.U8[15]);
	if (result < 0)
		FAIL(_T("Converting from GUID to string failed with error code %d"), errno);
	return Ret;
}

UUID UUIDFromCString(CString const &Str) {
	Cardinal128 Value;

	// Work around VC++ missing support for "hhX"
	int tail8[8];
	int result = sscanf_s(Str.c_str(), "%8X-%4hX-%4hX-%2X%2X-%2X%2X%2X%2X%2X%2X",
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
