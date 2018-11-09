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
 * @brief Misc definitions and routine
 * @author Zhenyu Wu
 * @date Jan 05, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_Misc_H
#define ZWUtils_Misc_H

#ifdef _WIN32
#define WINDOWS
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX

#if defined(_M_IX86) | defined(_M_AMD64)
#define LITTLE_ENDIAN
#endif

#ifdef _M_IX86
#define ARCH_32
#endif

#ifdef _M_AMD64
#define ARCH_64
#endif

#if __cplusplus < 201103L
#define NOEXCEPT throw()
#else
#define NOEXCEPT noexcept
#endif

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#define STRINGIZE_HELPER(x) #x
#define STRINGIZE(x) STRINGIZE_HELPER(x)
#define WARNING(desc) message(__FILE__ "(" STRINGIZE(__LINE__) ") WARNING: " desc)

#if defined(_M_IX86)
#define __LINKER_SYMBOL_INNER(name)	_ ## name
#define __LINKER_SYMBOL(name)	__LINKER_SYMBOL_INNER(name)
#endif
#if defined(_M_AMD64)
#define __LINKER_SYMBOL(name)	name
#endif

 // Visual Studio parser bug work-arounds
#define __EXPAND(x)		x
#define __PAREN(...)	(__VA_ARGS__)
#define __VAWRAP(...)	,##__VA_ARGS__

#define __NUM_ARGS_SELECTOR(x16, x15, x14, x13, x12, x11, x10, x9, x8, x7, x6, x5, x4, x3, x2, x1, x0, ...) x0
#define __NUM_ARGS(...) __EXPAND(__NUM_ARGS_SELECTOR(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define __APPLY_0(t, dummy)
#define __APPLY_1(t, a)					t(a)
#define __APPLY_2(t, a, b)				t(a) t(b)
#define __APPLY_3(t, a, b, c)			t(a) t(b) t(c)
#define __APPLY_4(t, a, b, c, d)		t(a) t(b) t(c) t(d)
#define __APPLY_5(t, a, b, c, d, e)				__APPLY_4(t, a, b, c, d)  __APPLY_1(t, e)
#define __APPLY_6(t, a, b, c, d, e, f)			__APPLY_4(t, a, b, c, d)  __APPLY_2(t, e, f)
#define __APPLY_7(t, a, b, c, d, e, f, g)		__APPLY_4(t, a, b, c, d)  __APPLY_3(t, e, f, g)
#define __APPLY_8(t, a, b, c, d, e, f, g, h)	__APPLY_4(t, a, b, c, d)  __APPLY_4(t, e, f, g, h)
#define __APPLY_9(t, a, b, c, d, e, f, g, h, i)							__APPLY_8(t, a, b, c, d, e, f, g, h)  __APPLY_1(t, i)
#define __APPLY_10(t, a, b, c, d, e, f, g, h, i, j)						__APPLY_8(t, a, b, c, d, e, f, g, h)  __APPLY_2(t, i, j)
#define __APPLY_11(t, a, b, c, d, e, f, g, h, i, j, k)					__APPLY_8(t, a, b, c, d, e, f, g, h)  __APPLY_3(t, i, j, k)
#define __APPLY_12(t, a, b, c, d, e, f, g, h, i, j, k, l)				__APPLY_8(t, a, b, c, d, e, f, g, h)  __APPLY_4(t, i, j, k, l)
#define __APPLY_13(t, a, b, c, d, e, f, g, h, i, j, k, l, m)			__APPLY_8(t, a, b, c, d, e, f, g, h)  __APPLY_5(t, i, j, k, l, m)
#define __APPLY_14(t, a, b, c, d, e, f, g, h, i, j, k, l, m, n)			__APPLY_8(t, a, b, c, d, e, f, g, h)  __APPLY_6(t, i, j, k, l, m, n)
#define __APPLY_15(t, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o)		__APPLY_8(t, a, b, c, d, e, f, g, h)  __APPLY_7(t, i, j, k, l, m, n, o)
#define __APPLY_16(t, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p)	__APPLY_8(t, a, b, c, d, e, f, g, h)  __APPLY_8(t, i, j, k, l, m, n, o, p)

#define __APPLY_ALL_N(t, n, ...)	__EXPAND(__APPLY_##n(t, __VA_ARGS__))
#define __APPLY_ALL_WRAP(t, n, ...)	__APPLY_ALL_N(t, n, __VA_ARGS__)
#define __APPLY_ALL(t, ...)			__APPLY_ALL_WRAP(t, __NUM_ARGS(__VA_ARGS__), __VA_ARGS__)

#endif

#endif