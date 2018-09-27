
// [AdvUtils] Generic System Service Interface

#include "Misc/Global.h"

#ifdef WINDOWS

#include "ServiceMain.h"

#include "Misc/Timing.h"
#include "Debug/Logging.h"
#include "Debug/Exception.h"
#include "Debug/SysError.h"
#include "Memory/Resource.h"

#include <Windows.h>
#include <KtmW32.h>

#include <algorithm>

#pragma comment(lib, "KtmW32.lib")

//---------------------------------------
// ServiceCtrl General Maintenance Code

static HMODULE GetCurrentModuleHandle() {
	HMODULE hMod = NULL;
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCWSTR>(&GetCurrentModuleHandle),
		&hMod)) {
		SYSFAIL(_T("Failed to query current module handle"));
	}
	return hMod;
}

static LPCTSTR _SERVICE_HOST_CMD = _T("%SystemRoot%\\system32\\svchost.exe -k ");

typedef TInitResource<SC_HANDLE> TSCHandle;
typedef TInitResource<HKEY> THKEY;

static void InstallAsService(LPCTSTR ServiceName, LPCTSTR ServiceDispName, LPCTSTR ServiceDesc, LPCTSTR ServiceGrp,
					  bool AutoStart, LPCTSTR DependOn = nullptr, LPCTSTR ServiceDLL = nullptr,
					  int CrashRestart = 0, LPCTSTR RunAccount = nullptr, LPCTSTR RunPassword = nullptr,
					  LPCTSTR Privileges = nullptr, DWORD ServiceSecurity = SERVICE_SID_TYPE_NONE,
					  DWORD RestartDelay = Convert(1, TimeUnit::MIN, TimeUnit::MSEC)) {
	SC_HANDLE _HANDLE = OpenSCManager(nullptr, nullptr, GENERIC_WRITE);
	if (_HANDLE == nullptr)
		SYSFAIL(_T("Unable to open service control manager"));

	{
		TSCHandle SCMHandle(_HANDLE, [](SC_HANDLE &X) {CloseServiceHandle(X); });

		TCHAR LibraryPath[MAX_PATH];
		if (!ServiceDLL) {
			ServiceDLL = LibraryPath;
			DWORD BufSize = GetModuleFileName(GetCurrentModuleHandle(), LibraryPath, MAX_PATH);
			if (BufSize == MAX_PATH)
				FAIL(_T("Service library file path too long"));
			if (BufSize == 0)
				SYSFAIL(_T("Could not get Service library file path"));
		}
		LOGV(_T("* Service library path: %s"), ServiceDLL);

		TCHAR Commandline[MAX_PATH];
		_tcscpy_s(Commandline, _SERVICE_HOST_CMD);
		if (_tcsncat_s(Commandline, ServiceGrp, _TRUNCATE) != 0)
			FAIL(_T("Unable to prepare executable path string, error %d"), errno);
		LOGV(_T("* Service commandline: %s"), Commandline);

		_HANDLE = CreateService(*SCMHandle, ServiceName, ServiceDispName,
								SERVICE_ALL_ACCESS, SERVICE_WIN32_SHARE_PROCESS,
								AutoStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START,
								SERVICE_ERROR_NORMAL, Commandline, nullptr, 0, DependOn,
								RunAccount, RunPassword);
		if (_HANDLE == nullptr)
			SYSFAIL(_T("Failed to create service"));

		{ // Handle SvcHost specific registry keys
			HKEY _KEY = nullptr;
			TString ServiceRegName = TStringCast(_T("SYSTEM\\CurrentControlSet\\services\\") << ServiceName);
			LONG Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ServiceRegName.c_str(), 0, KEY_ALL_ACCESS, &_KEY);
			if (Result != ERROR_SUCCESS)
				SYSERRFAIL(Result, _T("Unable to open service configuration"));

			{
				THKEY ServiceBaseKey(_KEY, [](HKEY &X) {CloseHandle(X); });

				Result = RegCreateKeyEx(*ServiceBaseKey, _T("Parameters"), 0, nullptr, REG_OPTION_NON_VOLATILE,
										KEY_ALL_ACCESS, nullptr, &_KEY, nullptr);
				if (Result != ERROR_SUCCESS)
					SYSERRFAIL(Result, _T("Unable to open service configuration parameters"));

				{
					THKEY ServiceParamKey(_KEY, [](HKEY &X) {CloseHandle(X); });

					Result = RegSetValueEx(*ServiceParamKey, _T("ServiceDll"), 0, REG_EXPAND_SZ,
										   (LPBYTE)ServiceDLL, (DWORD)(wcslen(ServiceDLL) + 1)*sizeof(TCHAR));
					if (Result != ERROR_SUCCESS)
						SYSERRFAIL(Result, _T("Unable to set service library path"));

					DWORD UnloadOnStop = 1;
					Result = RegSetValueEx(*ServiceParamKey, _T("ServiceDllUnloadOnStop"), 0, REG_DWORD,
										   (LPBYTE)&UnloadOnStop, sizeof(DWORD));
					if (Result != ERROR_SUCCESS)
						SYSERRFAIL(Result, _T("Unable to set service library unload flag"));
				}
			}

			LPCTSTR RegHandleName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost");
			Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegHandleName, 0, KEY_ALL_ACCESS, &_KEY);
			if (Result != ERROR_SUCCESS)
				SYSERRFAIL(Result, _T("Unable to open service host configuration"));

			{
				THKEY SvchostBaseKey(_KEY, [](HKEY &X) {CloseHandle(X); });

				DWORD DataSize = MAX_PATH*sizeof(TCHAR);
				TString GroupMembers;
				Result = ERROR_MORE_DATA;

				while (Result == ERROR_MORE_DATA) {
					GroupMembers.resize((DataSize + 1) / sizeof(TCHAR));
					Result = RegGetValue(*SvchostBaseKey, nullptr, ServiceGrp, RRF_RT_REG_MULTI_SZ,
										 nullptr, (LPBYTE)GroupMembers.data(), &DataSize);
				}
				if (Result != ERROR_SUCCESS) {
					if (Result != ERROR_FILE_NOT_FOUND) {
						SYSERRFAIL(Result, _T("Unable to query member of service group '%s'"), ServiceGrp);
					} else
						GroupMembers.clear();
				}
				bool IsMember = false;
				for (LPCTSTR Member = GroupMembers.data(); *Member != '\0'; Member += _tcslen(Member) + 1) {
					if (_tcsicmp(Member, ServiceName) == 0) {
						IsMember = true;
						break;
					}
				}
				if (!IsMember) {
					if (!GroupMembers.empty())
						GroupMembers.resize((DataSize - 1) / sizeof(TCHAR));
					GroupMembers.append(ServiceName).append(1, '\0');

					Result = RegSetValueEx(*SvchostBaseKey, ServiceGrp, 0, REG_MULTI_SZ,
										   (LPBYTE)GroupMembers.data(), (DWORD)(GroupMembers.length() + 1)*sizeof(TCHAR));
					if (Result != ERROR_SUCCESS)
						SYSERRFAIL(Result, _T("Unable to set member of service group '%s'"), ServiceGrp);
				}
			}
		}

		{
			TSCHandle ServiceHandle(_HANDLE, [](SC_HANDLE &X) {CloseServiceHandle(X); });

			// Add description
			if (ServiceDesc != nullptr) {
				if (!ChangeServiceConfig2(*ServiceHandle, SERVICE_CONFIG_DESCRIPTION, &ServiceDesc))
					SYSFAIL(_T("Failed to set service description"));
			}
			if (Privileges != nullptr) {
				if (!ChangeServiceConfig2(*ServiceHandle, SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO, &Privileges))
					SYSFAIL(_T("Failed to set service privileges"));
			}
			if (ServiceSecurity != SERVICE_SID_TYPE_NONE) {
				if (!ChangeServiceConfig2(*ServiceHandle, SERVICE_CONFIG_SERVICE_SID_INFO, &ServiceSecurity))
					SYSFAIL(_T("Failed to set service SID info"));
			}
			if (CrashRestart != 0) {
				LOGVV(_T("* Crash restart %s times with %s delay"),
					  CrashRestart > 0 ? TStringCast(CrashRestart).c_str() : _T("INFINITE"),
					  TimeSpan(RestartDelay, TimeUnit::MSEC).toString(TimeUnit::MIN).c_str());

				SERVICE_FAILURE_ACTIONS FailureActions{0};
				FailureActions.dwResetPeriod = (int)Convert(1, TimeUnit::DAY, TimeUnit::SEC);
				FailureActions.cActions = std::max(CrashRestart, 0) + 1;

				TTypedBuffer<SC_ACTION> ServiceAction(sizeof(SC_ACTION)*FailureActions.cActions);
				int Idx = FailureActions.cActions - 1;
				(&ServiceAction)[Idx].Delay = RestartDelay;
				(&ServiceAction)[Idx].Type = CrashRestart > 0 ? SC_ACTION_NONE : SC_ACTION_RESTART;
				while (--Idx >= 0) {
					(&ServiceAction)[Idx].Delay = RestartDelay;
					(&ServiceAction)[Idx].Type = SC_ACTION_RESTART;
				}
				FailureActions.lpsaActions = &ServiceAction;

				if (!ChangeServiceConfig2(*ServiceHandle, SERVICE_CONFIG_FAILURE_ACTIONS, &FailureActions))
					SYSFAIL(_T("Failed to set service failure actions"));

				SERVICE_FAILURE_ACTIONS_FLAG FailureActionFlag;
				FailureActionFlag.fFailureActionsOnNonCrashFailures = TRUE;
				if (!ChangeServiceConfig2(*ServiceHandle, SERVICE_CONFIG_FAILURE_ACTIONS_FLAG, &FailureActionFlag))
					SYSFAIL(_T("Failed to set service failure action flag"));
			}
		}
		LOG(_T("Service '%s' installed"), ServiceName);
	}
}

