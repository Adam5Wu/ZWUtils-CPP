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

#ifndef ZWUtils_GUIResources_H
#define ZWUtils_GUIResources_H

#include "Misc/Global.h"

#ifdef WINDOWS

#include "Misc/Types.h"
#include "Debug/SysError.h"
#include "Memory/Resource.h"

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
	static void FreeResource(HICON &X) {
		if (!DestroyIcon(X))
			SYSFAIL(_T("Unable to destroy icon resource"));
	}

	static bool IsIntResource(LPCTSTR value) {
		return (__ARC_UINT)value <= 0xFFFF;
	}

	static HICON __LoadIcon(HMODULE Module, LPCTSTR Name) {
		HICON Ret = LoadIcon(Module, Name);
		if (Ret == NULL) {
			if (IsIntResource(Name)) {
				SYSFAIL(_T("Unable to load icon resource #%d"), (__ARC_UINT)Name);
			} else {
				SYSFAIL(_T("Unable to load icon resource '%s'"), Name);
			}
		}
		return Ret;
	}

public:
	TIcon(TModule const &Module, LPCTSTR Name) :
		TAllocResource(__LoadIcon(*Module, Name), FreeResource)
	{}
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

	TWindow(HWND const &hWND, TResDealloc const &xDealloc = Dealloc_HWND, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(hWND, xDealloc, xAlloc) {}
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

	TMenu(HMENU const &hMENU, TResDealloc const &xDealloc = Dealloc_HMENU, TResAlloc const &xAlloc = NoAlloc) :
		TAllocResource(hMENU, xDealloc, xAlloc) {}

	void AddMenuItem(size_t index, TString const & DispText);
};

class TPopupMenu : public TMenu {
public:
	static HMENU Alloc_PopupMenu(void) {
		HMENU hMenu = CreatePopupMenu();
		if (hMenu == NULL)
			SYSFAIL(_T("Unable to create popup menu instance"));
		return hMenu;
	}

	using TMenu::TMenu;

	TPopupMenu(TResAlloc const &xAlloc = Alloc_PopupMenu, TResDealloc const &xDealloc = Dealloc_HMENU) :
		TMenu(xAlloc, xDealloc) {}
};

#endif

#endif //ZWUtils_GUIResources_H