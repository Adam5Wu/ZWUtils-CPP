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

#ifdef __ZWUTILS_JAVAEMBED

/**
 * @addtogroup AdvUtils Advanced Supporting Utilities
 * @file
 * @brief High efficiency data transport to Java
 * @author Zhenyu Wu
 * @date Sep 27, 2018: Refactored business logic independent code from AgentLib-NG
 **/

#ifndef ZWUtils_JavaTransport_H
#define ZWUtils_JavaTransport_H

#include "Misc/Global.h"
#include "Misc/TString.h"
#include "Debug/Exception.h"

#include "JVMLoader.h"
#include "NativeChunkPool.h"

#include <functional>

//#include <jni.h>
// Forward declare so we do not depend on jni.h here
class _jclass;
typedef _jclass *jclass;
struct _jmethodID;
typedef struct _jmethodID *jmethodID;

class TJavaTransport : protected TJVMThreadEnv {
	typedef TJavaTransport _this;

protected:
	TJavaTransport(TJVM &xJVM) : TJVMThreadEnv(xJVM) {}
	virtual ~TJavaTransport(void){}

public:
	virtual void Forward(TNativeChunk* Chunk)
	{ FAIL(_T("Abstract function")); }
	virtual void Return(TNativeChunk* Chunk)
	{ FAIL(_T("Abstract function")); }
	virtual void Terminate(void)
	{ FAIL(_T("Abstract function")); }
};

class TJavaTransportV1 : public TJavaTransport {
	typedef TJavaTransportV1 _this;

protected:
	jclass TransportClass;
	jmethodID ForwardMethodID;
	jmethodID TerminateMethodID;
	TNativeChunkPool &ChunkPool;

public:
	TJavaTransportV1(TJVM &JVM, TNativeChunkPool &xChunkPool,
					 TString const &TransportClassName,
					 TString const &ForwardMethodName,
					 TString const &ReturnMethodName,
					 TString const &TerminateMethodName);

	~TJavaTransportV1(void) override
	{ Terminate(); }

	void Forward(TNativeChunk* Chunk);
	void Return(TNativeChunk* Chunk);
	void Terminate(void);
};

#endif //ZWUtils_JavaTransport_H

#endif