static void UninstallAsService(LPCTSTR ServiceName, LPCTSTR ServiceGrp) {
	SC_HANDLE _HANDLE = OpenSCManager(nullptr, nullptr, GENERIC_WRITE);
	if (_HANDLE == nullptr)
		SYSFAIL(_T("Unable to open service control manager"));

	{
		TSCHandle SCMHandle(_HANDLE, [](SC_HANDLE &X) {CloseServiceHandle(X); });

		_HANDLE = OpenService(*SCMHandle, ServiceName, SERVICE_ALL_ACCESS);
		if (_HANDLE == nullptr) {
			DWORD ErrCode = GetLastError();
			if (ErrCode != ERROR_SERVICE_DOES_NOT_EXIST)
				SYSERRFAIL(ErrCode, _T("Failed to open service"));
			LOG(_T("Service '%s' not installed"), ServiceName);
		} else {
			TSCHandle ServiceHandle(_HANDLE, [](SC_HANDLE &X) {CloseServiceHandle(X); });

			if (!DeleteService(*ServiceHandle))
				SYSFAIL(_T("Failed to delete service"));

			{ // Handle SvcHost specific registry keys
				HKEY _KEY = nullptr;
				LPCTSTR RegHandleName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost");
				LONG Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegHandleName, 0, KEY_ALL_ACCESS, &_KEY);
				if (Result != ERROR_SUCCESS)
					SYSERRFAIL(Result, _T("Unable to open service host configuration"));

				{
					THKEY SvchostBaseKey(_KEY, [](HKEY &X) {CloseHandle(X); });

					DWORD DataSize = MAX_PATH*sizeof(TCHAR);
					TString GroupMembers;
					Result = ERROR_MORE_DATA;

					while (Result == ERROR_MORE_DATA) {
						GroupMembers.resize((DataSize + 1) / sizeof(TCHAR));
						Result = RegGetValue(*SvchostBaseKey, nullptr, ServiceGrp, RRF_RT_REG_MULTI_SZ,
											 nullptr, (LPBYTE)GroupMembers.data(), &DataSize);
					}
					if (Result != ERROR_SUCCESS) {
						if (Result != ERROR_FILE_NOT_FOUND) {
							SYSERRFAIL(Result, _T("Unable to query member of service group '%s'"), ServiceGrp);
						} else
							GroupMembers.clear();
					}
					bool IsMember = false;
					TString NewGroupMembers;
					for (LPCTSTR Member = GroupMembers.data(); *Member != '\0'; Member += _tcslen(Member) + 1) {
						if (_tcsicmp(Member, ServiceName) != 0)
							NewGroupMembers.append(Member).append(1, '\0');
						else
							IsMember = true;
					}
					if (IsMember) {
						if (NewGroupMembers.empty()) {
							Result = RegDeleteValue(*SvchostBaseKey, ServiceGrp);
							if (Result != ERROR_SUCCESS)
								SYSERRFAIL(Result, _T("Unable to remove service group '%s'"), ServiceGrp);

							Result = RegDeleteKey(*SvchostBaseKey, ServiceGrp);
							if (Result != ERROR_SUCCESS) {
								if (Result != ERROR_FILE_NOT_FOUND)
									SYSERRFAIL(Result, _T("Unable to remove configurations of service group '%s'"), ServiceGrp);
							}
						} else {
							Result = RegSetValueEx(*SvchostBaseKey, ServiceGrp, 0, REG_MULTI_SZ,
												   (LPBYTE)NewGroupMembers.data(), (DWORD)(NewGroupMembers.length() + 1)*sizeof(TCHAR));
							if (Result != ERROR_SUCCESS)
								SYSERRFAIL(Result, _T("Unable to set member of service group '%s'"), ServiceGrp);
						}
					}
				}
			}
			LOG(_T("Service '%s' uninstalled"), ServiceName);
		}
	}
}

