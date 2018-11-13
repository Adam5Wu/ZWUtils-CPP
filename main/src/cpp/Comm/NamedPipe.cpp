#include "NamedPipe.h"

#ifdef WINDOWS

#include "Threading/SyncContainers.h"
#include "Threading/WorkerThread.h"

#include <Aclapi.h>
#include <sddl.h>

#define NAMEDPIPE_PATHPFX _T("\\\\.\\pipe\\")
#define NAMEDPIPE_SERVER_NAMEPFX _T("NPServer")
#define NAMEDPIPE_CLIENT_NAMEPFX _T("NPClient")

class TNamedPipeServer : public INamedPipeServer {
	class TServRec : public ManagedObj {
	public:
		TString const _Name;
		TString const _Path;
		DWORD const _BufferSize;
		FLocalCommClientConnect _OnConnect;
		THandleWaitable &_TermSignal;
		TString const _DACL;

		TServRec(TString const &Path, DWORD BufferSize, FLocalCommClientConnect const &OnConnect,
				 THandleWaitable &TermSignal, TString const &DACL)
			: _Name(TStringCast(NAMEDPIPE_SERVER_NAMEPFX << _T('<') << Path << _T('>')))
			, _Path(Path), _BufferSize(BufferSize), _OnConnect(OnConnect)
			, _TermSignal(TermSignal), _DACL(DACL)
		{}
	};

	class TServRunnable : public TRunnable {
		ManagedRef<TServRec> _ServRec;
		TInterlockedOrdinal32<unsigned int> _ClientCnt;
		TEvent _IntTermSignal;
	public:
		explicit TServRunnable(TServRec *ServRec)
			: _ServRec(ServRec), _ClientCnt(0), _IntTermSignal(true) {}

		virtual TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) override;

		virtual void StopNotify(TWorkerThread &WorkerThread) override {
			_IntTermSignal.Set();
		}
	};

	ManagedRef<TServRec> _ServRec;
	MRWorkerThread _ServThread;

public:
	TNamedPipeServer(TString const &xPath, DWORD BufferSize, FLocalCommClientConnect const &OnConnect,
					 THandleWaitable &TermSignal, TString const &xDACL)
		: _ServRec(CONSTRUCTION::EMPLACE, xPath, BufferSize, OnConnect, TermSignal, xDACL)
		, _ServThread(CONSTRUCTION::EMPLACE, _ServRec->_Name,
					  MRRunnable(DEFAULT_NEW(TServRunnable, &_ServRec), CONSTRUCTION::HANDOFF))
	{
		// Do not automatically start the service thread, because the callback handler may not be ready
	}

	virtual void SignalStart(void) override {
		_ServThread->Start();
	}

	virtual void SignalStop(void) override {
		_ServThread->SignalTerminate();
	}

};

typedef TSyncBlockingDeque<TDynBuffer> TCommBufferQueue;

class TNamedPipeEndPoint : public ILocalCommEndPoint {
	typedef TNamedPipeEndPoint _this;
	friend class TNamedPipeServer;
	friend class INamedPipeClient;

	class TServRec : public ManagedObj {
	public:
		TString const _Name;
		THandle _Pipe;
		DWORD const _BufferSize;
		TCommBufferQueue _InQueue;
		TCommBufferQueue _OutQueue;
		THandleWaitable &_TermSignal;

		TServRec(TString && Name, THandle && Pipe, DWORD BufferSize, THandleWaitable &TermSignal)
			: _Name(std::move(Name)), _Pipe(std::move(Pipe)), _BufferSize(BufferSize)
			, _InQueue(TStringCast(_Name << _T("-InQ")))
			, _OutQueue(TStringCast(_Name << _T("-OutQ")))
			, _TermSignal(TermSignal)
		{}
	};

	class TServRunnable : public TRunnable {
		ManagedRef<TServRec> _ServRec;
		TEvent _IntTermSignal;
	protected:
		bool __Handle_AsyncReadInitiate(TDynBuffer &InBuffer, OVERLAPPED &AsyncRead, THandle &ReadSignalHandle,  TWorkerThread & WorkerThread);
		bool __Handle_AsyncReadCompletion(TDynBuffer &InBuffer, OVERLAPPED &AsyncRead, TEvent &ReadEvent, TWorkerThread &WorkerThread);
		bool __Handle_AsyncWriteInitiate(TDynBuffer &OutBuffer, OVERLAPPED &AsyncWrite, THandle &WriteSignalHandle, TWorkerThread & WorkerThread);
		bool __Handle_AsyncWriteCompletion(TDynBuffer &OutBuffer, OVERLAPPED &AsyncWrite, TEvent &WriteEvent, TWorkerThread & WorkerThread);

