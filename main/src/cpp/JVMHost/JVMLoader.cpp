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

// [AdvUtils] JVM loading and initialization support

#include "JVMLoader.h"

#include "Debug/Exception.h"
#include "Debug/SysError.h"
#include "Debug/Logging.h"

#include <jni.h>

#ifndef JRE_PLATFORM_PATHFRAG
#pragma WARNING("Please define the JRE platform path before compiling this file!")
#if _MSC_VER
#pragma WARNING("Hint - /D \"JRE_PLATFORM_PATHFRAG=\\\"$(TargetPlatformIdentifier)\\\\$(PlatformTarget)\\\"\"")
#endif
#define JRE_PLATFORM_PATHFRAG ""
#else
#pragma message("JRE_PLATFORM_PATHFRAG = " JRE_PLATFORM_PATHFRAG)
#endif //SOLUTION_PATH

#ifdef WINDOWS

#pragma comment(lib, "jvm.lib")

// For app-local JRE embedding, need to delay-load the jvm library

static void AddSearchPath(TString Value) {
	static LPCTSTR const ENV_PATH = _T("PATH");
	const DWORD BufSize = GetEnvironmentVariable(ENV_PATH, nullptr, 0);
	TString BufVal(BufSize, _T('\0'));
	if (!GetEnvironmentVariable(ENV_PATH, (LPTSTR)BufVal.data(), BufSize))
		SYSFAIL(_T("Failed to get '%s' environment variable!"), ENV_PATH);

	BufVal.append(_T(";"));
	BufVal.append(Value);

	if (!SetEnvironmentVariable(ENV_PATH, BufVal.c_str()))
		SYSFAIL(_T("Failed to set '%s' environment variable!"), ENV_PATH);
}

#endif

TJVM::InitArgs::InitArgs(void) : TTypedBuffer() {
	JavaVMInitArgs &Args = *Refer();
	Args.version = JNI_VERSION_1_6;
	Args.ignoreUnrecognized = JNI_FALSE;
}

void TJVM::InitArgs::SetOpt(TJVMOptions &&xOpts) {
	Opts = std::move(xOpts);
	ConvOpts = TTypedBuffer<JavaVMOption>(sizeof(JavaVMOption)*Opts.size());
	for (size_t idx = 0; idx < Opts.size(); idx++) {
		JavaVMOption &Opt = (&ConvOpts)[idx];
		auto &TOpt = Opts.at(idx);
		Opt.optionString = const_cast<char*>(TOpt.Value.c_str());
		Opt.extraInfo = TOpt.Info;
	}
	JavaVMInitArgs &Args = *Refer();
	Args.nOptions = (jint)Opts.size();
	Args.options = &ConvOpts;
}

JavaVM* TJVM::AllocJVM(InitArgs const &Args, JNIEnv* &Env) {
	JavaVM* JVM;
	jint res = JNI_CreateJavaVM(&JVM, (void**)&Env, &Args);
	if (res != JNI_OK)
		FAIL(_T("Unable to load JVM (%d)"), res);
	return JVM;
}

void TJVM::FreeJVM(JavaVM* &Inst) {
	jint res = Inst->DestroyJavaVM();
	if (res != JNI_OK)
		FAIL(_T("Unable to unload JVM (%d)"), res);
}

TJVM::InitArgs TJVM::PrepareArgs(TString const &ClassPath, TString const &LocalJREPath, int DebugPort, int RemotePort) {
	if (!LocalJREPath.empty()) {
		AddSearchPath(TStringCast(LocalJREPath << _T(JRE_PLATFORM_PATHFRAG "\\bin\\server")));
	}

	TJVMOptions Opts;
#ifdef UNICODE
	{
		TString ConvErrMsg;
		CString ConvClassPath = WStringtoCString(CP_ACP, ClassPath, ConvErrMsg);
		if (!ConvErrMsg.empty()) FAIL(_T("Failed to convert class path - %s"), ConvErrMsg.c_str());
		Opts.emplace_back(CStringCast("-Djava.class.path=" << ConvClassPath.c_str()), nullptr);
	}
#else
	Opts.emplace_back(CStringCast("-Djava.class.path=" << ClassPath.c_str()), nullptr);
#endif

	if (DebugPort != 0)
		Opts.emplace_back(CStringCast("-agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=" << DebugPort), nullptr);

	if (RemotePort != 0) {
		Opts.emplace_back(CStringCast("-Dcom.sun.management.jmxremote=true"), nullptr);
		Opts.emplace_back(CStringCast("-Dcom.sun.management.jmxremote.authenticate=false"), nullptr);
		Opts.emplace_back(CStringCast("-Dcom.sun.management.jmxremote.ssl=false"), nullptr);
		Opts.emplace_back(CStringCast("-Dcom.sun.management.jmxremote.port=" << RemotePort), nullptr);
	}

	InitArgs Ret;
	Ret.SetOpt(std::move(Opts));
	return Ret;
}

JNIEnv* TJVMThreadEnv::Attach(TJVM &JVM, bool &Attached) {
	JNIEnv* Ret;
	jint res = (*JVM)->GetEnv((void **)&Ret, JNI_VERSION_1_6);
	Attached = res == JNI_EDETACHED;
	if (Attached)
		res = (*JVM)->AttachCurrentThread((void **)&Ret, NULL);
	if (res != JNI_OK)
		FAIL(_T("Unable to attach to JVM (%d)"), res);
	return Ret;
}

void TJVMThreadEnv::Detach(JNIEnv*& Inst) {
	if (Attached) {
		JavaVM* JVM = nullptr;
		jint res = Inst->GetJavaVM(&JVM);
		if (res != JNI_OK)
			FAIL(_T("Unable to acquire JVM context (%d)"), res);

		res = JVM->DetachCurrentThread();
		if (res != JNI_OK)
			FAIL(_T("Unable to detach from JVM (%d)"), res);
	}
}