static DWORD ControlStartService(LPCTSTR ServiceName, bool WaitFor) {
	SC_HANDLE _HANDLE = OpenSCManager(nullptr, nullptr, GENERIC_READ);
	if (_HANDLE == nullptr)
		SYSFAIL(_T("Unable to open service control manager"));

	{
		TSCHandle SCMHandle(_HANDLE, [](SC_HANDLE &X) {CloseServiceHandle(X); });

		_HANDLE = OpenService(*SCMHandle, ServiceName, SERVICE_ALL_ACCESS);
		if (_HANDLE == nullptr)
			SYSFAIL(_T("Failed to open service"));

		{
			TSCHandle ServiceHandle(_HANDLE, [](SC_HANDLE &X) {CloseServiceHandle(X); });

			SERVICE_STATUS Status;
			if (!QueryServiceStatus(*ServiceHandle, &Status))
				SYSFAIL(_T("Failed to query service status"));
			if ((Status.dwCurrentState == SERVICE_START_PENDING) || (Status.dwCurrentState == SERVICE_RUNNING)) {
				LOG(_T("WARNING: Service '%s' has been started"), ServiceName);
			} else {
				LOG(_T("Starting service '%s'..."), ServiceName);
				if (!StartService(*ServiceHandle, 0, nullptr))
					SYSFAIL(_T("Failed to start service"));
				if (!QueryServiceStatus(*ServiceHandle, &Status))
					SYSFAIL(_T("Failed to query service status"));
			}

			if (WaitFor) {
				while (Status.dwCurrentState == SERVICE_START_PENDING) {
					Sleep(100);
					if (!QueryServiceStatus(*ServiceHandle, &Status))
						SYSFAIL(_T("Failed to query service status"));
				}
				if (Status.dwCurrentState == SERVICE_RUNNING) {
					LOG(_T("Service '%s' is running"), ServiceName);
				} else {
					LOG(_T("WARNING: Service '%s' did not start (%d)"), ServiceName, Status.dwCurrentState);
				}
			}
			return Status.dwCurrentState;
		}
	}
}

