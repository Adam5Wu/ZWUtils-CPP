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
 * @brief Stack walking support
 * @author Zhenyu Wu
 * @date Jul 29, 2015: Adapted from original code project: http://www.codeproject.com/Articles/11132/Walking-the-callstack
 **/

#ifndef ZWUtils_StackWalker_H
#define ZWUtils_StackWalker_H

 // Project global control 
#include "Misc/Global.h"

#include "Exception.h"

#include "Misc/Types.h"

#include "System/SysTypes.h"

#include <functional>

#ifdef WINDOWS

#define STACKWALK_MAX_NAMELEN 1024

class TStackWalker {
public:
	typedef struct {
		PVOID Address;
		PVOID ModuleBase;
		DWORD64 SmybolOffset;
		TString ModuleName;
		TString FileName;
		TString SymbolName;
		DWORD LineNumber;
		DWORD LineOffset;
		enum class TSymbolType {
			None, COFF, CV, PDB, Exported, Deferred, SYM, DIA, Virtual
		} SymbolType;
	} CallstackEntry;

	static TString STR_SymbolType(CallstackEntry::TSymbolType SymbolType);

	typedef std::function<bool(CallstackEntry const &)> OnStackEntry;
	typedef std::function<bool(LPCTSTR, DWORD, PVOID)> OnTraceError;

public:
	virtual ~TStackWalker(void) {};

	static bool LogEntry(CallstackEntry const &Entry);
	static bool LogError(LPCTSTR FuncHint, DWORD errCode, PVOID addr);

	static TString FormatEntry(CallstackEntry const &Entry);
	static TString FormatError(LPCTSTR FuncHint, DWORD errCode, PVOID addr);

	virtual bool Trace(THandle const &Thread, CONTEXT &Context,
		OnStackEntry const &EntryCallback = LogEntry,
		OnTraceError const &ErrorCallback = LogError) {
		FAIL(_T("Abstract Function"));
	}
};

void LocalStackWalker_Init(bool xOnlineSymServer = true, TString const &xOptSymPath = EMPTY_TSTRING());

bool LocalStackTrace(THandle const &Thread, CONTEXT &Context,
	TStackWalker::OnStackEntry const& EntryCallback = TStackWalker::LogEntry,
	TStackWalker::OnTraceError const& ErrorCallback = TStackWalker::LogError);

#endif

#endif
