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

// [Utilities] Logging support

#include "StackWalker.h"

#ifdef WINDOWS

#include "Debug/Logging.h"
#include "Debug/SysError.h"

#include "Memory/ManagedRef.h"

#include "Threading/SyncObjects.h"

#pragma comment(lib, "dbghelp.lib")

// We used extended support functions
// See: https://docs.microsoft.com/en-us/windows/desktop/debug/updated-platform-support
#define _IMAGEHLP64
#include <dbghelp.h>

#ifdef UNICODE
#define IMAGEHLP_LINEW IMAGEHLP_LINEW64

#define IMAGEHLP_MODULE_T IMAGEHLP_MODULEW
#define IMAGEHLP_LINE_T IMAGEHLP_LINEW
#define SYMBOL_INFO_PACKAGE_T SYMBOL_INFO_PACKAGEW
#define SYMBOL_INFO_T SYMBOL_INFOW
#define SymInitializeT SymInitializeW
#define SymGetLineFromAddrT SymGetLineFromAddrW
#define SymGetModuleInfoT SymGetModuleInfoW
#define SymFromAddrT SymFromAddrW
#define UnDecorateSymbolNameT UnDecorateSymbolNameW

// Ensure backward compatibility with early versions
typedef struct {
	DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_MODULE64)
	DWORD64  BaseOfImage;            // base load address of module
	DWORD    ImageSize;              // virtual size of the loaded module
	DWORD    TimeDateStamp;          // date/time stamp from pe header
	DWORD    CheckSum;               // checksum from the pe header
	DWORD    NumSyms;                // number of symbols in the symbol table
	SYM_TYPE SymType;                // type of symbols loaded
	WCHAR    ModuleName[32];         // module name
	WCHAR    ImageName[256];         // image name
} IMAGEHLP_MODULE_V2_T;
#else
#define IMAGEHLP_MODULE_T IMAGEHLP_MODULE
#define IMAGEHLP_LINE_T IMAGEHLP_LINE
#define SYMBOL_INFO_PACKAGE_T SYMBOL_INFO_PACKAGE
#define SYMBOL_INFO_T SYMBOL_INFO
#define SymInitializeT SymInitialize
#define SymGetLineFromAddrT SymGetLineFromAddr
#define SymGetModuleInfoT SymGetModuleInfo
#define SymFromAddrT SymFromAddr
#define UnDecorateSymbolNameT UnDecorateSymbolName

// Ensure backward compatibility with early versions
typedef struct IMAGEHLP_MODULE64_V2 {
	DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_MODULE64)
	DWORD64  BaseOfImage;            // base load address of module
	DWORD    ImageSize;              // virtual size of the loaded module
	DWORD    TimeDateStamp;          // date/time stamp from pe header
	DWORD    CheckSum;               // checksum from the pe header
	DWORD    NumSyms;                // number of symbols in the symbol table
	SYM_TYPE SymType;                // type of symbols loaded
	CHAR    ModuleName[32];          // module name
	CHAR    ImageName[256];          // image name
} IMAGEHLP_MODULE_V2_T;
#endif

#include <unordered_set>

TString TStackWalker::STR_SymbolType(CallstackEntry::TSymbolType SymbolType) {
	static PCTCHAR _STR_SymbolType[] = {
		_T("None"), _T("COFF"), _T("CV"), _T("PDB"),
		_T("Exported"), _T("Deferred"), _T("SYM"), _T("DIA"),
		_T("Virtual")
	};
	return (int)SymbolType < NumSymTypes ? _STR_SymbolType[(int)SymbolType] : _T("Unrecognized");
}

#define STR_NoSymbolName _T("(Unknown-Symbol)")
#define STR_NoModuleName _T("(Unknown-Module)")

bool TStackWalker::LogEntry(CallstackEntry const &Entry) {
	LOG(_T("%s"), FormatEntry(Entry).c_str());
	return true;
}

bool TStackWalker::LogError(LPCTSTR FuncHint, DWORD errCode, PVOID addr) {
	OutputDebugString((FormatError(FuncHint, errCode, addr) + _T("\r\n")).c_str());
	return true;
}