	public:
		explicit TServRunnable(TServRec *ServRec)
			: _ServRec(ServRec), _IntTermSignal(true) {}

		virtual TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) override;

		virtual void StopNotify(TWorkerThread &WorkerThread) override {
			_IntTermSignal.Set();
		}
	};

	ManagedRef<TServRec> _ServRec;
	MRWorkerThread _ClientThread;

protected:
	TNamedPipeEndPoint(TString &&Name, THandle &&Pipe, DWORD BufferSize, THandleWaitable &TermSignal)
		: _ServRec(CONSTRUCTION::EMPLACE, std::move(Name), std::move(Pipe), BufferSize, TermSignal)
		, _ClientThread(CONSTRUCTION::EMPLACE, _ServRec->_Name,
						MRRunnable(DEFAULT_NEW(TServRunnable, &_ServRec), CONSTRUCTION::HANDOFF))
	{
		// Automatically start the service thread
		_ClientThread->Start();
	}

public:
	TString const Name;

	virtual bool Send(TDynBuffer && Buffer, DWORD Timeout = FOREVER) override {
		return isConnected() && _ServRec->_OutQueue.Push_Back(std::move(Buffer), Timeout,
															  &_ServRec->_TermSignal);
	}

	virtual bool Receive(TDynBuffer & Buffer, DWORD Timeout = FOREVER) override {
		return isConnected() && _ServRec->_InQueue.Pop_Front(Buffer, Timeout,
															 &_ServRec->_TermSignal);
	}

	virtual THandleWaitable ReceiveWaitable(void) override {
		return _ServRec->_InQueue.ContentWaitable();
	}

	virtual bool isConnected(void) const {
		return _ClientThread->CurrentState() == TWorkerThread::State::Running;
	}

	virtual bool Disconnect(void) {
		return _ClientThread->SignalTerminate() == TWorkerThread::State::Running;
	}

};

#define NPLogHeader _T("{%s} ")

#define NPLOG(s, ...)		LOG(NPLogHeader s, _ServRec->_Name.c_str(), __VA_ARGS__)
#define NPLOGV(s, ...)		LOGV(NPLogHeader s, _ServRec->_Name.c_str(), __VA_ARGS__)
#define NPLOGVV(s, ...)		LOGVV(NPLogHeader s, _ServRec->_Name.c_str(), __VA_ARGS__)
#define NPERRLOG(e, s, ...)		ERRLOG(e, NPLogHeader s, _ServRec->_Name.c_str(), __VA_ARGS__)
#define NPSYSERRLOG(s, ...)		SYSERRLOG(NPLogHeader s, _ServRec->_Name.c_str(), __VA_ARGS__)
#define NPFAIL(s, ...)			FAIL(NPLogHeader s, _ServRec->_Name.c_str(), __VA_ARGS__)
#define NPSYSFAIL(s, ...)		SYSFAIL(NPLogHeader s, _ServRec->_Name.c_str(), __VA_ARGS__)
#define NPSYSERRFAIL(e, s, ...)	SYSERRFAIL(e, NPLogHeader s, _ServRec->_Name.c_str(), __VA_ARGS__)
#define NPLOGEXCEPTIONV(e, s, ...)	LOGEXCEPTIONV(e, NPLogHeader s, _ServRec->_Name.c_str(), __VA_ARGS__)

#define __GEN_CANCEL_PIPE_IO(__X,__O)								\
if (!CancelIoEx(__X, __O)) {										\
	DWORD ErrCode = GetLastError();									\
	switch (ErrCode) {												\
		case ERROR_NOT_FOUND:										\
			break;													\
		default:													\
			NPERRLOG(ErrCode, _T("Error cancelling IO operation"));	\
	}																\
}

#define __GEN_TERMSIGNAL_HANDLE										\
case 0:																\
	NPLOGV(_T("Detected external termination signal"));				\
	WorkerThread.SignalTerminate();									\
	continue;														\
case 1:																\
	NPLOGV(_T("Detected internal termination signal"));				\
	continue;

#define __GEN_ASYNCERRROR_HANDLE												\
switch (ErrCode) {																\
	case ERROR_MORE_DATA:														\
		NPLOG(_T("ERROR: Excessive data size"));								\
		break;																	\
	case ERROR_BROKEN_PIPE:														\
		NPLOGV(_T("ERROR: Pipe disconnected from the other end"));				\
		break;																	\
	default:																	\
		NPERRLOG(ErrCode, _T("ERROR: Unexpected asynchronous error status"));	\
}																				\
WorkerThread.SignalTerminate();

