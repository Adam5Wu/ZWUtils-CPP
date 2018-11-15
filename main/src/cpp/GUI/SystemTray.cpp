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

// [GUI] System Tray Helper

#include "SystemTray.h"

#ifdef WINDOWS

#include "Misc/Types.h"
#include "Misc/TString.h"
#include "Threading/WorkerThread.h"

#include <vector>

#include <windowsx.h>

#define EXPLORERTRAY_IDBASE		1000
#define EXPLORERTRAY_WMID		WM_USER
#define EXPLORERTRAY_CLASSNAME	"__EXPTRAY_CLS__"

class TSystemTray : public TRunnable {
private:
	TIcon _Icon;
	TString _ToolTip;
	std::vector<TMenuItem> _MenuItems;

	TWindow _Window;
	TPopupMenu _PopupMenu;
	UINT32 _TrayID;

	bool _PollMessage;
	bool _CallbackWIP;

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
		notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_SHOWTIP;
		notifyIconData.hIcon = *_Icon;
		notifyIconData.uCallbackMessage = EXPLORERTRAY_WMID;
		notifyIconData.uVersion = NOTIFYICON_VERSION_4;
		if (!_ToolTip.empty()) {
			notifyIconData.uFlags |= NIF_TIP;
			_tcscpy_s(notifyIconData.szTip, 128, _ToolTip.c_str());
		}

		if (!_MenuItems.empty()) {
			for (size_t idx = 0; idx < _MenuItems.size(); idx++) {
				if (!_MenuItems[idx].DispText.empty()) {
					_PopupMenu.AddMenuItem(idx + 1, _MenuItems[idx].DispText);
					if (idx == 0) {
						SetMenuDefaultItem(*_PopupMenu, (UINT)idx + 1, FALSE);
					}
				}
			}
		}

		if (!Shell_NotifyIcon(NIM_ADD, &notifyIconData)) {
			SYSFAIL(_T("Failed to add tray icon"));
		}
		if (!Shell_NotifyIcon(NIM_SETVERSION, &notifyIconData)) {
			SYSFAIL(_T("Failed to set tray icon version"));
		}
		_PollMessage = true;
		_CallbackWIP = false;
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

	TSystemTray(TString const &xName, TIcon &&Icon, TString const &ToolTip, TMenuItems && MenuItems)
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

		return {};
	}

	virtual void StopNotify(TWorkerThread &WorkerThread) override {
		PostMessage(*_Window, WM_CLOSE, 0, 0);
	}

	void __MakeCallback(std::function<void(void)> FCallback) {
		if (FCallback) {
			LOGV(_T("Processing callback..."));
			_CallbackWIP = true;
			FCallback();
			_CallbackWIP = false;
		}
	}

	LRESULT __WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg) {
			case WM_CLOSE:
				LOGV(_T("Tray removal message received!"));
				_PollMessage = false;
				return 0;

			case EXPLORERTRAY_WMID:
				LOGVV(_T("Tray activity message received! (%d / %X)"), LOWORD(lParam), LOWORD(lParam));

				switch (LOWORD(lParam)) {
					case WM_CONTEXTMENU:
						if (!_MenuItems.empty() && !_CallbackWIP) {
							SetForegroundWindow(hwnd);
							LOGV(_T("Context menu triggered, tracking..."));
							// TrackPopupMenu blocks until TrackPopupMenu returns
							UINT clicked = TrackPopupMenuEx(*_PopupMenu, TPM_RETURNCMD | TPM_NONOTIFY,
															GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam),
															hwnd, NULL);

							if (clicked) {
								auto &MenuItem = _MenuItems[clicked - 1];
								LOGV(_T("Context menu selected '%s'"), MenuItem.DispText.c_str());
								__MakeCallback(MenuItem.Callback);
							} else {
								LOGV(_T("Context menu cancelled"));
							}
						}
						break;

					case WM_LBUTTONDBLCLK:
						if (!_MenuItems.empty() && !_CallbackWIP) {
							auto &MenuItem = _MenuItems[0];
							LOGV(_T("Double click triggers context menu '%s'"), MenuItem.DispText.c_str());
							__MakeCallback(MenuItem.Callback);
						}
				}
				return 0;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
};

TInterlockedOrdinal32<UINT32> TSystemTray::_InstCnt(0);

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

	return reinterpret_cast<TSystemTray*>(Inst)->__WndProc(hwnd, msg, wParam, lParam);
}

MRWorkerThread CreateSystemTray(TString const &Name, TIcon &&Icon, TString const &ToolTip, TMenuItems && MenuItems) {
	return {
		CONSTRUCTION::EMPLACE, Name,
		MRRunnable(DEFAULT_NEW(TSystemTray, Name, std::move(Icon), ToolTip, std::move(MenuItems)),
				   CONSTRUCTION::HANDOFF)
	};
}

static ATOM SystemTray_WCls = 0;

void SystemTray_GlobalInit(HINSTANCE hInstance) {
	if (SystemTray_WCls) {
		FAIL(_T("System tray utility already initialized"));
	}

	WNDCLASSEX winclass = { 0 };
	winclass.cbSize = sizeof(WNDCLASSEX);
	winclass.hInstance = hInstance;
	winclass.lpszClassName = _T(EXPLORERTRAY_CLASSNAME);
	winclass.lpfnWndProc = __CallForwarder;

	SystemTray_WCls = RegisterClassEx(&winclass);
	if (!SystemTray_WCls) {
		SYSFAIL(_T("Unable to register explorer tray window class"));
	}
}

void SystemTray_GlobalFInit(void) {
	if (SystemTray_WCls) {
		UnregisterClass((LPCTSTR)SystemTray_WCls, NULL);
	}
}

#endif