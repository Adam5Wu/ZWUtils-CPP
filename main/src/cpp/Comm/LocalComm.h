#ifndef ZWUtils_LocalComm_H
#define ZWUtils_LocalComm_H

#include "Misc/TString.h"
#include "Debug/Exception.h"
#include "Memory/ManagedRef.h"
#include "Threading/SyncElements.h"

#include <functional>

class ILocalCommEndPoint {
public:
	virtual ~ILocalCommEndPoint(void) {}

	virtual bool Send(TDynBuffer &&Buffer, WAITTIME Timeout = FOREVER) {
		FAIL(_T("Abstract Function"));
	}
	virtual bool Receive(TDynBuffer &Buffer, WAITTIME Timeout = FOREVER) {
		FAIL(_T("Abstract Function"));
	}
	virtual THandleWaitable ReceiveWaitable(void) {
		FAIL(_T("Abstract Function"));
	}
	virtual bool isConnected(void) const {
		FAIL(_T("Abstract Function"));
	}
	virtual bool Disconnect(void) {
		FAIL(_T("Abstract Function"));
	}
};

typedef ManagedRef<ILocalCommEndPoint> MRLocalCommEndPoint;
typedef std::function<void(MRLocalCommEndPoint &&)> FLocalCommClientConnect;

class ILocalCommServer;
typedef ManagedRef<ILocalCommServer> MRLocalCommServer;

class ILocalCommServer {
public:
	virtual ~ILocalCommServer(void) {}

	virtual void SignalStart(void) {
		FAIL(_T("Abstract Function"));
	}
	virtual void SignalStop(void) {
		FAIL(_T("Abstract Function"));
	}
	virtual bool isServing(void) const {
		FAIL(_T("Abstract Function"));
	}
};

#endif //ZWUtils_LocalComm_H