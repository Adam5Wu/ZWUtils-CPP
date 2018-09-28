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

// [AdvUtils] High efficiency data transport to Java

#include "JavaTransport.h"

#include "Debug/Exception.h"
#include "Debug/Logging.h"

#include <jni.h>

void JNICALL __ReturnStubV1(JNIEnv* Env, jobject Object, jlong RetTicket1, jlong RetTicket2) {
	TJavaTransportV1* Transport = (TJavaTransportV1*)RetTicket2;
	TNativeChunk* Chunk = (TNativeChunk*)RetTicket1;
	Transport->Return(Chunk);
}

TJavaTransportV1::TJavaTransportV1(TJVM &JVM, TNativeChunkPool &xChunkPool,
	TString const &TransportClassName,
	TString const &ForwardMethodName,
	TString const &ReturnMethodName,
	TString const &TerminateMethodName) :
	TJavaTransport(JVM),
	ChunkPool(xChunkPool) {
	JNIEnv* Env = Refer();

#ifdef UNICODE
	{
		TString ConvErrMsg;
		CString ConvClassName = WStringtoCString(CP_ACP, TransportClassName, ConvErrMsg);
		if (!ConvErrMsg.empty()) FAIL(_T("Failed to convert class name - %s"), ConvErrMsg.c_str());
		TransportClass = Env->FindClass(ConvClassName.c_str());
	}
#else
	TransportClass = Env->FindClass(TransportClassName.c_str());
#endif
	if (TransportClass == nullptr) FAIL(_T("Unable to locate transport class '%s'"), TransportClassName.c_str());

#ifdef UNICODE
	{
		TString ConvErrMsg;
		CString ConvForwardMethodName = WStringtoCString(CP_ACP, ForwardMethodName, ConvErrMsg);
		if (!ConvErrMsg.empty()) FAIL(_T("Failed to convert forward method name - %s"), ConvErrMsg.c_str());
		ForwardMethodID = Env->GetStaticMethodID(TransportClass, ConvForwardMethodName.c_str(), "(Ljava/nio/ByteBuffer;)V");
	}
#else
	ForwardMethodID = Env->GetStaticMethodID(TransportClass, ForwardMethodName.c_str(), "(Ljava/nio/ByteBuffer;)V");
#endif
	if (ForwardMethodID == nullptr) FAIL(_T("Unable to locate forward method '%s'"), ForwardMethodName.c_str());

	JNINativeMethod RegMethod;
#ifdef UNICODE
	CString ConvReturnMethodName;
	{
		TString ConvErrMsg;
		ConvReturnMethodName = WStringtoCString(CP_ACP, ReturnMethodName, ConvErrMsg);
		if (!ConvErrMsg.empty()) FAIL(_T("Failed to convert return method name - %s"), ConvErrMsg.c_str());
		RegMethod = { const_cast<char*>(ConvReturnMethodName.c_str()), "(JJ)V", &__ReturnStubV1 };
	}
#else
	RegMethod = { const_cast<char*>(ReturnMethodName.c_str()), "(JJ)V", &__ReturnStubV1 };
#endif
	jint res = Env->RegisterNatives(TransportClass, &RegMethod, 1);
	if (res != JNI_OK) FAIL(_T("Unable to register return method (%d)"), res);

#ifdef UNICODE
	{
		TString ConvErrMsg;
		CString ConvTerminateMethodName = WStringtoCString(CP_ACP, TerminateMethodName, ConvErrMsg);
		if (!ConvErrMsg.empty()) FAIL(_T("Failed to convert terminate method name - %s"), ConvErrMsg.c_str());
		TerminateMethodID = Env->GetStaticMethodID(TransportClass, ConvTerminateMethodName.c_str(), "()V");
	}
#else
	TerminateMethodID = Env->GetStaticMethodID(TransportClass, TerminateMethodName.c_str(), "()V");
#endif
	if (TerminateMethodID == nullptr) FAIL(_T("Unable to locate terminate method '%s'"), TerminateMethodName.c_str());
}

void TJavaTransportV1::Forward(TNativeChunk* Chunk) {
	TNativeChunkV1* ChunkV1 = dynamic_cast<TNativeChunkV1*>(Chunk);
	if (ChunkV1 == nullptr) FAIL(_T("Unable to handle this chunk class"));

	// Set the instance address as part of the return ticket
	PNativeChunkBufV1 BufferV1 = (PNativeChunkBufV1)Chunk->GetBufferHead();
	BufferV1->RetTicket.Value[1] = this;

	JNIEnv* Env = Refer();
	jobject ChunkWrap = Env->NewDirectByteBuffer(BufferV1, ChunkV1->Size());
	if (ChunkWrap == nullptr) FAIL(_T("Unable to allocate buffer wrapper"));

	Env->CallStaticVoidMethod(TransportClass, ForwardMethodID, ChunkWrap);
	LOGVV(_T("Forwarded chunk #%p"), ChunkV1);
}

void TJavaTransportV1::Return(TNativeChunk* Chunk) {
	LOGVV(_T("Returned chunk #%p"), Chunk);
	ChunkPool.Return(Chunk);
}

void TJavaTransportV1::Terminate(void){
	JNIEnv* Env = Refer();
	Env->CallStaticVoidMethod(TransportClass, TerminateMethodID);
}
