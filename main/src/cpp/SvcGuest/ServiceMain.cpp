
// [AdvUtils] Generic System Service Interface

#include "ServiceMain.h"

#ifdef WINDOWS

#include "Debug/Logging.h"
#include "Debug/Exception.h"
#include "Debug/SysError.h"
#include "Misc/Timing.h"
#include "Threading/SyncObjects.h"

#include <Shlobj.h>
#include <vector>

#define __SERVICE_DEFSYMBOL_LINKPARAM(name)	\
	comment(linker, STRINGIZE(/alternatename:##__LINKER_SYMBOL(name)##=##__LINKER_SYMBOL(_DEF_##name)))

#pragma __SERVICE_DEFSYMBOL_LINKPARAM(SERVICE_NAME)
#pragma __SERVICE_DEFSYMBOL_LINKPARAM(SERVICE_DISPNAME)
#pragma __SERVICE_DEFSYMBOL_LINKPARAM(SERVICE_DESC)
#pragma __SERVICE_DEFSYMBOL_LINKPARAM(SERVICE_GROUP)
#pragma __SERVICE_DEFSYMBOL_LINKPARAM(SERVICE_DEPENDS)
#pragma __SERVICE_DEFSYMBOL_LINKPARAM(SERVICE_USER)
#pragma __SERVICE_DEFSYMBOL_LINKPARAM(SERVICE_PRIVILEGES)
#pragma __SERVICE_DEFSYMBOL_LINKPARAM(CONFIG_ServiceStatusQueryInterval)
#pragma __SERVICE_DEFSYMBOL_LINKPARAM(CONFIG_ServiceTerminationGraceTime)
#pragma __SERVICE_DEFSYMBOL_LINKPARAM(ServiceMain_LoadModules)
#pragma __SERVICE_DEFSYMBOL_LINKPARAM(ServiceMain_UnloadModules)

//-----------------------------------
// ServiceMain General Maintenance Code

static SERVICE_STATUS_HANDLE StatusHandle = nullptr;
static SERVICE_STATUS ServiceStatus;
static DWORD CheckPoint = 0;

static void ServiceReport(DWORD CurState, DWORD WaitHint = 0) {
	ServiceStatus.dwCurrentState = CurState;
	ServiceStatus.dwWaitHint = WaitHint;
	switch (CurState) {
		case SERVICE_START_PENDING:
		case SERVICE_STOP_PENDING:
		case SERVICE_PAUSE_PENDING:
		case SERVICE_CONTINUE_PENDING:
			ServiceStatus.dwCheckPoint = ++CheckPoint;
			break;
		default:
			ServiceStatus.dwCheckPoint = 0;
	}
	if (!SetServiceStatus(StatusHandle, &ServiceStatus))
		SYSFAIL(_T("Failed to report service status"));
}

static DWORD(*_SERVICE_MAIN)(DWORD dwArgc, LPCTSTR *lpszArgv) = nullptr;
static void(*_SERVICE_STOP)() = nullptr;
static void(*_SERVICE_SHUTDOWN)() = nullptr;
static void(*_SERVICE_PAUSE)(bool State) = nullptr;
static void(*_SERVICE_STAT)() = nullptr;

static VOID WINAPI _ServiceControlHandler(DWORD fdwControl) {
	try {
		switch (fdwControl) {
			case SERVICE_CONTROL_INTERROGATE:
				if (_SERVICE_STAT) {
					LOG(_T("* Service Control: Stat"));
					_SERVICE_STAT();
				}
				break;

			case SERVICE_CONTROL_PAUSE:
				if (_SERVICE_PAUSE) {
					LOG(_T("* Service Control: Pause"));
					_SERVICE_PAUSE(true);
				} else
					LOG(_T("WARNING: Unhandled service pause request"));
				break;

			case SERVICE_CONTROL_CONTINUE:
				if (_SERVICE_PAUSE) {
					LOG(_T("* Service Control: Resume"));
					_SERVICE_PAUSE(false);
				} else
					LOG(_T("WARNING: Unhandled service continue request"));
				break;

			case SERVICE_CONTROL_SHUTDOWN:
				if (_SERVICE_SHUTDOWN) {
					LOG(_T("* Service Control: Shutdown"));
					_SERVICE_SHUTDOWN();
				} else
					LOG(_T("WARNING: Unhandled service shutdown request"));
				break;

			case SERVICE_CONTROL_STOP:
				if (_SERVICE_STOP) {
					LOG(_T("* Service Control: Stop"));
					_SERVICE_STOP();
				} else
					LOG(_T("WARNING: Unhandled service stop request"));
				break;

			default:
				LOG(_T("WARNING: Unrecognized service control code %d, ignored"), fdwControl);
		}
	} catch (_ECR_ e) {
		LOGEXCEPTIONV(e, _T("WARNING: Unhanded Exception in service control handler"));
	}
}

static void _ServiceMain_Inner(DWORD dwArgc, LPCTSTR *lpszArgv) {
	LOG(_T("* Service '%s' starting..."), SERVICE_NAME);
	try {
		StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, _ServiceControlHandler);
		if (StatusHandle == nullptr)
			SYSFAIL(_T("Unable to register service control handler"));

		// Initialize shared status structure
		ServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
		ServiceStatus.dwControlsAccepted = 0;
		ServiceStatus.dwWin32ExitCode = NO_ERROR;
		ServiceStatus.dwServiceSpecificExitCode = 0;

		// Report start pending
		ServiceReport(SERVICE_START_PENDING, 100);

		ServiceStatus.dwControlsAccepted =
			(_SERVICE_PAUSE ? SERVICE_ACCEPT_PAUSE_CONTINUE : 0) |
			(_SERVICE_STOP ? SERVICE_ACCEPT_STOP : 0) |
			(_SERVICE_SHUTDOWN ? SERVICE_ACCEPT_SHUTDOWN : 0);

		// Invoke the main service function
		if (DWORD Ret = _SERVICE_MAIN(dwArgc, lpszArgv)) {
			ServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
			ServiceStatus.dwServiceSpecificExitCode = -1;
		}

		// Report stop pending
		ServiceReport(SERVICE_STOP_PENDING, 100);
	} catch (_ECR_ e) {
		LOGEXCEPTIONV(e, _T("WARNING: Abnormal service termination due to unhanded Exception"));

		ServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
		ServiceStatus.dwServiceSpecificExitCode = -1;
	}
	LOG(_T("* Service '%s' stopping..."), SERVICE_NAME);
}

static VOID WINAPI _ServiceMain_Outer(DWORD dwArgc, LPCTSTR *lpszArgv) {
	__try {
		_ServiceMain_Inner(dwArgc, lpszArgv);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		ServiceStatus.dwWin32ExitCode = ERROR_EXCEPTION_IN_SERVICE;
		ServiceStatus.dwServiceSpecificExitCode = GetExceptionCode();
#ifndef NDEBUG
		DebugBreak();
#endif
	}

	// Report stopped
	if (StatusHandle) {
		ServiceReport(SERVICE_STOPPED);
		StatusHandle = nullptr;
	}
}

void RunService(DWORD(*Service_Main)(DWORD dwArgc, LPCTSTR *lpszArgv),
				DWORD dwArgc, LPCTSTR *lpszArgv,
				void(*Service_Stop)() = nullptr,
				void(*Service_Shutdown)() = nullptr,
				void(*Service_Pause)(bool State) = nullptr,
				void(*Service_Stat)() = nullptr)
{
	_SERVICE_MAIN = Service_Main;
	_SERVICE_STOP = Service_Stop;
	_SERVICE_SHUTDOWN = Service_Shutdown;
	_SERVICE_PAUSE = Service_Pause;
	_SERVICE_STAT = Service_Stat;

	_ServiceMain_Outer(dwArgc, lpszArgv);
}


//------------------------------------
// ServiceMain Specific Maintenance Code

struct TServiceModuleRec {
	TString Name;
	ServiceEvent Start;
	ServiceEvent Stop;
	ServiceEvent Status;

	TServiceModuleRec(TString const &xName, ServiceEvent const &xStart, ServiceEvent const &xStop, ServiceEvent const &xStatus) :
		Name(xName), Start(xStart), Stop(xStop), Status(xStatus) { }
};

static std::vector<TServiceModuleRec>* ServiceModules = new std::vector<TServiceModuleRec>();

static TEvent &ServiceStop(void) {
	static TEvent __IoFU(true);
	return __IoFU;
}
static TEvent &ServiceAck(void) {
	static TEvent __IoFU(true);
	return __IoFU;
}

static TInterlockedOrdinal32<BOOL> ServiceRunLock(FALSE);
static TimeStamp ServiceStartTS;

static bool _Invoke_ServiceEvent(TString const &SvcName, LPCTSTR EvtKind, ServiceEvent const &SvcEvt) {
	try {
		SvcEvt();
		return true;
	} catch (_ECR_ e) {
		LOGEXCEPTIONV(e, _T("WARNING: Exception in service module '%s' %s handler"), SvcName.c_str(), EvtKind);
		return false;
	}
}

static DWORD _ServiceStart(void) {
	ServiceRunLock.Exchange(TRUE);
	for (size_t Idx = 0; Idx < ServiceModules->size(); Idx++) {
		if (ServiceModules->at(Idx).Start) {
			LOGV(_T("Starting service module '%s'..."), ServiceModules->at(Idx).Name.c_str());
			if (!_Invoke_ServiceEvent(ServiceModules->at(Idx).Name, _T("start"), ServiceModules->at(Idx).Start))
				return 200 + (DWORD)Idx + 1;
		}
	}
	return 0;
}

enum class ServiceMode {
	Normal,
	Debug
};

static LPCTSTR const SERVICE_LOG_FILENAME = _T("Service.log");
static FILE* LOGFILE = nullptr;

static DWORD ServiceModule_Init() {
	try {
		// Initialize each service module
		ServiceMain_LoadModules();
	} catch (_ECR_ e) {
		LOGEXCEPTIONV(e, _T("WARNING: Unhanded Exception in service module initializer"));
		return 100;
	}
	return 0;
}

static BOOL DirectoryExists(LPCTSTR szPath) {
	DWORD dwAttrib = GetFileAttributes(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static TString Service_GetDataDir(void) {
	TString PROGRAMDATA(MAX_PATH, NullTChar);
	HRESULT ShResult = SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, (LPTSTR)PROGRAMDATA.data());
	if (!SUCCEEDED(ShResult)) {
		LOG(_T("WARNING: Unable to acquire common application data directory (Shell API error %d)"), ShResult);
		return EmptyTText;
	}
	LOGVV(_T("+ Common application data directory '%s'"), PROGRAMDATA.c_str());

	TString Ret = TStringCast(PROGRAMDATA.c_str() << _T('\\') << SERVICE_NAME);
	if (CreateDirectory(Ret.c_str(), NULL) == 0) {
		if (GetLastError() != ERROR_ALREADY_EXISTS) {
			SYSERRLOG(_T("Unable to create service data directory '%s'"), Ret.c_str());
			return EmptyTText;
		}
	} else {
		LOGV(_T("+ Created new service data directory '%s'"), Ret.c_str());
	}
	return Ret;
}

static TLockableCS &ServiceWaitLock(void) {
	static TLockableCS __IoFU;
	return __IoFU;
}
static TLockable::MRLock ServiceRunning;

static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
	bool WaitFor = false;
	switch (dwCtrlType) {
		case CTRL_C_EVENT:
			LOG(_T("Received Ctrl-C Signal"));
			break;
		case CTRL_BREAK_EVENT:
			LOG(_T("Received Ctrl-Break Signal"));
			break;
		case CTRL_CLOSE_EVENT:
			LOG(_T("Received Close Signal"));
			WaitFor = true;
			break;
		case CTRL_LOGOFF_EVENT:
			LOG(_T("Received Log-off Signal"));
			WaitFor = true;
			break;
		default:
			LOG(_T("Received Unknown Signal (%d)"), dwCtrlType);
			break;
	}

	ServiceMain_ServiceStopNotify();
	if (WaitFor) ServiceWaitLock().Lock();
	return TRUE;
}

static TString const LOGTARGET_SERVICE(_T("Service"));

static DWORD ServiceInit(ServiceMode Mode) {
	if (Mode != ServiceMode::Debug) {
		SETLOGTARGET(LOGTARGET_CONSOLE, nullptr);
	}

	LOGV(_T("* Service initializing..."));

	TString SERVICE_DATADIR = Service_GetDataDir();
	if (SERVICE_DATADIR.empty()) return 1;

	TString SERVICE_LOG = TStringCast(SERVICE_DATADIR << _T('\\') << SERVICE_LOG_FILENAME);
	LOGFILE = _tfsopen(SERVICE_LOG.c_str(), _T("a+t, ccs=UNICODE"), _SH_DENYNO);
	if (LOGFILE == nullptr) {
		LOG(_T("WARNING: Unable to open service log file (runtime error %d)"), errno);
		return 2;
	}
	SETLOGTARGET(LOGTARGET_SERVICE, LOGFILE);

	ServiceStartTS = TimeStamp::Now();

	ServiceStop().Reset();
	ServiceAck().Reset();

	TString CurrentDir(MAX_PATH, '\0');
	GetCurrentDirectory(MAX_PATH, (LPTSTR)CurrentDir.data());
	LOGV(_T("+ Current directory: %s"), CurrentDir.c_str());

	// Initializing functional modules (Order is important)
	DWORD InitError = 0;
	switch (Mode) {
		case ServiceMode::Debug:
			// Debug specific initializations
			if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, true)) {
				LOG(_T("WARNING: Unable to set console control handler"));
				InitError = 1;
			}
			if (InitError) break;
		case ServiceMode::Normal:
			InitError = ServiceModule_Init();
			break;
		default:
			LOG(_T("WARNING: Unrecognized service mode"));
			InitError = 99;
	}
	if (InitError) return InitError;

	ServiceRunning.Assign(DEFAULT_NEW(TLockable::TLock, ServiceWaitLock().Lock()));
	// Start each service module
	return _ServiceStart();
}