TFixedBuffer TNamedPipeServer::TServRunnable::Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) {
	TString FullPath = TStringCast(NAMEDPIPE_PATHPFX << _ServRec->_Path);
	NPLOGV(_T("Starting serving (%s)"), FullPath.c_str());

	TEvent ConnectEvent(true);
	THandle ConnectSignalHandle = ConnectEvent.SignalHandle();
	OVERLAPPED AsyncConnect;

	try {
		while (WorkerThread.CurrentState() == TWorkerThread::State::Running) {
			THandle _Pipe(
				[&] {
					SECURITY_ATTRIBUTES SA = { 0 };
					SA.nLength = sizeof(SECURITY_ATTRIBUTES);

					if (!_ServRec->_DACL.empty()) {
						// Prepare DACL that allow users read access
						PSECURITY_DESCRIPTOR pSD;
						if (!ConvertStringSecurityDescriptorToSecurityDescriptor(_ServRec->_DACL.c_str(), SDDL_REVISION_1, &pSD, nullptr)) {
							NPSYSFAIL(_T("Unable to create security descriptor"));
						}
						TInitResource<PSECURITY_DESCRIPTOR> SD_RAII(pSD, [](PSECURITY_DESCRIPTOR &X) { LocalFree(X); });

						SA.lpSecurityDescriptor = pSD;
					}

					// Create NamedPipe server side
					HANDLE Ret = CreateNamedPipe(FullPath.c_str(),
												 PIPE_ACCESS_DUPLEX |
												 FILE_FLAG_OVERLAPPED,
												 PIPE_TYPE_MESSAGE |
												 PIPE_READMODE_MESSAGE |
												 PIPE_WAIT,
												 PIPE_UNLIMITED_INSTANCES,
												 _ServRec->_BufferSize,
												 _ServRec->_BufferSize,
												 0, &SA);
					if (Ret == INVALID_HANDLE_VALUE) {
						NPSYSFAIL(_T("Unable to create named pipe"));
					}
					return Ret;
				});

			{
				// Prepare overlapped IO structure
				AsyncConnect = { 0 };
				AsyncConnect.hEvent = *ConnectSignalHandle;

				TInitResource<OVERLAPPED*> OverlappedConnect_RAII(
					&AsyncConnect,
					[&](OVERLAPPED* &X) {
						if (_Pipe.Allocated() && !HasOverlappedIoCompleted(X)) {
							// Need to cancel the unfinished wait for connection
							NPLOGVV(_T("Try to cancel unfinished connect..."));
							__GEN_CANCEL_PIPE_IO(*_Pipe, X);
						}
						ConnectEvent.Reset();
					});

				// Try asynchronous ConnectNamedPipe
				if (ConnectNamedPipe(*_Pipe, &AsyncConnect))
					NPFAIL(_T("Unexpected success return for asynchronous ConnectNamedPipe"));
				DWORD ErrCode = GetLastError();
				switch (ErrCode) {
					case ERROR_IO_PENDING:
					{
						NPLOGVV(_T("Waiting for client to connect"));
						auto WaitResult = WaitMultiple({ _ServRec->_TermSignal, _IntTermSignal, ConnectEvent }, false);
						if ((WaitResult >= WaitResult::Signaled_0) && (WaitResult <= WaitResult::Signaled_MAX)) {
							auto SlotIdx = WaitSlot_Signaled(WaitResult);
							switch (SlotIdx) {
								__GEN_TERMSIGNAL_HANDLE

								case 2:
									// Client connected, handle in the next case via fall through
									break;

								default:
									NPLOG(_T("ERROR: Unexpected signaled wait slot (%d)"), SlotIdx);
									WorkerThread.SignalTerminate();
							}
						} else {
							NPFAIL(_T("ERROR: Unexpected wait result '%s'"), WaitResultToString(WaitResult).c_str());
						}
					} // Fall through...

					case ERROR_PIPE_CONNECTED:
					{
						auto ClientIdx = _ClientCnt.Increment();
						NPLOGV(_T("Client #%d connected"), ClientIdx);
						TString ClientName = TStringCast(_ServRec->_Name << _T('#') << ClientIdx);
						_ServRec->_OnConnect({
							DEFAULT_NEW(TNamedPipeEndPoint, std::move(ClientName),
								std::move(_Pipe), _ServRec->_BufferSize, _ServRec->_TermSignal),
							CONSTRUCTION::HANDOFF });
					} break;

					default:
						NPSYSERRFAIL(ErrCode, _T("Unexpected error for asynchronous ConnectNamedPipe"));
				}
			}
		}
	} catch (_ECR_ e) {
		NPLOGEXCEPTIONV(e, _T("Named pipe server failed"));
	}

	NPLOGV(_T("Stopped serving (%s)"), FullPath.c_str());
	return {};
}