static DWORD ControlStopService(LPCTSTR ServiceName, bool WaitFor) {
	SC_HANDLE _HANDLE = OpenSCManager(nullptr, nullptr, GENERIC_READ);
	if (_HANDLE == nullptr)
		SYSFAIL(_T("Unable to open service control manager"));

	{
		TSCHandle SCMHandle(_HANDLE, [](SC_HANDLE &X) {CloseServiceHandle(X); });

		_HANDLE = OpenService(*SCMHandle, ServiceName, SERVICE_ALL_ACCESS);
		if (_HANDLE == nullptr)
			SYSFAIL(_T("Failed to open service"));

		{
			TSCHandle ServiceHandle(_HANDLE, [](SC_HANDLE &X) {CloseServiceHandle(X); });

			SERVICE_STATUS Status;
			if (!QueryServiceStatus(*ServiceHandle, &Status))
				SYSFAIL(_T("Failed to query service status"));
			if ((Status.dwCurrentState == SERVICE_STOP_PENDING) || (Status.dwCurrentState == SERVICE_STOPPED)) {
				LOG(_T("WARNING: Service '%s' has been signaled to stop"), ServiceName);
			} else {
				LOG(_T("Stopping service '%s'..."), ServiceName);
				if (!ControlService(*ServiceHandle, SERVICE_CONTROL_STOP, &Status))
					SYSFAIL(_T("Failed to stop service"));
			}
			if (WaitFor) {
				while (Status.dwCurrentState == SERVICE_STOP_PENDING) {
					Sleep(100);
					if (!QueryServiceStatus(*ServiceHandle, &Status))
						SYSFAIL(_T("Failed to query service status"));
				}
				if (Status.dwCurrentState == SERVICE_STOPPED) {
					LOG(_T("Service '%s' has stopped"), ServiceName);
				} else {
					LOG(_T("WARNING: Service '%s' did not stop (%d)"), ServiceName, Status.dwCurrentState);
				}
			}
			return Status.dwCurrentState;
		}
	}
}