static DWORD ServiceCheckStatus(void) {
	if (ServiceStop().WaitFor(0) != WaitResult::Signaled) {
		LOGV(_T("* Service status checkpoint..."));

		// Check each service module
		for (size_t Idx = 0; Idx < ServiceModules->size(); Idx++) {
			if (ServiceModules->at(Idx).Status) {
				LOGVV(_T("+ Checking service module '%s'..."), ServiceModules->at(Idx).Name.c_str());
				if (!_Invoke_ServiceEvent(ServiceModules->at(Idx).Name, _T("status"), ServiceModules->at(Idx).Status))
					return 300 + (DWORD)Idx + 1;
			}
		}
	}
	return 0;
}

static DWORD _ServiceStop(void) {
	DWORD failcount = 0;
	if (ServiceRunLock.CompareAndSwap(TRUE, FALSE)) {
		for (int Idx = (int)ServiceModules->size() - 1; Idx >= 0; Idx--) {
			if (ServiceModules->at(Idx).Stop) {
				LOGV(_T("Stopping service module '%s'..."), ServiceModules->at(Idx).Name.c_str());
				if (!_Invoke_ServiceEvent(ServiceModules->at(Idx).Name, _T("stop"), ServiceModules->at(Idx).Stop))
					failcount++;
			}
		}

		TimeSpan ServiceDuration = TimeStamp::Now() - ServiceStartTS;
		LOG(_T("* Service execution duration: %s"), ServiceDuration.toString().c_str());
	}
	return failcount;
}

