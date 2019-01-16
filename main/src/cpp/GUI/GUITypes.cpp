/*
Copyright (c) 2005 - 2018, Zhenyu Wu; 2012 - 2018, NEC Labs America Inc.
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

// [GUI] Elemental GUI Resources

#include "GUITypes.h"

#include "Debug/SysError.h"

#include <Windows.h>
#include <shellapi.h>

#define DISP_NEMU_HBAR	"---"
#define DISP_NEMU_VBAR	"|||"

#ifdef WINDOWS

static HICON __LoadIcon(HMODULE Module, LPCTSTR Name) {
	HICON Ret = LoadIcon(Module, Name);
	if (Ret == NULL) {
		if (IS_INTRESOURCE(Name)) {
			SYSFAIL(_T("Unable to load icon resource #%d"), (__ARC_UINT)Name);
		} else {
			SYSFAIL(_T("Unable to load icon resource '%s'"), Name);
		}
	}
	return Ret;
}

static HICON __LoadIconImage(HMODULE Module, LPCTSTR Name, int cx, int cy) {
	UINT LoadFlags = 0;
	if (cx * cy == 0)  LoadFlags |= LR_DEFAULTSIZE;
	if (!IS_INTRESOURCE(Name)) LoadFlags |= LR_LOADFROMFILE;
	HANDLE Ret = LoadImage(Module, Name, IMAGE_ICON, cx, cy, LoadFlags);
	if (Ret == NULL) {
		if (IS_INTRESOURCE(Name)) {
			SYSFAIL(_T("Unable to load icon resource #%d (%dx%x%d)"), (__ARC_UINT)Name, cx, cy);
		} else {
			SYSFAIL(_T("Unable to load icon resource '%s' (%dx%x%d)"), Name, cx, cy);
		}
	}
	return (HICON)Ret;
}

TIcon::TIcon(TModule const &Module, LPCTSTR Name) :
	TAllocResource(__LoadIcon(*Module, Name), FreeIconResource)
{}

TIcon::TIcon(TModule const &Module, LPCTSTR Name, int cx, int cy) :
	TAllocResource(__LoadIconImage(*Module, Name, cx, cy), FreeIconResource)
{}


void TMenu::AddMenuItem(size_t index, TString const & DispText) {
	BOOL iRet;
	if (DispText == _T(DISP_NEMU_HBAR)) {
		iRet = AppendMenu(Refer(), MF_MENUBREAK, index, NULL);
	} else if (DispText == _T(DISP_NEMU_VBAR)) {
		iRet = AppendMenu(Refer(), MF_MENUBARBREAK, index, NULL);
	} else {
		iRet = AppendMenu(Refer(), MF_STRING, index, DispText.c_str());
	}
	if (!iRet) {
		SYSFAIL(_T("Failed to append menu item '%s'"), DispText.c_str());
	}
}


static HMENU __Alloc_PopupMenu(void) {
	HMENU hMenu = CreatePopupMenu();
	if (hMenu == NULL)
		SYSFAIL(_T("Unable to create popup menu instance"));
	return hMenu;
}

TPopupMenu::TPopupMenu(TResDealloc const &xDealloc) :
	TMenu(__Alloc_PopupMenu, xDealloc)
{}


static HDC __AllocDCResource(HWND WND) {
	HDC Ret = GetDC(WND);
	if (Ret == NULL) {
		SYSFAIL(_T("Failed to allocate DC resource"));
	}
	return Ret;
}

TDC::TDC(TWindow &Window) :
	TAllocResource(__AllocDCResource(*Window),
				   std::bind(FreeDCResource, *Window, std::placeholders::_1))
{}

TDC::TDC(CONSTRUCTION::DEFER_T &, TWindow &Window) :
	TAllocResource(std::bind(__AllocDCResource, *Window),
				   std::bind(FreeDCResource, *Window, std::placeholders::_1))
{}

#endif