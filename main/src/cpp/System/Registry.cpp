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

// [System] Registry access

#include "Registry.h"

#include "Debug/Logging.h"
#include "Debug/Exception.h"
#include "Debug/SysError.h"

#include "Memory/Resource.h"

#ifdef WINDOWS

static HKEY OpenRegistry(HKEY RegBase, LPCTSTR Name, REGSAM samDesired, bool createIfNeeded) {
	HKEY hKey;
	DWORD Result = RegOpenKeyEx(RegBase, Name, 0, samDesired, &hKey);
	if (Result != ERROR_SUCCESS) {
		if ((Result == ERROR_FILE_NOT_FOUND) && createIfNeeded) {
			LOGVV(_T("Creating new registry key '%s'"), Name);
			Result = RegCreateKeyEx(RegBase, Name, 0, NULL, 0, samDesired, NULL, &hKey, NULL);
			if (Result != ERROR_SUCCESS)
				SYSERRFAIL(Result, _T("Unable to create registry key"));
		} else
			SYSERRFAIL(Result, _T("Unable to open registry key"));
	}
	return hKey;
}

TRegistry::TRegistry(TRegistry const &Base, TString const &Name, REGSAM samDesired, bool createIfNeeded) :
	TAllocResource(OpenRegistry(*Base, Name.c_str(), samDesired, createIfNeeded), HandleDealloc_Standard)
{}

INT32 TRegistry::GetInt32(TString const &Name, INT32 Default) {
	INT32 Ret = 0;
	DWORD DataSize = 4;
	DWORD Result = RegQueryValueEx(Refer(), Name.c_str(), 0, nullptr, (BYTE*)&Ret, &DataSize);
	if (Result != ERROR_SUCCESS) {
		if ((Result == ERROR_FILE_NOT_FOUND) || (Result == ERROR_PATH_NOT_FOUND)) {
			LOGVV(_T("Value '%s' does not exist, using default value: 0x%08X (%d)"), Name.c_str(), Default, Default);
			return Default;
		}
		SYSFAIL(_T("Failed to query registry value '%s'"), Name.c_str());
	}

	if (DataSize != 4)
		FAIL(_T("Unexpected data size for '%s'"), Name.c_str());

	return Ret;
}

INT64 TRegistry::GetInt64(TString const &Name, INT64 Default) {
	INT64 Ret = 0;
	DWORD DataSize = 8;
	DWORD Result = RegQueryValueEx(Refer(), Name.c_str(), 0, nullptr, (BYTE*)&Ret, &DataSize);
	if (Result != ERROR_SUCCESS) {
		if ((Result == ERROR_FILE_NOT_FOUND) || (Result == ERROR_PATH_NOT_FOUND)) {
			LOGV(_T("Value '%s' does not exist, using default value: 0x%016llX (%lld)"), Name.c_str(), Default, Default);
			return Default;
		}
		SYSFAIL(_T("Failed to query registry value '%s'"), Name.c_str());
	}

	if (DataSize != 8)
		FAIL(_T("Unexpected data size for '%s'"), Name.c_str());

	return Ret;
}

TString TRegistry::GetString(TString const &Name, TString const &Default) {
	DWORD DataSize = 0;
	DWORD Result = RegQueryValueEx(Refer(), Name.c_str(), 0, nullptr, nullptr, &DataSize);
	if (Result != ERROR_SUCCESS) {
		if ((Result == ERROR_FILE_NOT_FOUND) || (Result == ERROR_PATH_NOT_FOUND)) {
			LOGV(_T("Value '%s' does not exist, using default value: '%s'"), Name.c_str(), Default.c_str());
			return Default;
		}
		SYSFAIL(_T("Failed to query registry data '%s'"), Name.c_str());
	}

	if ((DataSize & 1) != 0)
		FAIL(_T("Invalid string size %d for data '%s'"), DataSize, Name.c_str());

	TString Ret(DataSize / 2, NullWChar);
	Result = RegQueryValueEx(Refer(), Name.c_str(), 0, nullptr, (BYTE*)&Ret.front(), &DataSize);
	if (Result != ERROR_SUCCESS) {
		SYSFAIL(_T("Failed to retrieve registry data '%s'"), Name.c_str());
	}

	while (!Ret.empty() && (*--Ret.end() == '\0')) Ret.pop_back();
	return Ret;
}

void TRegistry::SetInt32(TString const &Name, INT32 Value) {
	DWORD Result = RegSetValueEx(Refer(), Name.c_str(), 0, REG_DWORD, (BYTE*)&Value, 4);
	if (Result != ERROR_SUCCESS) {
		SYSFAIL(_T("Failed to store registry data '%s'"), Name.c_str());
	}
}

void TRegistry::SetInt64(TString const &Name, INT64 Value) {
	DWORD Result = RegSetValueEx(Refer(), Name.c_str(), 0, REG_QWORD, (BYTE*)&Value, 8);
	if (Result != ERROR_SUCCESS) {
		SYSFAIL(_T("Failed to store registry data '%s'"), Name.c_str());
	}
}

void TRegistry::SetString(TString const &Name, TString const &Value) {
	DWORD Result = RegSetValueEx(Refer(), Name.c_str(), 0, REG_SZ, (BYTE const*)Value.c_str(), (DWORD)Value.length() * sizeof(TCHAR));
	if (Result != ERROR_SUCCESS) {
		SYSFAIL(_T("Failed to store registry data '%s'"), Name.c_str());
	}
}

#endif //WINDOWS