static void ServiceModule_FInit(void) {
	try {
		// Finalize each service module
		ServiceMain_UnloadModules();
	} catch (_ECR_ e) {
		LOGEXCEPTIONV(e, _T("WARNING: Unhanded Exception in service module finalizer"));
	}
}

static void ServiceFInit(ServiceMode Mode) {
	LOGV(_T("* Service finalizing..."));

	// Stop each service module
	if (DWORD FailCount = _ServiceStop()) {
		LOG(_T("WARNING: %d service module stop handler have failed"), FailCount);
	}

	// Finalizing functional modules
	switch (Mode) {
		case ServiceMode::Debug:
			// Debug specific finalizations
			//...
		case ServiceMode::Normal:
			ServiceModule_FInit();
			break;
		default:
			LOG(_T("WARNING: Unrecognized service mode"));
	}

	SETLOGTARGET(LOGTARGET_SERVICE, nullptr);
	if (LOGFILE != nullptr) fclose(LOGFILE);

	ServiceRunning.Clear();
}

static DWORD __SvcMain(void) {
	__try {
		ServiceReport(SERVICE_START_PENDING, 100);
		__try {
			if (DWORD ExitCode = ServiceInit(ServiceMode::Normal))
				return ExitCode;

			ServiceReport(SERVICE_RUNNING);
			while (ServiceStop().WaitFor(CONFIG_ServiceStatusQueryInterval) != WaitResult::Signaled) {
				if (DWORD ExitCode = ServiceCheckStatus())
					return ExitCode;
			}
			ServiceAck().Set();
		} __finally {
			ServiceReport(SERVICE_STOP_PENDING, CONFIG_ServiceTerminationGraceTime);
			ServiceFInit(ServiceMode::Normal);
		}
	} __finally {
		ServiceReport(SERVICE_STOP_PENDING, 100);
	}
	return 0;
}