bool TNamedPipeEndPoint::TServRunnable::__Handle_AsyncReadInitiate(TDynBuffer &InBuffer, OVERLAPPED &AsyncRead, THandle &ReadSignalHandle, TWorkerThread & WorkerThread)
{
	AsyncRead.hEvent = *ReadSignalHandle;
	InBuffer.SetSize(_ServRec->_BufferSize);
	if (!ReadFile(*_ServRec->_Pipe, &InBuffer, _ServRec->_BufferSize, NULL, &AsyncRead)) {
		DWORD ErrCode = GetLastError();
		if (ErrCode != ERROR_IO_PENDING) {
			__GEN_ASYNCERRROR_HANDLE;
			return false;
		} else {
			NPLOGVV(_T("Waiting for incoming data..."));
		}
	}
	// If read completed synchronously, just handle it later as async completed case
	return true;
}

bool TNamedPipeEndPoint::TServRunnable::__Handle_AsyncReadCompletion(TDynBuffer &InBuffer, OVERLAPPED &AsyncRead, TEvent &ReadEvent, TWorkerThread &WorkerThread)
{
	DWORD cbRead;
	if (!GetOverlappedResult(*_ServRec->_Pipe, &AsyncRead, &cbRead, FALSE)) {
		DWORD ErrCode = GetLastError();
		__GEN_ASYNCERRROR_HANDLE;
		return false;
	}
	AsyncRead = { 0 };
	ReadEvent.Reset();
	InBuffer.SetSize(cbRead);
	_ServRec->_InQueue.Push_Back(std::move(InBuffer), FOREVER, &_ServRec->_TermSignal);
	return true;
}

bool TNamedPipeEndPoint::TServRunnable::__Handle_AsyncWriteInitiate(TDynBuffer &OutBuffer, OVERLAPPED &AsyncWrite, THandle &WriteSignalHandle, TWorkerThread & WorkerThread)
{
	AsyncWrite.hEvent = *WriteSignalHandle;
	if (!WriteFile(*_ServRec->_Pipe, &OutBuffer, (DWORD)OutBuffer.GetSize(), NULL, &AsyncWrite)) {
		DWORD ErrCode = GetLastError();
		if (ErrCode != ERROR_IO_PENDING) {
			__GEN_ASYNCERRROR_HANDLE;
			return false;
		} else {
			NPLOGVV(_T("Waiting for outgoing write..."));
		}
	}
	// If write completed synchronously, just handle it later as async completed case
	return true;
}

bool TNamedPipeEndPoint::TServRunnable::__Handle_AsyncWriteCompletion(TDynBuffer &OutBuffer, OVERLAPPED &AsyncWrite, TEvent &WriteEvent, TWorkerThread & WorkerThread)
{
	DWORD cbWrite;
	if (!GetOverlappedResult(*_ServRec->_Pipe, &AsyncWrite, &cbWrite, FALSE)) {
		DWORD ErrCode = GetLastError();
		__GEN_ASYNCERRROR_HANDLE;
		return false;
	}
	if (cbWrite != OutBuffer.GetSize()) {
		NPLOGV(_T("WARNING: Unexpected written data size (%d, expect %d)"), cbWrite, OutBuffer.GetSize());
	}
	OutBuffer.Deallocate();
	AsyncWrite = { 0 };
	WriteEvent.Reset();
	return true;
}

