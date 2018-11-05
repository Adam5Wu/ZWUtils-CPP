#ifndef ZWUtilsAppAILib_LocalComm_H
#define AppAILib_LocalComm_H

#include "LocalComm.h"

#ifdef WINDOWS

class INamedPipeServer : public ILocalCommServer {
public:
	static MRLocalCommServer Create(TString const &xPath, DWORD BufferSize, FLocalCommClientConnect const &OnConnect,
									THandleWaitable &TermSignal, TString const &DACL = EMPTY_TSTRING());
};

class INamedPipeClient {
public:
	static MRLocalCommEndPoint Connect(TString const &xPath, DWORD BufferSize, THandleWaitable &TermSignal);
};

#endif

#endif //AppAILib_LocalComm_H