static DWORD ControlQueryService(LPCTSTR ServiceName) {
	SC_HANDLE _HANDLE = OpenSCManager(nullptr, nullptr, GENERIC_READ);
	if (_HANDLE == nullptr)
		SYSFAIL(_T("Unable to open service control manager"));

	{
		TSCHandle SCMHandle(_HANDLE, [](SC_HANDLE &X) {CloseServiceHandle(X); });

		_HANDLE = OpenService(*SCMHandle, ServiceName, SERVICE_ALL_ACCESS);
		if (_HANDLE == nullptr)
			SYSFAIL(_T("Failed to open service"));

		{
			TSCHandle ServiceHandle(_HANDLE, [](SC_HANDLE &X) {CloseServiceHandle(X); });

			SERVICE_STATUS Status;
			if (!QueryServiceStatus(*ServiceHandle, &Status)) {
				CloseServiceHandle(*ServiceHandle);
				SYSFAIL(_T("Failed to query service status"));
			}
			switch (Status.dwCurrentState) {
				case SERVICE_START_PENDING:
					LOG(_T("Service '%s' starting..."), ServiceName);
					break;
				case SERVICE_RUNNING:
					LOG(_T("Service '%s' running"), ServiceName);
					break;
				case SERVICE_STOP_PENDING:
					LOG(_T("Service '%s' stopping..."), ServiceName);
					break;
				case SERVICE_STOPPED:
					LOG(_T("Service '%s' stopped"), ServiceName);
					break;
				case SERVICE_PAUSE_PENDING:
					LOG(_T("Service '%s' pausing..."), ServiceName);
					break;
				case SERVICE_PAUSED:
					LOG(_T("Service '%s' paused"), ServiceName);
					break;
				case SERVICE_CONTINUE_PENDING:
					LOG(_T("Service '%s' resuming..."), ServiceName);
					break;
				default:
					LOG(_T("WARNING: Unknown state (%d) for service '%s'"), Status.dwCurrentState);
			}
			return Status.dwCurrentState;
		}
	}
}