TFixedBuffer TNamedPipeEndPoint::TServRunnable::Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) {
	NPLOGV(_T("Starting serving..."));
	TDynBuffer OutBuffer;
	TDynBuffer InBuffer;
	THandleWaitable OutRequest = _ServRec->_OutQueue.ContentWaitable();

	THandle ExtTermWaitHandle = _ServRec->_TermSignal.WaitHandle();
	THandle IntTermWaitHandle = _IntTermSignal.WaitHandle();

	TEvent ReadEvent(true);
	THandle ReadWaitHandle = ReadEvent.WaitHandle();
	THandle ReadSignalHandle = ReadEvent.SignalHandle();
	OVERLAPPED AsyncRead = { 0 };

	TEvent WriteEvent(true);
	THandle WriteWaitHandle = WriteEvent.WaitHandle();
	THandle WriteSignalHandle = WriteEvent.SignalHandle();
	OVERLAPPED AsyncWrite = { 0 };

	std::vector<HANDLE> WaitEvents = { *ExtTermWaitHandle, *IntTermWaitHandle, *ReadWaitHandle, INVALID_HANDLE_VALUE };

	// RAII-based cleanup
	TInitResource<int> Cleanup_RAII(
		0, [&](int &) {
			// Close pipe before worker exit
			_ServRec->_Pipe.Deallocate();
		});

	try {
		TInitResource<OVERLAPPED*> OverlappedRead_RAII(
			&AsyncRead,
			[&](OVERLAPPED* &X) {
				if (X->hEvent && !HasOverlappedIoCompleted(X)) {
					// Need to cancel the unfinished read
					NPLOGVV(_T("Try to cancel unfinished read..."));
					__GEN_CANCEL_PIPE_IO(*_ServRec->_Pipe, X);
				}
			});

		TInitResource<OVERLAPPED*> OverlappedWrite_RAII(
			&AsyncWrite,
			[&](OVERLAPPED* &X) {
				if (X->hEvent && !HasOverlappedIoCompleted(X)) {
					// Need to cancel the unfinished wait for connection
					NPLOGVV(_T("Try to cancel unfinished write..."));
					__GEN_CANCEL_PIPE_IO(*_ServRec->_Pipe, X);
				}
			});

		while (WorkerThread.CurrentState() == TWorkerThread::State::Running) {
			if (!AsyncRead.hEvent) {
				if (!__Handle_AsyncReadInitiate(InBuffer, AsyncRead, ReadSignalHandle, WorkerThread))
					break;
			}

			WaitEvents[3] = AsyncWrite.hEvent ? *WriteWaitHandle : *OutRequest.WaitHandle();
			auto WaitResult = WaitMultiple(WaitEvents, false);
			if ((WaitResult >= WaitResult::Signaled_0) && (WaitResult <= WaitResult::Signaled_MAX)) {
				auto SlotIdx = WaitSlot_Signaled(WaitResult);
				switch (SlotIdx) {
					__GEN_TERMSIGNAL_HANDLE

					case 2:
						NPLOGVV(_T("Detected incoming activity"));
						__Handle_AsyncReadCompletion(InBuffer, AsyncRead, ReadEvent, WorkerThread);
						break;

					case 3:
						if (AsyncWrite.hEvent) {
							NPLOGVV(_T("Outgoing write finished"));
							__Handle_AsyncWriteCompletion(OutBuffer, AsyncWrite, WriteEvent, WorkerThread);
						} else {
							if (_ServRec->_OutQueue.Pop_Front(OutBuffer, FOREVER, &_ServRec->_TermSignal)) {
								__Handle_AsyncWriteInitiate(OutBuffer, AsyncWrite, WriteSignalHandle, WorkerThread);
							}
						}
						break;

					default:
						NPLOG(_T("ERROR: Unexpected signaled wait slot (%d)"), SlotIdx);
						WorkerThread.SignalTerminate();
				}
			} else {
				NPFAIL(_T("ERROR: Unexpected wait result '%s'"), WaitResultToString(WaitResult).c_str());
			}
		}
	} catch (_ECR_ e) {
		NPLOGEXCEPTIONV(e, _T("Named pipe endpoint failed"));
	}

	NPLOGV(_T("Stopped serving"));
	return {};
}

MRLocalCommServer INamedPipeServer::Create(TString const &xPath, DWORD BufferSize, FLocalCommClientConnect const &OnConnect,
										   THandleWaitable &TermSignal, TString const &DACL) {
	return {
		DEFAULT_NEW(TNamedPipeServer, xPath, BufferSize, OnConnect, TermSignal, DACL),
		CONSTRUCTION::HANDOFF
	};
}

MRLocalCommEndPoint INamedPipeClient::Connect(TString const &xPath, DWORD BufferSize, THandleWaitable &TermSignal) {
	TString FullPath = TStringCast(NAMEDPIPE_PATHPFX << xPath);
	LOGVV(_T("Connecting to %s"), FullPath.c_str());

	HANDLE hPipe = CreateFile(FullPath.c_str(), GENERIC_READ | GENERIC_WRITE,
							  0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		SYSFAIL(_T("Failed to connect to pipe <%s>"), xPath.c_str());
	}
	TString ClientName = TStringCast(NAMEDPIPE_CLIENT_NAMEPFX << _T('<') << xPath << _T('>'));
	return {
		DEFAULT_NEW(TNamedPipeEndPoint, std::move(ClientName), { hPipe }, BufferSize, TermSignal),
		CONSTRUCTION::HANDOFF
	};
}

#endif