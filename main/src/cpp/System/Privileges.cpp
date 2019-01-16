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

// [System] Privilege access

#include "Privileges.h"

static THandle &__ProcessAdjustToken(void) {
	static THandle __IoFU(
		[] {
			HANDLE hToken = nullptr;
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken)) {
				SYSFAIL(_T("Could not open current process token"));
			}
			return hToken;
		});
	return __IoFU;
}

static LUID __LookupPrivilege(LPCTSTR lpszPrivilege) {
	LUID PrivID;
	if (!LookupPrivilegeValue(nullptr, lpszPrivilege, &PrivID)) {
		SYSFAIL(_T("Failed to lookup privilege ID for '%s'"), lpszPrivilege);
	}
	return PrivID;
}

static DWORD __SetPrivilege(HANDLE hToken, LUID const &PrivID, DWORD State) {
	TOKEN_PRIVILEGES TP;

	TP.PrivilegeCount = 1;
	TP.Privileges[0].Luid = PrivID;
	TP.Privileges[0].Attributes = State;

	TOKEN_PRIVILEGES PrevTP;
	DWORD PrevSize = sizeof(TOKEN_PRIVILEGES);

	if (!AdjustTokenPrivileges(hToken, FALSE, &TP, PrevSize, &PrevTP, &PrevSize)) {
		SYSFAIL(_T("Failed to adjust token privileges"));
	}

	// The function may succeed if not all privileges were adjusted
	DWORD ErrCode = GetLastError();
	if (ErrCode != ERROR_SUCCESS) {
		SYSERRFAIL(ErrCode, _T("Error adjusting token privileges"));
	}
	return PrevTP.Privileges[0].Attributes;
}

TPrivilege::TPrivilege(TString const &xName, bool Enable) :
	TPrivilege(__ProcessAdjustToken(), xName, Enable)
{}

TPrivilege::TPrivilege(THandle const &hTokenAdjust, TString const &xName, bool Enable) :
	TAllocResource(NoAlloc, NoDealloc), Name(xName), PrivID(__LookupPrivilege(xName.c_str()))
{
	_Dealloc = [&, PrivID = this->PrivID](DWORD &X) { __SetPrivilege(*hTokenAdjust, PrivID, X); };
	try {
		_ResRef = __SetPrivilege(*hTokenAdjust, PrivID, Enable ? SE_PRIVILEGE_ENABLED : 0);
		_ResValid = true;
	} catch (_ECR_ e) {
		FAIL(_T("Unable to acquire privilege '%s' - %s"), Name.c_str(), e.Why().c_str());
	}
}