TString TStackWalker::FormatEntry(CallstackEntry const &Entry) {
	return Entry.FileName.empty() ?
		TStringCast(_T('!') << (Entry.ModuleName.empty() ? STR_NoModuleName : Entry.ModuleName.c_str())
					<< _T('@') << (Entry.ModuleBase ?
								   TStringCast(Entry.ModuleBase
											   << _T('+') << std::hex
											   << (__ARC_INT)Entry.Address - (__ARC_INT)Entry.ModuleBase) :
								   TStringCast(Entry.Address))
					<< _T(':')
					<< (Entry.SymbolName.empty() ? STR_NoSymbolName : Entry.SymbolName.c_str())
					<< (Entry.SmybolOffset ? TStringCast(_T('+') << Entry.SmybolOffset) : _T(""))
		) :
		TStringCast(Entry.FileName << _T('#') << Entry.LineNumber
					// << (Entry.LineOffset ? _T("+") : _T(""))
					<< _T(':')
					<< (Entry.SymbolName.empty() ? STR_NoSymbolName : Entry.SymbolName.c_str())
		);
}

TString TStackWalker::FormatError(LPCTSTR FuncHint, DWORD errCode, PVOID addr) {
	TString ErrMsg;
	if (DecodeSysError(errCode, ErrMsg) != nullptr)
		return TStringCast(_T('!') << FuncHint << _T('@') << addr << _T(':') << ErrMsg);
	return TStringCast(_T('!') << FuncHint << _T('@') << addr << _T(":0x") << std::hex << errCode);
}

class StackWalker_Impl : public TStackWalker {
protected:
	THandle Process;
	TString TraceSymPath;

	TString GetSymPath(void) {
		// First add the (optional) provided sympath:
		TString Ret(OptSymPath);

		// Next append current working directory
		TString Path(STACKWALK_MAX_NAMELEN + 1, _T(';'));
		int PathLen = GetCurrentDirectory(STACKWALK_MAX_NAMELEN, (LPTSTR)Path.data() + 1);
		if (PathLen > 0) {
			Path.at(PathLen + 1) = _T('\0');
			Ret.append(Path.data() + (Ret.empty() ? 1 : 0));
		} else SYSERRLOG(_T("Unable to get current working directory"));

		// Now add the path for the main-module
		PathLen = GetModuleFileName(NULL, (LPTSTR)Path.data() + 1, STACKWALK_MAX_NAMELEN);
		if (PathLen > 0) {
			Path.at(PathLen + 1) = _T('\0');
			for (int Idx = PathLen; Idx > 1; Idx--) {
				// locate the rightmost path separator
				TCHAR At = Path.at(Idx);
				if ((At == _T('\\')) || (At == _T('/')) || (At == _T(':'))) {
					Path.at(Idx) = _T('\0');
					break;
				}
			}
			Ret.append(Path.data());
		} else SYSERRLOG(_T("Unable to get main module path"));

		// Now add known symbol path
		PathLen = GetEnvironmentVariable(_T("_NT_SYMBOL_PATH"), (LPTSTR)Path.data() + 1, STACKWALK_MAX_NAMELEN);
		if (PathLen > 0) Ret.append(Path.data());
		PathLen = GetEnvironmentVariable(_T("_NT_ALTERNATE_SYMBOL_PATH"), (LPTSTR)Path.data() + 1, STACKWALK_MAX_NAMELEN);
		if (PathLen > 0) Ret.append(Path.data());

		// Now add system root
		PathLen = GetEnvironmentVariable(_T("SYSTEMROOT"), (LPTSTR)Path.data() + 1, STACKWALK_MAX_NAMELEN);
		if (PathLen > 0) {
			Ret.append(Path.data());
			// Heuristically add "System32" directory
			Ret.append(Path.data()).append(_T("\\System32"));
		}

		// Add online symbol repository, if requested
		if (OnlineSymServer) {
			Ret.append(_T(";SRV*"));
			PathLen = GetEnvironmentVariable(_T("SYSTEMDRIVE"), (LPTSTR)Path.data(), STACKWALK_MAX_NAMELEN);
			Ret.append((PathLen > 0) ? Path.data() : _T("C:"));
			Ret.append(_T("\\symbols*https://msdl.microsoft.com/download/symbols"));
		}

		LOGVV(_T("* Symbol Search Path: %s"), Ret.c_str());
		return Ret;
	}

public:
	bool const OnlineSymServer;
	TString const OptSymPath;

	StackWalker_Impl(bool xOnlineSymServer, TString const &xOptSymPath) :
		StackWalker_Impl(THandle::Unmanaged(GetCurrentProcess()), xOnlineSymServer, xOptSymPath) {}