static DWORD _SvcMain(DWORD dwArgc, LPCTSTR *lpszArgv) {
	LOGV(_T("* Service main function for '%s'"), lpszArgv[0]);

	if (dwArgc > 1) {
		TString DispToken(lpszArgv[1]);
		for (DWORD i = 2; i < dwArgc; i++)
			DispToken.append(_T(" ")).append(lpszArgv[i]);
		LOG(_T("WARNING: Ignored extra arguments '%s'"), DispToken.c_str());
	}
	return __SvcMain();
}

static DWORD __DbgSvcMain(void) {
	__try {
		//ServiceReport(SERVICE_START_PENDING, 100);
		__try {
			if (DWORD ExitCode = ServiceInit(ServiceMode::Debug))
				return ExitCode;

			//ServiceReport(SERVICE_RUNNING);
			while (ServiceStop().WaitFor(CONFIG_ServiceStatusQueryInterval) != WaitResult::Signaled) {
				if (DWORD ExitCode = ServiceCheckStatus())
					return ExitCode;
			}
			ServiceAck().Set();
		} __finally {
			//ServiceReport(SERVICE_STOP_PENDING, CONFIG_ServiceTerminationGraceTime);
			ServiceFInit(ServiceMode::Debug);
		}
	} __finally {
		//ServiceReport(SERVICE_STOP_PENDING, 100);
	}
	return 0;
}