//----------------------------------------
// ServiceCtrl Specific Maintenance Code

static void CloneServiceGroup(LPCTSTR ServiceSrcGrp, LPCTSTR ServiceDstGrp) {
	HKEY _KEY = nullptr;
	LPCTSTR RegHandleName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost");
	LONG Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegHandleName, 0, KEY_ALL_ACCESS, &_KEY);
	if (Result != ERROR_SUCCESS)
		SYSERRFAIL(Result, _T("Unable to open service host configuration"));

	THKEY SvchostBaseKey(_KEY, [](HKEY &X) {CloseHandle(X); });

	Result = RegCreateKeyEx(*SvchostBaseKey, ServiceDstGrp, 0, nullptr, REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, nullptr, &_KEY, nullptr);
	if (Result != ERROR_SUCCESS)
		SYSERRFAIL(Result, _T("Unable to open target service group '%s'"), ServiceDstGrp);

	THKEY DstGroupKey(_KEY, [](HKEY &X) {CloseHandle(X); });

	Result = RegOpenKeyEx(*SvchostBaseKey, ServiceSrcGrp, 0, KEY_ALL_ACCESS, &_KEY);
	if (Result != ERROR_SUCCESS)
		SYSERRFAIL(Result, _T("Unable to open source service group '%s'"), ServiceSrcGrp);

	THKEY SrcGroupKey(_KEY, [](HKEY &X) {CloseHandle(X); });

	TCHAR Item[16384];
	DWORD DataSize = 0;
	std::string Value;
	DWORD ValueType = REG_NONE;
	for (int idx = 0; true; idx++) {
		do {
			Value.resize(DataSize);
			DWORD ItemSize = 16384;
			Result = RegEnumValue(*SrcGroupKey, idx, Item, &ItemSize, 0,
								  &ValueType, (LPBYTE)Value.data(), &DataSize);
		} while (Result == ERROR_MORE_DATA);

		if (Result != ERROR_SUCCESS) {
			if (Result == ERROR_NO_MORE_ITEMS) break;
			SYSERRFAIL(Result, _T("Unable to query value #%d on source service group '%s'"), idx, ServiceSrcGrp);
		}
		Result = RegSetValueEx(*DstGroupKey, Item, 0, ValueType,
							   (LPBYTE)Value.data(), DataSize);
		if (Result != ERROR_SUCCESS)
			SYSERRFAIL(Result, _T("Unable to set value '%s' on target service group '%s'"), Item, ServiceDstGrp);

		DataSize = (int)Value.size();
	}
}