	StackWalker_Impl(THandle &&xProcess, bool xOnlineSymServer, TString const &xOptSymPath) :
		Process(std::move(xProcess)), OnlineSymServer(xOnlineSymServer), OptSymPath(xOptSymPath) {
		// Produce a derived symbol path
		TraceSymPath = GetSymPath();

		// Initialize symbols
		if (!SymInitializeT(*Process, TraceSymPath.c_str(), TRUE))
			SYSFAIL(_T("Unable to initialize symbol resolver"));

		// Configure symbol lookup
		DWORD symOptions = SymGetOptions();
		symOptions |= SYMOPT_LOAD_LINES;
		symOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
		symOptions |= SYMOPT_DEFERRED_LOADS;
		symOptions |= SYMOPT_NO_PROMPTS;
		//symOptions |= SYMOPT_DEBUG;
		symOptions = SymSetOptions(symOptions);
	}

	~StackWalker_Impl(void) override {
		SymCleanup(*Process);
	}

	bool Trace(THandle const &Thread, CONTEXT &Context, OnStackEntry const &EntryCallback, OnTraceError const &ErrorCallback) override {
		CallstackEntry csEntry;

		// init STACKFRAME for first call
		STACKFRAME stackFrame;
		memset(&stackFrame, 0, sizeof(stackFrame));
		DWORD imageType;

#ifdef _M_IX86
		// normally, call ImageNtHeader() and use machine info from PE header
		imageType = IMAGE_FILE_MACHINE_I386;
		stackFrame.AddrPC.Offset = Context.Eip;
		stackFrame.AddrPC.Mode = AddrModeFlat;
		stackFrame.AddrFrame.Offset = Context.Ebp;
		stackFrame.AddrFrame.Mode = AddrModeFlat;
		stackFrame.AddrStack.Offset = Context.Esp;
		stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
		imageType = IMAGE_FILE_MACHINE_AMD64;
		stackFrame.AddrPC.Offset = Context.Rip;
		stackFrame.AddrPC.Mode = AddrModeFlat;
		stackFrame.AddrFrame.Offset = Context.Rbp;
		stackFrame.AddrFrame.Mode = AddrModeFlat;
		stackFrame.AddrStack.Offset = Context.Rsp;
		stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
		imageType = IMAGE_FILE_MACHINE_IA64;
		stackFrame.AddrPC.Offset = Context.StIIP;
		stackFrames.AddrPC.Mode = AddrModeFlat;
		stackFrame.AddrFrame.Offset = Context.IntSp;
		stackFrame.AddrFrame.Mode = AddrModeFlat;
		stackFrame.AddrBStore.Offset = Context.RsBSP;
		stackFrame.AddrBStore.Mode = AddrModeFlat;
		stackFrame.AddrStack.Offset = Context.IntSp;
		stackFrame.AddrStack.Mode = AddrModeFlat;
#else
#error "Platform not supported!"
#endif

		SYMBOL_INFO_PACKAGE_T SymbolBuffer;
		SYMBOL_INFO_T &Symbol = SymbolBuffer.si;
		memset(&SymbolBuffer, 0, sizeof(SymbolBuffer));
		Symbol.SizeOfStruct = sizeof(SYMBOL_INFO_T);
		Symbol.MaxNameLen = MAX_SYM_NAME;

		IMAGEHLP_LINE_T Line;
		memset(&Line, 0, sizeof(Line));
		Line.SizeOfStruct = sizeof(Line);

		// We prepare enough buffer, but only ask for smaller structure
		TTypedBuffer<IMAGEHLP_MODULE_T> Module;
		memset(&Module, 0, sizeof(IMAGEHLP_MODULE_T));
		Module->SizeOfStruct = sizeof(IMAGEHLP_MODULE_V2_T);

		bool Continue = true;
		while (Continue) {
			// Get next stack frame (StackWalk64(), SymFunctionTableAccess64(), SymGetModuleBase64())
			if (!StackWalk(imageType, *Process, *Thread, &stackFrame, &Context, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL)) {
				// If this returns ERROR_INVALID_ADDRESS (487) or ERROR_NOACCESS (998), you can
				// assume that either you are done, or that the stack is so hosed that the next
				// deeper frame could not be found.
				if (GetLastError())
					Continue = ErrorCallback(_T("StackWalk"), GetLastError(), (PVOID)stackFrame.AddrPC.Offset);
				break;
			}

			// Initialize stack entry
			csEntry.Address = (PVOID)stackFrame.AddrPC.Offset;
			csEntry.ModuleBase = 0;
			csEntry.SmybolOffset = 0;
			csEntry.LineOffset = 0;
			csEntry.ModuleName.clear();
			csEntry.FileName.clear();
			csEntry.SymbolName.clear();
			csEntry.LineNumber = 0;

			// Basic sanify check
			if (stackFrame.AddrPC.Offset == stackFrame.AddrReturn.Offset) {
				Continue = ErrorCallback(_T("RetAddrCheck"), GetLastError(), (PVOID)stackFrame.AddrPC.Offset);
				// Invalid return / call address, proceed no further
				break;
			}

			// Do we have a valid PC?
			if (stackFrame.AddrPC.Offset != 0) {
				// Gather module info
				if (SymGetModuleInfoT(*Process, stackFrame.AddrPC.Offset, &Module)) {
					csEntry.SymbolType = (CallstackEntry::TSymbolType)Module->SymType;
					csEntry.ModuleName = Module->ModuleName;
					csEntry.ModuleBase = (PVOID)Module->BaseOfImage;
				} else {
					Continue = ErrorCallback(_T("SymGetModuleInfo"), GetLastError(), (PVOID)stackFrame.AddrPC.Offset);
					if (!Continue) break;
				}

				// Gather line info
				if (SymGetLineFromAddrT(*Process, stackFrame.AddrPC.Offset, &csEntry.LineOffset, &Line)) {
					csEntry.LineNumber = Line.LineNumber;
					csEntry.FileName.assign(__RelPath(Line.FileName));
				} else {
					Continue = ErrorCallback(_T("SymGetLineFromAddr"), GetLastError(), (PVOID)stackFrame.AddrPC.Offset);
					if (!Continue) break;
				}

				// Gather procedure info
				if (SymFromAddrT(*Process, stackFrame.AddrPC.Offset, &csEntry.SmybolOffset, &Symbol)) {
					csEntry.SymbolName.assign(STACKWALK_MAX_NAMELEN, _T('\0'));
					csEntry.SymbolName.resize(UnDecorateSymbolNameT(Symbol.Name, (LPTSTR)csEntry.SymbolName.data(), STACKWALK_MAX_NAMELEN, UNDNAME_NAME_ONLY));
					if (csEntry.SymbolName.empty())
						Continue = ErrorCallback(_T("UnDecorateSymbolName"), GetLastError(), (PVOID)stackFrame.AddrPC.Offset);
				} else
					Continue = ErrorCallback(_T("SymGetSymFromAddr"), GetLastError(), (PVOID)stackFrame.AddrPC.Offset);
				if (!Continue) break;

				Continue = EntryCallback(csEntry);
				SetLastError(ERROR_SUCCESS);
			} else
				Continue = ErrorCallback(_T("ProgCtrCheck"), GetLastError(), (PVOID)stackFrame.AddrPC.Offset);

			if (!Continue) break;
		}

		return Continue;
	}
};