UINT32 ServiceMain_DebugRun(UINT32 dwArgc, PCTCHAR *lpszArgv) {
	LOGV(_T("* Debug service main function for '%s'"), lpszArgv[0]);
#ifdef _DEBUG
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF);
#endif

	if (dwArgc > 1) {
		TString DispToken(lpszArgv[1]);
		for (UINT32 i = 2; i < dwArgc; i++)
			DispToken.append(_T(" ")).append(lpszArgv[i]);
		LOG(_T("WARNING: Ignored extra arguments '%s'"), DispToken.c_str());
	}
	return __DbgSvcMain();
}

void ServiceMain_ServiceStopNotify(void) {
	LOGV(_T("Signaling service to stop..."));
	ServiceStop().Set();
	if (ServiceAck().WaitFor(CONFIG_ServiceTerminationGraceTime) == WaitResult::Signaled) {
		LOGV(_T("Stop request is being acknowledged..."));
	} else {
		LOGV(_T("WARNING: Stop request acknowledgement timeout"));
	}
}

THandleWaitable &ServiceMain_TermSignal(void) {
	static THandleWaitable __IoFU = ServiceStop().DupWaitable();
	return __IoFU;
}


DWORD WINAPI ServiceMain(DWORD dwArgc, LPCWSTR *lpszArgv) {
	ControlSEHTranslation(true);
	try {
		RunService(_SvcMain, dwArgc, lpszArgv, ServiceMain_ServiceStopNotify, ServiceMain_ServiceStopNotify);
	} catch (_ECR_ e) {
		LOGEXCEPTIONV(e, _T("WARNING: Unhanded Exception in service main"));
		return -1;
	}
	return 0;
}

void Module_ServiceMain_AddModule(TString const &Name, ServiceEvent const &Start, ServiceEvent const &Stop, ServiceEvent const &Status) {
	if (~ServiceRunLock)
		FAIL(_T("Service already running!"));

	for (size_t Idx = 0; Idx < ServiceModules->size(); Idx++) {
		if (ServiceModules->at(Idx).Name.compare(Name) == 0)
			FAIL(_T("Service module '%s' has been registered"), Name);
	}
	ServiceModules->emplace_back(Name, Start, Stop, Status);
}

#endif