static void MoveServiceGroup(LPCTSTR ServiceName, LPCTSTR ServiceSrcGrp, LPCTSTR ServiceDstGrp) {
	HANDLE MoveTXN = CreateTransaction(nullptr, 0, 0, 0, 0, 0, _T("Move Service"));
	if (MoveTXN == INVALID_HANDLE_VALUE)
		SYSFAIL(_T("Unable to initiate service move transaction"));

	{
		THandle HMoveTXN(MoveTXN, [](HANDLE &X) {RollbackTransaction(X); });

		HKEY _KEY = nullptr;
		LPCTSTR RegHandleName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost");
		LONG Result = RegOpenKeyTransacted(HKEY_LOCAL_MACHINE, RegHandleName, 0, KEY_ALL_ACCESS, &_KEY, MoveTXN, nullptr);
		if (Result != ERROR_SUCCESS)
			SYSERRFAIL(Result, _T("Unable to open service host configuration"));

		{
			THKEY SvchostBaseKey(_KEY, [](HKEY &X) {CloseHandle(X); });

			// Remove from source group
			DWORD DataSize = MAX_PATH*sizeof(TCHAR);
			TString GroupMembers;
			Result = ERROR_MORE_DATA;

			while (Result == ERROR_MORE_DATA) {
				GroupMembers.resize((DataSize + 1) / sizeof(TCHAR));
				Result = RegGetValue(*SvchostBaseKey, nullptr, ServiceSrcGrp, RRF_RT_REG_MULTI_SZ,
									 nullptr, (LPBYTE)GroupMembers.data(), &DataSize);
			}
			if (Result != ERROR_SUCCESS) {
				if (Result != ERROR_FILE_NOT_FOUND) {
					SYSERRFAIL(Result, _T("Unable to query member of service group '%s'"), ServiceSrcGrp);
				} else
					GroupMembers.clear();
			}
			bool IsMember = false;
			TString NewGroupMembers;
			for (LPCTSTR Member = GroupMembers.data(); *Member != '\0'; Member += _tcslen(Member) + 1) {
				if (_tcsicmp(Member, ServiceName) != 0)
					NewGroupMembers.append(Member).append(1, '\0');
				else
					IsMember = true;
			}

			if (!IsMember) {
				LOGV(_T("WARNING: Service '%s' is not a member of service group '%s'"), ServiceName, ServiceSrcGrp);
				return;
			}

			// Note: We do NOT perform empty value clean up here
			Result = RegSetValueEx(*SvchostBaseKey, ServiceSrcGrp, 0, REG_MULTI_SZ,
								   (LPBYTE)NewGroupMembers.data(), (DWORD)(NewGroupMembers.length() + 1)*sizeof(TCHAR));
			if (Result != ERROR_SUCCESS)
				SYSERRFAIL(Result, _T("Unable to set member of service group '%s'"), ServiceSrcGrp);

			// Add to target group
			GroupMembers.clear();
			DataSize = (int)GroupMembers.size()*sizeof(TCHAR);

			do {
				GroupMembers.resize((DataSize + 1) / sizeof(TCHAR));
				Result = RegGetValue(*SvchostBaseKey, nullptr, ServiceDstGrp, RRF_RT_REG_MULTI_SZ,
									 nullptr, (LPBYTE)GroupMembers.data(), &DataSize);
			} while (Result == ERROR_MORE_DATA);
			if (Result != ERROR_SUCCESS) {
				if (Result != ERROR_FILE_NOT_FOUND) {
					SYSERRFAIL(Result, _T("Unable to query member of service group '%s'"), ServiceDstGrp);
				} else
					GroupMembers.clear();
			}
			IsMember = false;
			for (LPCTSTR Member = GroupMembers.data(); *Member != '\0'; Member += _tcslen(Member) + 1) {
				if (_tcsicmp(Member, ServiceName) == 0) {
					IsMember = true;
					break;
				}
			}
			if (!IsMember) {
				if (!GroupMembers.empty())
					GroupMembers.resize((DataSize - 1) / sizeof(TCHAR));
				GroupMembers.append(ServiceName).append(1, '\0');

				Result = RegSetValueEx(*SvchostBaseKey, ServiceDstGrp, 0, REG_MULTI_SZ,
									   (LPBYTE)GroupMembers.data(), (DWORD)(GroupMembers.length() + 1)*sizeof(TCHAR));
				if (Result != ERROR_SUCCESS)
					SYSERRFAIL(Result, _T("Unable to set member of service group '%s'"), ServiceDstGrp);
			}

			TString ServiceRegName = TStringCast(_T("SYSTEM\\CurrentControlSet\\services\\") << ServiceName);
			Result = RegOpenKeyTransacted(HKEY_LOCAL_MACHINE, ServiceRegName.c_str(), 0, KEY_ALL_ACCESS, &_KEY, MoveTXN, nullptr);
			if (Result != ERROR_SUCCESS)
				SYSERRFAIL(Result, _T("Unable to open service configuration"));
		}

		{
			THKEY ServiceBaseKey(_KEY, [](HKEY &X) {CloseHandle(X); });

			DWORD DataSize = MAX_PATH*sizeof(TCHAR);
			TString ImagePath;
			Result = ERROR_MORE_DATA;

			while (Result == ERROR_MORE_DATA) {
				ImagePath.resize((DataSize + 1) / sizeof(TCHAR));
				Result = RegGetValue(*ServiceBaseKey, nullptr, _T("ImagePath"), RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND,
									 nullptr, (LPBYTE)ImagePath.data(), &DataSize);
			}
			if (Result != ERROR_SUCCESS)
				SYSERRFAIL(Result, _T("Unable to query image path for service '%s'"), ServiceName);

			TString GroupIdent = TStringCast(_T(" -k ") << ServiceSrcGrp);
			LPCTSTR Match = _tcsstr(ImagePath.data(), GroupIdent.data());
			if (!Match)
				FAIL(_T("Unable to match service group identifier in image path of service '%s'"), ServiceName, ServiceSrcGrp);

			ImagePath.replace(Match - ImagePath.data() + 4, _tcslen(ServiceSrcGrp), ServiceDstGrp);

			Result = RegSetValueEx(*ServiceBaseKey, _T("ImagePath"), 0, REG_EXPAND_SZ,
								   (LPBYTE)ImagePath.data(), (DWORD)(ImagePath.length() + 1)*sizeof(TCHAR));
			if (Result != ERROR_SUCCESS)
				SYSERRFAIL(Result, _T("Unable to set image path for service '% '%s'"), ServiceName);
		}

		// If we reach here we are done with the transaction!
		if (!CommitTransaction(MoveTXN))
			SYSFAIL(_T("Unable to commit service move transaction"));

		HMoveTXN.Invalidate();
	}

	// Stop the service being moved
	ControlStopService(ServiceName, true);
}