typedef ManagedRef<TStackWalker> MRStackWalker;

TSyncObj<MRStackWalker>& _LocalStackWalker(bool xOnlineSymServer, TString const &xOptSymPath) {
	static TSyncObj<MRStackWalker> __IoFU(DEFAULT_NEW(StackWalker_Impl, xOnlineSymServer, xOptSymPath), CONSTRUCTION::HANDOFF);
	return __IoFU;
}

void LocalStackWalker_Init(bool xOnlineSymServer, TString const &xOptSymPath) {
	auto AMRSW = _LocalStackWalker(xOnlineSymServer, xOptSymPath).Pickup(); // Hold the Lock
	StackWalker_Impl* SW = static_cast<StackWalker_Impl*>(&*AMRSW); // Get Reference
	if ((SW == nullptr) || ((SW->OnlineSymServer != xOnlineSymServer) || (SW->OptSymPath.compare(xOptSymPath) != 0))) {
		if (SW == nullptr) {
			LOGV(_T("Initializing local stack walker..."));
		} else {
			LOGV(_T("Reconfiguring local stack walker..."));
		}
		*AMRSW = MRStackWalker(DEFAULT_NEW(StackWalker_Impl, xOnlineSymServer, xOptSymPath), CONSTRUCTION::HANDOFF);
	}
}

bool LocalStackTrace(THandle const &Thread, CONTEXT &Context,
					 TStackWalker::OnStackEntry const &EntryCallback,
					 TStackWalker::OnTraceError const &ErrorCallback) {
	auto AMRSW = _LocalStackWalker(true, EMPTY_TSTRING()).Pickup(); // Hold the Lock
	return (*AMRSW)->Trace(Thread, std::move(Context), EntryCallback, ErrorCallback);
}

#endif