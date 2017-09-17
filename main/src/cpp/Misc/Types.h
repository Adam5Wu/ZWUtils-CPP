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
 * @brief Various extended type support
 * @author Zhenyu Wu
 * @date Jan 06, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_Types_H
#define ZWUtils_Types_H

#include "Global.h"
#include "TString.h"

#ifdef WINDOWS
#include <Windows.h>
#else
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;
typedef signed long long INT64;

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
#endif

#define ENFORCE_DERIVE(TBase, TDerived) \
	static_assert(std::is_base_of<TBase, TDerived>::value, "Type inheritance constraint violation (require parent of <" #TBase ">)")

#define ENFORCE_TYPE(TObject, TRefType) \
	static_assert(std::is_same<TObject, TRefType>::value, "Type constraint violation (require <" #TRefType ">)")

#define ENFORCE_POLYMORPHIC(TObject) \
	static_assert(std::is_polymorphic<TObject>::value, "Type is not polymorphic")

#define MEMBERFUNC_PROBE(Func)									\
template<class T>												\
struct Has_ ## Func {											\
	template<class C> static int  _Probe(decltype(&C::Func));	\
	template<class C> static char _Probe(...);					\
	static const bool value = sizeof(_Probe<T>(nullptr)) > 1;	\
}

#define MEMBERFUNC_PROBE(Func)									\
template<class T>												\
struct Has_ ## Func {											\
	template<class C> static int  _Probe(decltype(&C::Func));	\
	template<class C> static char _Probe(...);					\
	static const bool value = sizeof(_Probe<T>(nullptr)) > 1;	\
}

#ifdef ARCH_32
typedef long			__ARC_INT;
typedef unsigned long	__ARC_UINT;
#endif

#ifdef ARCH_64
typedef long long			__ARC_INT;
typedef unsigned long long	__ARC_UINT;
#endif

union Cardinal32 {
	unsigned long U32;
	struct { unsigned short U16A, U16B; };
	struct { unsigned char U8[4]; };
	long S32;
	struct { short S16A, S16B; };
	struct { char S8[4]; };

	Cardinal32(void) {}
	Cardinal32(unsigned long const & _Value) : U32(_Value) {}
	Cardinal32(long const & _Value) : S32(_Value) {}

	size_t hashcode(void) const;
	TString toString(void) const;
	bool equalto(Cardinal32 const &T) const;

	static Cardinal32 const& ZERO(void);
};

inline bool operator ==(Cardinal32 const &A, Cardinal32 const &B)
{ return A.equalto(B); }
inline bool operator !=(Cardinal32 const &A, Cardinal32 const &B)
{ return !(A == B); }

union Cardinal64 {
	unsigned long long U64;
	struct { unsigned long U32A, U32B; };
	struct { unsigned short U16[4]; };
	struct { unsigned char U8[8]; };
	long long S64;
	struct { long S32A, S32B; };
	struct { short S16[4]; };
	struct { char S8[8]; };

	Cardinal64(void) {}
	Cardinal64(unsigned long long const & _Value) : U64(_Value) {}
	Cardinal64(long long const & _Value) : S64(_Value) {}

	size_t hashcode(void) const;
	TString toString(void) const;
	bool equalto(Cardinal64 const &T) const;

	static Cardinal64 const& ZERO(void);
};

inline bool operator ==(Cardinal64 const &A, Cardinal64 const &B)
{ return A.equalto(B); }
inline bool operator !=(Cardinal64 const &A, Cardinal64 const &B)
{ return !(A == B); }

union Cardinal128 {
	struct { unsigned long long U64A, U64B; };
	struct { unsigned long U32[4]; };
	struct { unsigned short U16[8]; };
	struct { unsigned char U8[16]; };
	struct { long long S64A, S64B; };
	struct { long S32[4]; };
	struct { short S16[8]; };
	struct { char S8[16]; };

	Cardinal128(void) {}
	Cardinal128(unsigned long long const & _VA, unsigned long long const & _VB) : U64A(_VA), U64B(_VB) {}
	Cardinal128(long long const & _VA, long long const & _VB) : S64A(_VA), S64B(_VB) {}

	size_t hashcode(void) const;
	TString toString(void) const;
	bool equalto(Cardinal128 const &T) const;

	static Cardinal128 const& ZERO(void);
};