void CALLBACK ServiceCtrlW(HWND hwnd, HINSTANCE hinst, LPCWSTR lpszCmdLine, int nCmdShow) {
	ControlSEHTranslation(true);

	BOOL HasConsole = false;

	DEBUG_DO(HasConsole = AllocConsole());
	if (HasConsole) {
		FILE *conin, *conout, *conerr;
		freopen_s(&conin, "CONIN$", "r", stdin);
		freopen_s(&conout, "CONOUT$", "w", stdout);
		freopen_s(&conerr, "CONOUT$", "w", stderr);
	}

	LOG(_T("Process request '%s'..."), lpszCmdLine);
	try {
		TString Cmdline = lpszCmdLine;
		if (Cmdline.compare(_T("Install")) == 0) {
			InstallAsService(SERVICE_NAME, SERVICE_DISPNAME, SERVICE_DESC, SERVICE_GROUP,
							 true, SERVICE_DEPENDS, nullptr, 2, SERVICE_USER, nullptr, SERVICE_PRIVILEGES);
		} else if (Cmdline.compare(_T("Uninstall")) == 0) {
			if (ControlQueryService(SERVICE_NAME) != SERVICE_STOPPED)
				FAIL(_T("Please stop service before uninstall"));
			UninstallAsService(SERVICE_NAME, SERVICE_GROUP);
		} else if (Cmdline.compare(_T("Start")) == 0) {
			ControlStartService(SERVICE_NAME, true);
		} else if (Cmdline.compare(_T("Stop")) == 0) {
			ControlStopService(SERVICE_NAME, true);
		} else if (Cmdline.compare(_T("Status")) == 0) {
			ControlQueryService(SERVICE_NAME);
		} else if (Cmdline.compare(_T("DebugRun")) == 0) {
			LPCTSTR MainArgs[] = { _T("ConsoleDebugSvc") };
			ServiceMain_DebugRun(1, MainArgs);
		} else {
			LOG(_T("Unrecognized service operation '%s'"), lpszCmdLine);
		}
	} catch (Exception *e) {
		LOGEXCEPTIONV(e, _T("WARNING: Failed to process request"));
		DEFAULT_DESTROY(Exception, e);
	}

	if (HasConsole) system("pause");
}

#endif
