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

/**
 * @addtogroup AdvUtils Advanced Supporting Utilities
 * @file
 * @brief JVM loading and initialization support
 * @author Zhenyu Wu
 * @date Sep 26, 2018: Refactored business logic independent code from AgentLib-NG
 **/

#ifndef ZWUtils_JVMLoader_H
#define ZWUtils_JVMLoader_H

#include "Misc/Global.h"

#include "Memory/Resource.h"

#include <vector>

//#include <jni.h>
// Forward declare so we do not depend on jni.h here
struct JavaVM_;
typedef JavaVM_ JavaVM;
struct JNIEnv_;
typedef JNIEnv_ JNIEnv;
struct JavaVMInitArgs;
struct JavaVMOption;

class TJVM : public TInitResource<JavaVM*> {
protected:
	JNIEnv *Env;

	struct TJVMOption {
		std::string Value;
		void* Info;

		TJVMOption(std::string &&xValue, void* xInfo) :
			Value(std::move(xValue)), Info(xInfo) {}
	};
	typedef std::vector<TJVMOption> TJVMOptions;
	class InitArgs : public TTypedBuffer<JavaVMInitArgs> {
		typedef InitArgs _this;
	protected:
		TJVMOptions Opts;
		TTypedBuffer<JavaVMOption> ConvOpts;
	public:
		InitArgs(void);

		InitArgs(_this &&xArgs) :
			TTypedBuffer(std::move(xArgs)), Opts(std::move(xArgs.Opts)), ConvOpts(std::move(xArgs.ConvOpts)) {}

		void SetOpt(TJVMOptions && xOpts);
	};

	static JavaVM* AllocJVM(InitArgs const &Args, JNIEnv* &Env);
	static void FreeJVM(JavaVM*& Inst);

	static InitArgs PrepareArgs(TString const &ClassPath, TString const &LocalJREPath, int DebugPort, int RemotePort);

public:
	// Note that currently only a single JVM instance can be created per process, due to limitation of JVM implementation
	TJVM(TString const &ClassPath, TString const &LocalJREPath = EMPTY_TSTRING(), int DebugPort = 0, int RemotePort = 0) :
		TInitResource(AllocJVM(PrepareArgs(ClassPath, LocalJREPath, DebugPort, RemotePort), Env), FreeJVM) {}

	// Do not use this environment to make calls, use TJVMThreadEnv instead
	JNIEnv* Environment(void)
	{ return Refer(), Env; }
};

// This class is supposed to be ALWAYS used by its constructor
// DO NOT pass reference to different thread, it won't work!
class TJVMThreadEnv : public TInitResource < JNIEnv* > {
	typedef TJVMThreadEnv _this;

protected:
	bool Attached;
	static JNIEnv* Attach(TJVM &JVM, bool &);
	void Detach(JNIEnv* &Inst);

public:
	TJVMThreadEnv(TJVM &xJVM) : TInitResource(Attach(xJVM, Attached), [&](JNIEnv* &Inst) { this->Detach(Inst); }) {}
};

#endif //ZWUtils_JVMLoader_H