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

// [Utilities] Basic debug support

#include "Debug.h"

#ifndef SOLUTION_PATH
#pragma WARNING("Please define the project path before compiling this file!")
#if _MSC_VER
#pragma WARNING("Hint - /D \"SOLUTION_PATH=\\\"$(SolutionDir.Replace('\\','\\\\'))\\\\\"\"")
#endif
#define SOLUTION_PATH ""
#else
#pragma message("SOLUTION_PATH = " SOLUTION_PATH)
#endif //SOLUTION_PATH

// Aquire the skip length of source code (so we can properly print relative source file paths)
#ifdef WINDOWS

#include "Misc/TString.h"

PCTCHAR __RelPath(PCTCHAR Path) {
	static size_t __RelPathLen = wcslen(_T(SOLUTION_PATH));
	if (_tcsnicmp(Path, _T(SOLUTION_PATH), __RelPathLen) == 0)
		return Path + __RelPathLen;
	return Path;
}

PCTCHAR __PTID(void) {
	__declspec(thread) static TCHAR PTID[12]{ NullWChar };
	if (PTID[0] == NullWChar)
		BUFFMT(&PTID[0], 12, _T("%5d:%-5d"), GetCurrentProcessId(), GetCurrentThreadId());
	return PTID;
}

#endif