inline bool operator ==(Cardinal128 const &A, Cardinal128 const &B)
{ return A.equalto(B); }
inline bool operator !=(Cardinal128 const &A, Cardinal128 const &B)
{ return !(A == B); }

union Cardinal256 {
	struct { unsigned long long U64[4]; };
	struct { unsigned long U32[8]; };
	struct { unsigned short U16[16]; };
	struct { unsigned char U8[32]; };
	struct { long long S64[4]; };
	struct { long S32[8]; };
	struct { short S16[16]; };
	struct { char S8[32]; };

#if _MSC_VER >= 1900
	Cardinal256(void) {}
	Cardinal256(unsigned long long const _V0, unsigned long long const _V1,
				unsigned long long const _V2, unsigned long long const _V3) : U64{_V0, _V1, _V2, _V3} {}
	Cardinal256(long long const _V0, long long const _V1,
				long long const _V2, long long const _V3) : S64{_V0, _V1, _V2, _V3} {}
#endif

	size_t hashcode(void) const;
	TString toString(void) const;
	bool equalto(Cardinal256 const &T) const;

	static Cardinal256 const& ZERO(void);
};

inline bool operator ==(Cardinal256 const &A, Cardinal256 const &B)
{ return A.equalto(B); }
inline bool operator !=(Cardinal256 const &A, Cardinal256 const &B)
{ return !(A == B); }

#ifdef WINDOWS
typedef GUID UUID;
#endif

TString UUIDToString(UUID const &Val);
UUID UUIDFromString(TString const &Str);

extern UUID const UUID_NULL;

TString HexInspect(void* Buf, size_t Len);

template<typename TOrdinal32>
class TInterlockedOrdinal32 {
private:
	long volatile rOrdinal;

public:
	TInterlockedOrdinal32(TOrdinal32 const &xOrdinal) : rOrdinal((long)xOrdinal) {}
	~TInterlockedOrdinal32(void) {}

	TOrdinal32 Increment(void);
	TOrdinal32 Decrement(void);
	TOrdinal32 Add(TOrdinal32 const &Amount);
	TOrdinal32 Exchange(TOrdinal32 const &SwpVal);
	TOrdinal32 ExchangeAdd(TOrdinal32 const &Amount);
	TOrdinal32 CompareAndSwap(TOrdinal32 const &Cmp, TOrdinal32 const &SwpVal);
	TOrdinal32 And(TOrdinal32 const &Value);
	TOrdinal32 Or(TOrdinal32 const &Value);
	TOrdinal32 Xor(TOrdinal32 const &Value);
	bool BitTestSet(unsigned int const &Bit);
	bool BitTestReset(unsigned int const &Bit);

	TOrdinal32 operator++(void)
	{ return Increment(); }
	TOrdinal32 operator++(int)
	{ return Increment() - 1; }
	TOrdinal32 operator--(void)
	{ return Decrement(); }
	TOrdinal32 operator--(int)
	{ return Decrement() + 1; }
	TOrdinal32 operator+=(TOrdinal32 const &Amount)
	{ return Add(Amount); }
	TOrdinal32 operator-=(TOrdinal32 const &Amount)
	{ return Add(-Amount); }
	TOrdinal32 operator=(TOrdinal32 const &Value)
	{ return Exchange(Value), Value; }
	TOrdinal32 const& operator~(void) const
	{ return (TOrdinal32 &)rOrdinal; }
};

template<typename TOrdinal64>
class TInterlockedOrdinal64 {
private:
	long long volatile rOrdinal;

public:
	TInterlockedOrdinal64(TOrdinal64 const &xOrdinal) : rOrdinal((long long)xOrdinal) {}
	~TInterlockedOrdinal64(void) {}

	TOrdinal64 Increment(void);
	TOrdinal64 Decrement(void);
	TOrdinal64 Add(TOrdinal64 const &Amount);
	TOrdinal64 Exchange(TOrdinal64 const &SwpVal);
	TOrdinal64 ExchangeAdd(TOrdinal64 const &Amount);
	TOrdinal64 CompareAndSwap(TOrdinal64 const &Cmp, TOrdinal64 const &SwpVal);
	TOrdinal64 And(TOrdinal64 const &Value);
	TOrdinal64 Or(TOrdinal64 const &Value);
	TOrdinal64 Xor(TOrdinal64 const &Value);
	bool BitTestSet(unsigned int const &Bit);
	bool BitTestReset(unsigned int const &Bit);

