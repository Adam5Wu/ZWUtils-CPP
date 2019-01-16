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

/**
 * @addtogroup GUI Graphic User Interface Utilities
 * @file
 * @brief Elemental GUI Resources
 * @author Zhenyu Wu
 * @date Nov 09, 2018: Initial Implementation
 **/

#ifndef ZWUtils_GUITypes_H
#define ZWUtils_GUITypes_H

 // Project global control
#include "Misc/Global.h"

#ifdef WINDOWS

#include "Debug/SysError.h"

#include "Memory/Resource.h"

#include "System/SysTypes.h"

#include <Windows.h>
#include <shellapi.h>

#include <functional>
#include <vector>

struct TMenuItem {
	TString DispText;
	std::function<void(void)> Callback;
};
typedef std::vector<TMenuItem> TMenuItems;

class TIcon : public TAllocResource<HICON> {
protected:
	static void FreeIconResource(HICON &X) {
		if (!DestroyIcon(X))
			SYSFAIL(_T("Unable to destroy icon resource"));
	}

public:
	TIcon(TModule const &Module, LPCTSTR Name);
	TIcon(TModule const &Module, LPCTSTR Name, int cx, int cy);

	TIcon(CONSTRUCTION::HANDOFF_T const &, HICON const &hIcon, TResDealloc const &xDealloc = FreeIconResource, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(hIcon, xDealloc, xAlloc) {}

	static TIcon Unmanaged(HICON const &hIcon)
	{ return { CONSTRUCTION::HANDOFF, hIcon, NullDealloc }; }
};

class TWindow : public TAllocResource<HWND> {
public:
	static void Dealloc_HWND(HWND &hWND) {
		if (!DestroyWindow(hWND)) {
			SYSFAIL(_T("Fail to destroy window instance"));
		}
	}

	TWindow(TResAlloc const &xAlloc, TResDealloc const &xDealloc = Dealloc_HWND) :
		TAllocResource(xAlloc, xDealloc) {}

	TWindow(CONSTRUCTION::HANDOFF_T const &, HWND const &hWND, TResDealloc const &xDealloc = Dealloc_HWND, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(hWND, xDealloc, xAlloc) {}

	static TWindow Unmanaged(HWND const &hWND)
	{ return { CONSTRUCTION::HANDOFF, hWND, NullDealloc }; }
};

class TMenu : public TAllocResource<HMENU> {
public:
	static void Dealloc_HMENU(HMENU &hMENU) {
		if (!DestroyMenu(hMENU)) {
			SYSFAIL(_T("Fail to destroy window instance"));
		}
	}

	TMenu(TResAlloc const &xAlloc, TResDealloc const &xDealloc = Dealloc_HMENU) :
		TAllocResource(xAlloc, xDealloc) {}

	TMenu(CONSTRUCTION::HANDOFF_T const &, HMENU const &hMENU, TResDealloc const &xDealloc = Dealloc_HMENU, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(hMENU, xDealloc, xAlloc) {}

	void AddMenuItem(size_t index, TString const & DispText);

	static TMenu Unmanaged(HMENU const &hMENU)
	{ return { CONSTRUCTION::HANDOFF, hMENU, NullDealloc }; }
};

class TPopupMenu : public TMenu {
public:
	using TMenu::TMenu;

	TPopupMenu(TResDealloc const &xDealloc = Dealloc_HMENU);
};

class TDC : public TAllocResource<HDC> {
protected:
	static void FreeDCResource(HWND WND, HDC &X) {
		if (!ReleaseDC(WND, X))
			SYSFAIL(_T("Unable to release DC resource"));
	}

public:
	TDC(TWindow &Window);
	TDC(CONSTRUCTION::DEFER_T &, TWindow &Window);

	TDC(CONSTRUCTION::HANDOFF_T const &, HDC const &hDC, HWND const &hWND) :
		TAllocResource(hDC, hWND ? (TResDealloc)std::bind(FreeDCResource, hWND, std::placeholders::_1) : NullDealloc, NoAlloc) {}

	static TDC Unmanaged(HDC const &hDC)
	{ return { CONSTRUCTION::HANDOFF, hDC, NULL }; }
};

#endif

#endif //ZWUtils_GUITypes_H