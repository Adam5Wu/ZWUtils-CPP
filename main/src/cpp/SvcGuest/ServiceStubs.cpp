
#include "Misc/Global.h"

#include "Misc/Types.h"
#include "Misc/TString.h"
#include "Misc/Timing.h"
#include "Debug/Logging.h"

extern "C" {

	//----------------------
	// Service Public Info

	extern PCTCHAR const _DEF_SERVICE_NAME = _T("SvcGuest");
	extern PCTCHAR const _DEF_SERVICE_DISPNAME = _T("SvcHost Guest Service");
	extern PCTCHAR const _DEF_SERVICE_DESC = _T("A empty service doing nothing");
	extern PCTCHAR const _DEF_SERVICE_GROUP = _T("SvcGuest");
	extern PCTCHAR const _DEF_SERVICE_DEPENDS = NullTStr;
	extern PCTCHAR const _DEF_SERVICE_USER = _T("NT AUTHORITY\\System");
	extern PCTCHAR const _DEF_SERVICE_PRIVILEGES = SE_CREATE_GLOBAL_NAME NullTStr;
	extern PCTCHAR const _DEF_SERVICE_PROGRAMPATH = _T("ZWUtil\\SvcGuest");

	//----------------------
	// Service Configurations

	extern int _DEF_CONFIG_ServiceStatusQueryInterval = (int)Convert(10, TimeUnit::MIN, TimeUnit::MSEC);
	extern int _DEF_CONFIG_ServiceTerminationGraceTime = (int)Convert(3, TimeUnit::SEC, TimeUnit::MSEC);

	extern void _DEF_ServiceMain_LoadModules(void) {
		LOGV(_T("Service started OK, but there is no module loaded."));
		LOGV(_T("Please provide ServiceMain_LoadModules() implementation to load your own logic!"));
	}

	extern void _DEF_ServiceMain_UnloadModules(void) {
		LOGV(_T("Service is terminating, no custom module unload function provided."));
		LOGV(_T("For custom module loading, please provide ServiceMain_UnloadModules() implementation!"));	}

}