	TOrdinal64 operator++(void)
	{ return Increment(); }
	TOrdinal64 operator++(int)
	{ return Increment() - 1; }
	TOrdinal64 operator--(void)
	{ return Decrement(); }
	TOrdinal64 operator--(int)
	{ return Decrement() + 1; }
	TOrdinal64 operator+=(TOrdinal64 const &Amount)
	{ return Add(Amount); }
	TOrdinal64 operator-=(TOrdinal64 const &Amount)
	{ return Add(-Amount); }
	TOrdinal64 operator=(TOrdinal64 const &Value)
	{ return Exchange(Value), Value; }
	TOrdinal64 const& operator~(void) const
	{ return (TOrdinal64 &)rOrdinal; }
};

#ifdef ARCH_32
#define TInterlockedOrdinal TInterlockedOrdinal32
typedef TInterlockedOrdinal<__ARC_INT> TInterlockedArchInt;
typedef TInterlockedOrdinal<__ARC_UINT> TInterlockedArchUInt;
#endif

#ifdef ARCH_64
#define TInterlockedOrdinal TInterlockedOrdinal64
typedef TInterlockedOrdinal<__ARC_INT> TInterlockedArchInt;
typedef TInterlockedOrdinal<__ARC_UINT> TInterlockedArchUInt;
#endif

#ifdef WINDOWS

template<typename TOrdinal32>
TOrdinal32 TInterlockedOrdinal32<TOrdinal32>::Increment(void)
{ return (TOrdinal32)InterlockedIncrement(&rOrdinal); }

template<typename TOrdinal32>
TOrdinal32 TInterlockedOrdinal32<TOrdinal32>::Decrement(void)
{ return (TOrdinal32)InterlockedDecrement(&rOrdinal); }

template<typename TOrdinal32>
TOrdinal32 TInterlockedOrdinal32<TOrdinal32>::Add(TOrdinal32 const &Amount)
{ return (TOrdinal32)InterlockedAdd(&rOrdinal, (LONG)Amount); }

template<typename TOrdinal32>
TOrdinal32 TInterlockedOrdinal32<TOrdinal32>::Exchange(TOrdinal32 const &SwpVal)
{ return (TOrdinal32)InterlockedExchange(&rOrdinal, (LONG)SwpVal); }

template<typename TOrdinal32>
TOrdinal32 TInterlockedOrdinal32<TOrdinal32>::ExchangeAdd(TOrdinal32 const &Amount)
{ return (TOrdinal32)InterlockedExchangeAdd(&rOrdinal, (LONG)Amount); }

template<typename TOrdinal32>
TOrdinal32 TInterlockedOrdinal32<TOrdinal32>::CompareAndSwap(TOrdinal32 const &Cmp, TOrdinal32 const &SwpVal)
{ return (TOrdinal32)InterlockedCompareExchange(&rOrdinal, (LONG)SwpVal, (LONG)Cmp); }

template<typename TOrdinal32>
TOrdinal32 TInterlockedOrdinal32<TOrdinal32>::And(TOrdinal32 const &Value)
{ return (TOrdinal32)InterlockedAnd(&rOrdinal, (LONG)Value); }

template<typename TOrdinal32>
TOrdinal32 TInterlockedOrdinal32<TOrdinal32>::Or(TOrdinal32 const &Value)
{ return (TOrdinal32)InterlockedOr(&rOrdinal, (LONG)Value); }

template<typename TOrdinal32>
TOrdinal32 TInterlockedOrdinal32<TOrdinal32>::Xor(TOrdinal32 const &Value)
{ return (TOrdinal32)InterlockedXor(&rOrdinal, (LONG)Value); }

template<typename TOrdinal32>
bool TInterlockedOrdinal32<TOrdinal32>::BitTestSet(unsigned int const &Bit)
{ return InterlockedBitTestAndSet(&rOrdinal, (LONG)Bit); }

template<typename TOrdinal32>
bool TInterlockedOrdinal32<TOrdinal32>::BitTestReset(unsigned int const &Bit)
{ return InterlockedBitTestAndReset(&rOrdinal, (LONG)Bit); }


template<typename TOrdinal64>
TOrdinal64 TInterlockedOrdinal64<TOrdinal64>::Increment(void)
{ return (TOrdinal64)InterlockedIncrement64(&rOrdinal); }

template<typename TOrdinal64>
TOrdinal64 TInterlockedOrdinal64<TOrdinal64>::Decrement(void)
{ return (TOrdinal64)InterlockedDecrement64(&rOrdinal); }

