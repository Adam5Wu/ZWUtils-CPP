/*
Copyright (c) 2005 - 2019, Zhenyu Wu; 2012 - 2019, NEC Labs America Inc.
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

// [System] Resource resolution

#include "SysRes.h"

#include "Misc/Types.h"

#include "Memory/Resource.h"

#include "Debug/SysError.h"

#include <iomanip>

#ifdef WINDOWS

#pragma comment(lib, "version")

TModule LoadResourceModule(TString const &ModuleName) {
	auto ExModule = GetModuleHandle(ModuleName.c_str());
	return ExModule ? TModule(CONSTRUCTION::HANDOFF, ExModule, TModule::NullDealloc)
		: TModule(
			[&] {
				HMODULE Ret = LoadLibraryEx(ModuleName.c_str(), NULL,
											LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
				if (!Ret) SYSFAIL(_T("Unable to load module '%s' for resource reading"), ModuleName.c_str());
				return Ret;
			});
}

TDynBuffer GetVersionResource(TString const &ResKey, TString const &ModuleName) {
	TModule RedModule = LoadResourceModule(ModuleName);
	return GetVersionResource(ResKey, RedModule);
}

TDynBuffer LoadVersionResource(TModule const &Module) {
	HRSRC ResHandle = FindResource(*Module, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
	if (!ResHandle) SYSFAIL(_T("Could not find version resource"));

	DWORD ResSize = SizeofResource(*Module, ResHandle);
	if (!ResSize) SYSFAIL(_T("Failed to query version resource size"));

	HGLOBAL ResEntry = LoadResource(*Module, ResHandle);
	if (!ResEntry) SYSFAIL(_T("Failed to load version resource entry"));

	LPVOID ResData = LockResource(ResEntry);
	if (!ResEntry) SYSFAIL(_T("Failed to lock version resource data"));

	// Make a copy of the resource (required by VerQueryValue, plus we can release the module afterwards)
	TDynBuffer Ret(ResSize);
	memcpy_s(*Ret, ResSize, ResData, ResSize);
	return Ret;
}

TDynBuffer GetVersionResource(TString const &ResKey, TModule const &Module) {
	TDynBuffer VerResData = LoadVersionResource(Module);
	return GetVersionResource(ResKey, VerResData);
}

static TString const __VERSION_STRINGFILEINFO = _T("\\StringFileInfo\\");

TDynBuffer GetVersionResource(TString const &ResKey, TDynBuffer const &VerResData) {
	PBYTE VerData;
	UINT VerDataLen;
	if (!VerQueryValue(*VerResData, ResKey.c_str(), (LPVOID*)&VerData, &VerDataLen))
		SYSFAIL(_T("Could not fetch specified info from version resource"));
	if (ResKey.find(__VERSION_STRINGFILEINFO, 0) == 0) VerDataLen *= sizeof(TCHAR);

	return { VerData, VerDataLen, DummyAllocator() };
}

static TString const __VERSION_VARFILEINFO_TRANSLATION = _T("\\VarFileInfo\\Translation");

TTypedBuffer<LANGANDCODEPAGE> GetVersionTranslations(TDynBuffer const &ResData) {
	auto TranslationBuf = GetVersionResource(_T("\\VarFileInfo\\Translation"), ResData);
	if (TranslationBuf.GetSize() % sizeof(LANGANDCODEPAGE))
		FAIL(_T("Unexpected version info translation data size (%d, expect N*%d)"),
			 TranslationBuf.GetSize(), sizeof(LANGANDCODEPAGE));
	return TTypedBuffer<LANGANDCODEPAGE>(*(LANGANDCODEPAGE*)&TranslationBuf, TranslationBuf.GetSize(), DummyAllocator());
}

TString ExtractVersionString(TDynBuffer const &ResData, LANGANDCODEPAGE const& LCData, TString const &ResKey) {
	TString FileInfoPfx = TStringCast(__VERSION_STRINGFILEINFO << std::hex << std::setfill(_T('0')) <<
									  std::setw(4) << LCData.wLanguage << std::setw(4) << LCData.wCodePage << _T('\\'));

	auto StrBuf = GetVersionResource(FileInfoPfx + ResKey, ResData);
	if (StrBuf.GetSize() % sizeof(TCHAR)) {
		LOGV(_T("WARNING: Unaligned string resource (length %d)"), StrBuf.GetSize());
	}
	TTypedBuffer<TCHAR> CharBuf(*(TCHAR*)&StrBuf, StrBuf.GetSize(), DummyAllocator());
	size_t StrLen = StrBuf.GetSize() / sizeof(TCHAR);
	while (StrLen && !CharBuf[--StrLen]);
	return TString(&CharBuf, StrLen + 1);
}

#endif //WINDOWS