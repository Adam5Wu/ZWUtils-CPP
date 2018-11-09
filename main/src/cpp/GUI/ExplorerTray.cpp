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

// [GUI] Explorer Tray Helper

#include "ExplorerTray.h"

#ifdef WINDOWS

#include "Misc/Types.h"
#include "Misc/TString.h"
#include "Threading/WorkerThread.h"

#include <vector>

#define EXPLORERTRAY_IDBASE		1000
#define EXPLORERTRAY_WMID		WM_USER
#define EXPLORERTRAY_CLASSNAME	"__EXPTRAY_CLS__"

class TExplorerTray : public TRunnable {
protected:
	TIcon _Icon;
	TString _ToolTip;
	std::vector<TMenuItem> _MenuItems;

	TWindow _Window;
	TPopupMenu _PopupMenu;
	UINT32 _TrayID;

	bool _PollMessage;

	static TInterlockedOrdinal32<UINT32> _InstCnt;

	void InitializeTray(void) {
		_Window = { [&] {
			HWND hWnd = CreateWindowEx(0, _T(EXPLORERTRAY_CLASSNAME), _T(EXPLORERTRAY_CLASSNAME),
									   0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, this);
			if (hWnd == NULL)
				SYSFAIL(_T("Unable to create explorer tray window instance"));
			return hWnd;
		} };

		NOTIFYICONDATA notifyIconData = { 0 };
		notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
		notifyIconData.hWnd = *_Window;
		notifyIconData.uID = EXPLORERTRAY_IDBASE + _TrayID;
		notifyIconData.uCallbackMessage = EXPLORERTRAY_WMID;
		notifyIconData.hIcon = *_Icon;
		notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE;
		if (!_ToolTip.empty()) {
			notifyIconData.uFlags |= NIF_TIP;
			_tcscpy_s(notifyIconData.szTip, 128, _ToolTip.c_str());
		}

		if (!_MenuItems.empty()) {
			for (size_t idx = 0; idx < _MenuItems.size(); idx++) {
				_PopupMenu.AddMenuItem(idx, _MenuItems[idx].DispText);
			}
		}

		if (!Shell_NotifyIcon(NIM_ADD, &notifyIconData)) {
			SYSFAIL(_T("Failed to add tray icon"));
		}
	}

	void FinalizeTray(void) {
		if (_Window.Allocated()) {
			NOTIFYICONDATA notifyIconData = { 0 };
			notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
			notifyIconData.hWnd = *_Window;
			notifyIconData.uID = EXPLORERTRAY_IDBASE + _TrayID;
			if (!Shell_NotifyIcon(NIM_DELETE, &notifyIconData)) {
				SYSERRLOG(_T("WARNING: Failed to remove tray icon"));
			}

			_PopupMenu.Clear();
			_Window.Clear();
		}
	}

public:
	TString const Name;

	TExplorerTray(TString const &xName, TIcon &&Icon, TString const &ToolTip, TMenuItems && MenuItems)
		: Name(xName), _Icon(std::move(Icon)), _ToolTip(ToolTip), _MenuItems(std::move(MenuItems))
		, _Window(NULL, TResource<HWND>::NullDealloc), _TrayID(++_InstCnt)
	{}

	virtual TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) override {
		TInitResource<int> Tray_RAII(0, [&](int&) { FinalizeTray(); });
		InitializeTray();

		MSG messages;
		while (_PollMessage && (WorkerThread.CurrentState() == TWorkerThread::State::Running)) {
			if (GetMessage(&messages, NULL, 0, 0)) {
				TranslateMessage(&messages);
				DispatchMessage(&messages);
			}
		}

		return { nullptr };
	}

	virtual void StopNotify(TWorkerThread &WorkerThread) override {
		PostMessage(*_Window, WM_CLOSE, 0, 0);
	}

	LRESULT __WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg) {
			case WM_CLOSE:
				LOGV(_T("Tray removal message received!"));
				_PollMessage = false;
				return 0;

			case EXPLORERTRAY_WMID:
				LOGVV(_T("Tray activity message received! (%d:%d)"), wParam, lParam);

				if ((lParam == WM_RBUTTONDOWN) && !_MenuItems.empty()) {
					// Get current mouse position.
					POINT curPoint;
					GetCursorPos(&curPoint);

					LOGV(_T("Popup menu triggered, tracking..."));
					// TrackPopupMenu blocks the app until TrackPopupMenu returns
					UINT clicked = TrackPopupMenu(*_PopupMenu, TPM_RETURNCMD | TPM_NONOTIFY,
												  curPoint.x, curPoint.y, 0, hwnd, NULL);

					// Send benign message to window to make sure the menu goes away.
					SendMessage(hwnd, WM_NULL, 0, 0);

					auto &MenuItem = _MenuItems[clicked];
					LOGV(_T("Popup menu selected '%s'"), MenuItem.DispText.c_str());
					if (MenuItem.Callback) {
						LOGV(_T("Processing callback..."));
						MenuItem.Callback();
					}
				}
				return 0;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
};

TInterlockedOrdinal32<UINT32> TExplorerTray::_InstCnt(0);

static LRESULT CALLBACK __CallForwarder(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	LONG_PTR Inst;

	if (msg == WM_NCCREATE) {
		Inst = reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);

		SetLastError(0);
		if (!SetWindowLongPtr(hwnd, GWLP_USERDATA, Inst)) {
			if (GetLastError() != 0) {
				SYSERRLOG(_T("Failed to store class instance pointer"));
				return FALSE;
			}
		}
	} else {
		Inst = GetWindowLongPtr(hwnd, GWLP_USERDATA);
	}

	return reinterpret_cast<TExplorerTray*>(Inst)->__WndProc(hwnd, msg, wParam, lParam);
}

MRWorkerThread CreateExplorerTray(TString const &Name, TIcon &&Icon, TString const &ToolTip, TMenuItems && MenuItems) {
	return {
		CONSTRUCTION::EMPLACE, Name,
		MRRunnable(DEFAULT_NEW(TExplorerTray, Name, std::move(Icon), ToolTip, std::move(MenuItems)),
				   CONSTRUCTION::HANDOFF)
	};
}

static ATOM ExplorerTray_WCls = 0;

void ExplorerTray_GlobalInit(HINSTANCE hInstance) {
	if (ExplorerTray_WCls) {
		FAIL(_T("Explorer tray utility already initialized"));
	}

	WNDCLASSEX winclass = { 0 };
	winclass.cbSize = sizeof(WNDCLASSEX);
	winclass.hInstance = hInstance;
	winclass.lpszClassName = _T(EXPLORERTRAY_CLASSNAME);
	winclass.lpfnWndProc = __CallForwarder;

	ExplorerTray_WCls = RegisterClassEx(&winclass);
	if (!ExplorerTray_WCls) {
		SYSFAIL(_T("Unable to register explorer tray window class"));
	}
}

void ExplorerTray_GlobalFInit(void) {
	if (ExplorerTray_WCls) {
		UnregisterClass((LPCTSTR)ExplorerTray_WCls, NULL);
	}
}

#endif