template<typename TOrdinal64>
TOrdinal64 TInterlockedOrdinal64<TOrdinal64>::Add(TOrdinal64 const &Amount)
{ return (TOrdinal64)InterlockedAdd64(&rOrdinal, (LONGLONG)Amount); }

template<typename TOrdinal64>
TOrdinal64 TInterlockedOrdinal64<TOrdinal64>::Exchange(TOrdinal64 const &SwpVal)
{ return (TOrdinal64)InterlockedExchange64(&rOrdinal, (LONGLONG)SwpVal); }

template<typename TOrdinal64>
TOrdinal64 TInterlockedOrdinal64<TOrdinal64>::ExchangeAdd(TOrdinal64 const &Amount)
{ return (TOrdinal64)InterlockedExchangeAdd64(&rOrdinal, (LONGLONG)Amount); }

template<typename TOrdinal64>
TOrdinal64 TInterlockedOrdinal64<TOrdinal64>::CompareAndSwap(TOrdinal64 const &Cmp, TOrdinal64 const &SwpVal)
{ return (TOrdinal64)InterlockedCompareExchange64(&rOrdinal, (LONGLONG)SwpVal, (LONGLONG)Cmp); }

template<typename TOrdinal64>
TOrdinal64 TInterlockedOrdinal64<TOrdinal64>::And(TOrdinal64 const &Value)
{ return (TOrdinal64)InterlockedAnd64(&rOrdinal, (LONGLONG)Value); }

template<typename TOrdinal64>
TOrdinal64 TInterlockedOrdinal64<TOrdinal64>::Or(TOrdinal64 const &Value)
{ return (TOrdinal64)InterlockedOr64(&rOrdinal, (LONGLONG)Value); }

template<typename TOrdinal64>
TOrdinal64 TInterlockedOrdinal64<TOrdinal64>::Xor(TOrdinal64 const &Value)
{ return (TOrdinal64)InterlockedXor64(&rOrdinal, (LONGLONG)Value); }

template<typename TOrdinal64>
bool TInterlockedOrdinal64<TOrdinal64>::BitTestSet(unsigned int const &Bit)
{ return InterlockedBitTestAndSet64(&rOrdinal, (LONGLONG)Bit); }

template<typename TOrdinal64>
bool TInterlockedOrdinal64<TOrdinal64>::BitTestReset(unsigned int const &Bit)
{ return InterlockedBitTestAndReset64(&rOrdinal, (LONGLONG)Bit); }

#endif

template<class X>
class TCastable {
	typedef TCastable _this;

protected:
	virtual ~TCastable(void) {}

public:
	template<class T,
		typename = std::enable_if<std::is_base_of<X, T>::value>::type
	>
	static X* Cast(T *Obj)
	{ return Obj; }

	template<class T,
		typename = std::enable_if<std::is_base_of<X, T>::value>::type
	>
	static X const* Cast(T const *Obj)
	{ return Obj; }

	template<class T,
		typename = std::enable_if<!std::is_base_of<X, T>::value>::type,
		typename = std::enable_if<std::is_polymorphic<T>::value>::type
	>
	static X* Cast(T *Obj)
	{ return dynamic_cast<X *>(Obj); }

	template<class T,
		typename = std::enable_if<!std::is_base_of<X, T>::value>::type,
		typename = std::enable_if<std::is_polymorphic<T>::value>::type
	>
	static X const* Cast(T const *Obj)
	{ return dynamic_cast<X const*>(Obj); }

	template<class T,
		typename = std::enable_if<!std::is_base_of<X, T>::value>::type,
		typename = std::enable_if<!std::is_polymorphic<T>::value>::type,
		typename = void
	>
	static X* Cast(T*)
	{ return nullptr; }

	template<class T,
		typename = std::enable_if<!std::is_base_of<X, T>::value>::type,
		typename = std::enable_if<!std::is_polymorphic<T>::value>::type,
		typename = void
	>
	static X const* Cast(T const*)
	{ return nullptr; }
};

// For use in wrapper classes to determine how to construct wrapped instance
class CONSTRUCTION {
public:
	// Construct using parameters in-place
	static struct EMPLACE_T {} const EMPLACE;
	// Take ownership of an already constructed object
	static struct HANDOFF_T {} const HANDOFF;
	// Make a clone of an constructed object
	static struct CLONE_T {} const CLONE;
};

#endif