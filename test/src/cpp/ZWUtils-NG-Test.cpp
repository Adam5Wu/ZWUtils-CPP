/*
Copyright (c) 2005 - 2017, Zhenyu Wu; 2012 - 2017, NEC Labs America Inc.
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

#include "Misc/Global.h"
#include "Misc/TString.h"
#include "Memory/Allocator.h"
#include "Debug/Logging.h"
#include "Debug/Exception.h"
#include "Debug/SysError.h"

#ifdef WINDOWS
#include <Windows.h>
#endif

void TestException();
void TestErrCode();
void TestStringConv();
void TestSize();
void TestTiming();
void TestDynBuffer();
void TestManagedObj();
void TestSyncPrems();
void TestSyncObj_1();
void TestWorkerThread();
void TestSyncObj_2(bool Robust = false);
void TestSyncQueue(bool Profiling = false);

#ifdef WINDOWS
int _tmain(int argc, _TCHAR* argv[])
#endif
{
	ControlSEHTranslation(true);

	//_LOG(_T("%s"), __REL_FILE__);
	try {
		if (argc != 2)
			FAIL(_T("Require 1 parameter: <TestType> = 'ALL' | ")
				_T("'Exception' / 'ErrCode' / 'StringConv' / 'SyncPrems' / 'DynBuffer' / ")
				_T("'ManagedObj' / 'SyncObj' / 'Size' / 'Timing' / 'WorkerThread' / 'SyncQueue'")
				_T("'SyncQueueProf' / 'SyncObjRobust'"));

		bool TestAll = _tcsicmp(argv[1], _T("ALL")) == 0;
		if (TestAll || (_tcsicmp(argv[1], _T("Exception")) == 0)) {
			TestException();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("ErrCode")) == 0)) {
			TestErrCode();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("StringConv")) == 0)) {
			TestStringConv();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("Size")) == 0)) {
			TestSize();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("Timing")) == 0)) {
			TestTiming();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("DynBuffer")) == 0)) {
			TestDynBuffer();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("ManagedObj")) == 0)) {
			TestManagedObj();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("SyncPrems")) == 0)) {
			TestSyncPrems();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("SyncObj")) == 0)) {
			TestSyncObj_1();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("WorkerThread")) == 0)) {
			TestWorkerThread();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("SyncObj")) == 0)) {
			TestSyncObj_2();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("SyncQueue")) == 0)) {
			TestSyncQueue();
		}
		if (_tcsicmp(argv[1], _T("SyncQueueProf")) == 0) {
			TestSyncQueue(true);
		}
		if (_tcsicmp(argv[1], _T("SyncObjRobust")) == 0) {
			TestSyncObj_2(true);
		}
	} catch (Exception *e) {
		e->Show();
		DefaultObjAllocator<Exception>().Destroy(e);
	}

#ifdef WINDOWS

	if (IsDebuggerPresent())
		system("pause");

#ifdef _DEBUG
	DefaultAllocator().Alloc(200);
	_LOG(_T("* Checking memory leaks... (expecting 1 block of 200 bytes)"));
	OutputDebugString(_T("* Checking memory leaks... (expecting 1 block of 200 bytes)") TNewLine);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF);
#endif

#endif

	return 0;
}

void TestException(void) {
	_LOG(_T("*** Test Exception throwing and catching"));
	try {
		FAIL(_T("XD"));
	} catch (Exception *e) {
		e->Show();
		DefaultObjAllocator<Exception>().Destroy(e);
	}

	_LOG(_T("*** Test SEH Exception translation"));
	try {
		int* P = nullptr;
		*P = 1; // Write access violation
		_LOG(_T("XD %d"), *P); // Read access violation
	} catch (SEHException *e) {
		e->Show();
		DefaultObjAllocator<SEHException>().Destroy(e);
	}

	_LOG(_T("*** Test Stack-traced Exception"));
	try {
		FAILST(_T("XD"));
	} catch (STException *e) {
		e->Show();
		DefaultObjAllocator<STException>().Destroy(e);
	}
}

void TestErrCode(void) {
	LOGS(_T("*** Test Error code decoding"));
	TCHAR Msg[2048];
	size_t BufLen = 2048;
	DecodeSysError(6, Msg, BufLen);
	LOGS(_T("Error 6: '%s' (%d, %d)"), Msg, _tcslen(Msg), BufLen);

	TString MsgStr;
	DecodeSysError(123, MsgStr);
	LOGS(_T("Error 123: %s (%d, %d)"), MsgStr.c_str(), _tcslen(MsgStr.c_str()), MsgStr.size());

	DecodeSysError(234, MsgStr);
	LOGS(_T("Error 234: %s (%d, %d)"), MsgStr.c_str(), _tcslen(MsgStr.c_str()), MsgStr.size());

#ifdef WINDOWS
	HMODULE ModuleHandle = GetModuleHandle(_T("ntdll.dll"));
	DecodeSysError(ModuleHandle, STATUS_INVALID_HANDLE, MsgStr);
	LOGS(_T("{ntdll.dll} Error %0.8x: %s (%d, %d)"), STATUS_INVALID_HANDLE, MsgStr.c_str(), _tcslen(MsgStr.c_str()), MsgStr.size());
#endif
}

void TestStringConv(void) {
	_LOG(_T("*** Test String Conversions"));

#ifdef UNICODE
	WString Test1{ L"This is a test..." };
	_LOG(_T("Converting: '%s'"), Test1.c_str());
	CString Test1C = WStringtoUTF8(Test1);
	WString Test1R = UTF8toWString(Test1C);
	if (Test1.compare(Test1R) != 0)
		FAIL(_T("String failed to round-trip!"));

	_wsetlocale(LC_ALL, L"chs");
	WString Test2{ L"这是一个测试..." };
	_LOG(_T("Converting: '%s'"), Test2.c_str());
	CString Test2C = WStringtoUTF8(Test2);
	TString Test2R = UTF8toWString(Test2C);
	if (Test2.compare(Test2R) != 0)
		FAIL(_T("String failed to round-trip!"));
	_wsetlocale(LC_ALL, ACP_LOCALE());
#else
	// TODO: need test case for non-unicode conversions
#endif
}

void TestSize(void) {
	_LOG(_T("*** Test Size"));
	_LOG(_T("1 MB: %s"), ToString(1, SizeUnit::MB).c_str());
	_LOG(_T(" - byte unit: %s"), ToString(1, SizeUnit::MB, false, true, SizeUnit::BYTE).c_str());
	_LOG(_T(" - KB unit: %s"), ToString(1, SizeUnit::MB, false, true, SizeUnit::KB).c_str());

	long long S1G1K = Convert(1, SizeUnit::GB, SizeUnit::BYTE) + Convert(1, SizeUnit::KB, SizeUnit::BYTE);
	_LOG(_T("1 GB + 1KB: %s"), ToString(S1G1K, SizeUnit::BYTE).c_str());
	_LOG(_T(" - MB unit: %s"), ToString(S1G1K, SizeUnit::BYTE, false, true, SizeUnit::MB, SizeUnit::MB).c_str());
	_LOG(_T(" - GB-MB unit: %s"), ToString(S1G1K, SizeUnit::BYTE, false, true, SizeUnit::GB, SizeUnit::MB).c_str());

	long long S1G1KN1B = S1G1K - 1;
	_LOG(_T("1 GB + 1KB - 1B: %s"), ToString(S1G1KN1B, SizeUnit::BYTE).c_str());
	_LOG(_T(" - MB unit: %s"), ToString(S1G1KN1B, SizeUnit::BYTE, false, true, SizeUnit::MB, SizeUnit::MB).c_str());
	_LOG(_T(" - GB-MB unit: %s"), ToString(S1G1KN1B, SizeUnit::BYTE, false, true, SizeUnit::GB, SizeUnit::MB).c_str());
	_LOG(_T(" - GB-MB unit, abbreviate: %s"), ToString(S1G1KN1B, SizeUnit::BYTE, true, true, SizeUnit::GB, SizeUnit::MB).c_str());
}

void TestTiming(void) {
	_LOG(_T("*** Test Timing"));
	TimeStamp Now(TimeStamp::Now());

	_LOG(_T("--- Timestamp"));
	_LOG(_T("Current time: %s"), Now.toString().c_str());
	_LOG(_T(" = second resolution: %s"), Now.toString(TimeUnit::SEC).c_str());
	_LOG(_T(" = minute resolution: %s"), Now.toString(TimeUnit::MIN).c_str());
	_LOG(_T(" = hour resolution: %s"), Now.toString(TimeUnit::HR).c_str());
	_LOG(_T(" = day resolution: %s"), Now.toString(TimeUnit::DAY).c_str());

	_LOG(_T("--- Timespan"));
	TimeSpan TS3S = TimeSpan(3, TimeUnit::SEC);
	_LOG(_T("3 second timespan: %s"), TS3S.toString().c_str());
	_LOG(_T(" = millisecond resolution: %s"), TS3S.toString(TimeUnit::MSEC, false).c_str());
	_LOG(_T(" = millisecond resolution, abbreviate: %s"), TS3S.toString(TimeUnit::MSEC).c_str());
	_LOG(_T(" = minute resolution: %s"), TS3S.toString(false, true, TimeUnit::MIN).c_str());
	_LOG(_T(" = minute-millisecond resolution: %s"), TS3S.toString(false, true, TimeUnit::MIN, TimeUnit::MSEC).c_str());

	TimeSpan TS3M = TimeSpan(3, TimeUnit::MIN);
	_LOG(_T("3 minute timespan: %s"), TS3M.toString().c_str());
	_LOG(_T(" = millisecond resolution: %s"), TS3M.toString(TimeUnit::MSEC, false).c_str());
	_LOG(_T(" = second-millisecond resolution: %s"), TS3M.toString(false, true, TimeUnit::MIN, TimeUnit::MSEC).c_str());

	_LOG(_T("--- Time arithmetics"));
	TimeSpan TS3D = TimeSpan(3, TimeUnit::DAY);
	auto TS3D3M = TS3D + TS3M;
	_LOG(_T("3 days + 3 minute timespan: %s"), TS3D3M.toString().c_str());
	_LOG(_T(" = hour-millisecond resolution: %s"), TS3D3M.toString(false, true, TimeUnit::HR, TimeUnit::MSEC).c_str());
	_LOG(_T(" = day-millisecond resolution: %s"), TS3D3M.toString(false, true, TimeUnit::DAY, TimeUnit::MSEC).c_str());
	_LOG(_T(" = day-hour resolution: %s"), TS3D3M.toString(false, true, TimeUnit::DAY, TimeUnit::HR).c_str());
	_LOG(_T(" = day-hour resolution, abbreviate: %s"), TS3D3M.toString(true, true, TimeUnit::DAY, TimeUnit::HR).c_str());

	auto TS3D3MN3S = TS3D3M - TS3S;
	_LOG(_T("3 days + 3 minute - 3 seconds second timespan: %s"), TS3D3MN3S.toString().c_str());
	_LOG(_T(" = hour-millisecond resolution: %s"), TS3D3MN3S.toString(false, true, TimeUnit::HR, TimeUnit::MSEC).c_str());
	_LOG(_T(" = day-millisecond resolution: %s"), TS3D3MN3S.toString(false, true, TimeUnit::DAY, TimeUnit::MSEC).c_str());
	_LOG(_T(" = day-hour resolution: %s"), TS3D3MN3S.toString(false, true, TimeUnit::DAY, TimeUnit::HR).c_str());
	_LOG(_T(" = day-hour resolution, abbreviate: %s"), TS3D3MN3S.toString(true, true, TimeUnit::DAY, TimeUnit::HR).c_str());

	_LOG(_T("Current time + 3 seconds: %s"), Now.Offset(TS3S).toString().c_str());
	_LOG(_T("Current time - 3 minute: %s"), Now.Offset(-TS3M).toString().c_str());
	_LOG(_T("Current time + 3 days: %s"), Now.Offset(TS3D).toString().c_str());
	_LOG(_T("Current time + 3 days + 3 minute - 3 seconds: %s"), Now.Offset(TS3D3MN3S).toString().c_str());
}

#include "Memory/ManagedObj.h"
#include "Memory/ManagedRef.h"

void TestManagedObj() {
	_LOG(_T("*** Test Managed Objects"));
	_LOG(_T("--- Manage Object"));
	class TestMObj : public ManagedObj {
	protected:
		TString const Name;
	public:
		TestMObj(TString const &xName) : Name(xName) { _LOG(_T("MObj '%s' Created"), Name.c_str()); }
		virtual ~TestMObj(void) { _LOG(_T("MObj '%s' Destroyed"), Name.c_str()); }
	};
	auto A = DefaultObjAllocator<TestMObj>().Create(RLAMBDANEW(TestMObj, _T("A")));
	_LOG(_T("A: %s"), A->toString().c_str());
	DefaultObjAllocator<TestMObj>().Destroy(A);

	class TestMXObj : public ManagedObj {
	protected:
		TString const Name;
	public:
		TestMXObj(TString const &xName) : Name(xName) { FAIL(_T("MXObj Creation Failed!")); }
		virtual ~TestMXObj(void) { _LOG(_T("MXObj '%s' Destroyed"), Name.c_str()); }
	};
	_LOG(_T("Creating MXObj (Expect exception)"));
	try {
		auto AX = DefaultObjAllocator<TestMXObj>().Create(RLAMBDANEW(TestMXObj, _T("AX")));
		FAIL(_T("Should not reach"));
	} catch (Exception *e) {
		ManagedRef<Exception> E(e, CONSTRUCTION::HANDOFF);
		E->Show();
	}

	_LOG(_T("--- Manage Object Adapter"));
	class TestPObj {
	protected:
		TString const Name;
	public:
		TestPObj(TString const &xName) : Name(xName) { _LOG(_T("PObj '%s' Created"), Name.c_str()); }
		virtual ~TestPObj(void) { _LOG(_T("PObj '%s' Destroyed"), Name.c_str()); }
		virtual TString toString(void) const { return { _T("TestPObj") }; }
	};
	auto B = ManagedObjAdapter<TestPObj>::Create(_T("B"));
	_LOG(_T("B: %s"), B->toString().c_str());
	DefaultObjAllocator<TestPObj>().Destroy(B);

	class TestPXObj {
	protected:
		TString const Name;
	public:
		TestPXObj(TString const &xName) : Name(xName) { _LOG(_T("PXObj '%s' Created"), Name.c_str()); }
		virtual ~TestPXObj(void) { _LOG(_T("PXObj '%s' Destroyed"), Name.c_str()); }
	};
	auto BX = ManagedObjAdapter<TestPXObj>::Create(_T("BX"));
	_LOG(_T("BX: %s"), dynamic_cast<ManagedObjAdapter<TestPXObj>*>(BX)->toString().c_str());
	DefaultObjAllocator<TestPXObj>().Destroy(BX);

	_LOG(_T("--- Clonable Object"));
	class TestCObj : public Cloneable {
	protected:
		TString const Name;
		IObjAllocator<TestCObj> & _Alloc = DefaultObjAllocator<TestCObj>();
		Cloneable* MakeClone(IObjAllocator<void> &xAlloc) const override {
			_LOG(_T("CObj '%s' Cloning..."), Name.c_str());
			return ((IObjAllocator<TestCObj>&)xAlloc).Transfer(_Alloc.Create(
				[&](void *X) {return new (X)TestCObj(Name); }), _Alloc);
		}
	public:
		TestCObj(TString const &xName) : Name(xName) { _LOG(_T("CObj '%s' Created"), Name.c_str()); }
		virtual ~TestCObj(void) { _LOG(_T("CObj '%s' Destroyed"), Name.c_str()); }
		virtual TString toString(void) const { return { _T("TestCObj") }; }
	};
	auto C1 = DefaultObjAllocator<TestCObj>().Create(RLAMBDANEW(TestCObj, _T("C")));
	_LOG(_T("C1: %s"), C1->toString().c_str());
	auto C2 = dynamic_cast<TestCObj*>(TestCObj::Clone(C1));
	_LOG(_T("C2: %s"), C2->toString().c_str());
	DefaultObjAllocator<TestCObj>().Destroy(C1);
	DefaultObjAllocator<TestCObj>().Destroy(C2);

	_LOG(_T("--- Managed References"));
	{
		_LOG(_T("- Creating Native ManagedObj"));
		ManagedRef<TestMObj> MR1(CONSTRUCTION::EMPLACE, _T("D"));
		_LOG(_T("MR1: %s"), MR1->toString().c_str());
		_LOG(_T("- MR2 = MR1 (Expect reference increase)"));
		ManagedRef<TestMObj> MR2 = MR1;
		_LOG(_T("MR1: %s"), MR1->toString().c_str());
		_LOG(_T("MR2: %s"), MR2->toString().c_str());
		_LOG(_T("* Releasing MR1 and MR2 (Expect object deletion)"));
	}

	{
		_LOG(_T("- Creating Clonable Object"));
		ManagedRef<TestCObj> MR3(CONSTRUCTION::EMPLACE, _T("F"));
		_LOG(_T("MR3: %s"), MR3->toString().c_str());
		_LOG(_T("- MR3 = MR4 (Expect object clone)"));
		ManagedRef<TestCObj> MR4 = MR3;
		_LOG(_T("MR3: %s"), MR3->toString().c_str());
		_LOG(_T("MR4: %s"), MR4->toString().c_str());
		_LOG(_T("* Releasing MR3 and MR4 (Expect object deletions)"));
	}

	{
		_LOG(_T("- Creating Plain Object"));
		ManagedRef<TestPObj> MR5(CONSTRUCTION::EMPLACE, _T("G"));
		_LOG(_T("MR5: %s"), MR5->toString().c_str());
		_LOG(_T("- MR6 = MR5 (Expect exception)"));
		try {
			ManagedRef<TestPObj> MR6 = MR5;
			FAIL(_T("Should not reach"));
		} catch (Exception *e) {
			ManagedRef<Exception> E(e, CONSTRUCTION::HANDOFF);
			E->Show();
		}
		_LOG(_T("* Releasing MR5 (Expect object deletions)"));
	}

	{
		_LOG(_T("- Creating ManagedObj-Adapted Plain Object"));
		ManagedRef<TestPObj> MR7(ManagedObjAdapter<TestPObj>::Create(_T("H")));
		_LOG(_T("MR7: %s"), dynamic_cast<ManagedObjAdapter<TestPObj>*>(&MR7)->toString(true).c_str());
		_LOG(_T("- MR8 = MR7 (Expect reference increase)"));
		ManagedRef<TestPObj> MR8 = MR7;
		_LOG(_T("MR7: %s"), dynamic_cast<ManagedObjAdapter<TestPObj>*>(&MR7)->toString(true).c_str());
		_LOG(_T("MR8: %s"), dynamic_cast<ManagedObjAdapter<TestPObj>*>(&MR8)->toString(true).c_str());
		_LOG(_T("* Releasing MR7 and MR8 (Expect object deletion)"));
	}
}

#include "Threading/SyncElements.h"

void TestSyncPrems() {
	_LOG(_T("*** Test Synchronization Premises"));
	TInterlockedArchInt A{ 0 };
	__ARC_INT B = 55;

	_LOG(_T("--- Assignment"));
	_LOG(_T("A: %d"), ~A);
	_LOG(_T("B = %d"), B);
	_LOG(_T("A = B : %d"), A = B);

	_LOG(_T("--- Increment"));
	_LOG(_T("A = %d"), ~A);
	_LOG(_T("A++ = %d"), A++);
	_LOG(_T("A = %d"), ~A);
	_LOG(_T("++A = %d"), ++A);
	_LOG(_T("A = %d"), ~A);
	_LOG(_T("A+= 10 : %d"), A += (__ARC_INT)10);
	_LOG(_T("A-= 5 : %d"), A -= (__ARC_INT)5);

	_LOG(_T("--- Exchange"));
	__ARC_INT C = ~A;
	_LOG(_T("A = %d"), ~A);
	_LOG(_T("B = %d"), B);
	_LOG(_T("A <-> B : %d"), A.Exchange(B));
	_LOG(_T("A = %d"), ~A);
	_LOG(_T("B = %d"), B);

	_LOG(_T("--- ExchangeAdd"));
	_LOG(_T("A = %d"), ~A);
	_LOG(_T("C = %d"), C);
	_LOG(_T("A <+> C : %d"), A.ExchangeAdd(C));
	_LOG(_T("A = %d"), ~A);
	_LOG(_T("C = %d"), C);

	_LOG(_T("--- Compare and Swap"));
	__ARC_INT D = ~A;
	_LOG(_T("A = %d"), ~A);
	_LOG(_T("B = %d"), B);
	_LOG(_T("D = %d"), D);
	_LOG(_T("A <=D=> B : %d"), A.CompareAndSwap(D, B));
	_LOG(_T("A = %d"), ~A);
	_LOG(_T("B = %d"), B);
	_LOG(_T("D = %d"), D);
	_LOG(_T("++A = %d"), ++A);
	_LOG(_T("A <=D=> B : %d"), A.CompareAndSwap(D, B));
	_LOG(_T("A = %d"), ~A);
	_LOG(_T("B = %d"), B);
	_LOG(_T("D = %d"), D);
}

#include "Memory/Resource.h"

void TestDynBuffer() {
	_LOG(_T("*** Test Dynamic Buffers"));

	_LOG(_T("--- Allocation and deallocation"));
	{
		TDynBuffer TVoidDynBuffer;
		_LOG(_T("Void null buffer: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));
		_LOG(_T("Buffer pointer = %p"), &TVoidDynBuffer);
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Increase to 10B: %s"), TVoidDynBuffer.SetSize(10) ? _T("Success") : _T("Fail"));
		memset(&TVoidDynBuffer, 'A', 9);
		((char*)&TVoidDynBuffer)[9] = '\0';
		_LOG(_T("Filled with 'A' = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Extend to 2KB: %s, %s"), TVoidDynBuffer.SetSize(2048) ? _T("Success") : _T("Fail"),
			TStringCast((char*)&TVoidDynBuffer).c_str());
		memset(((char*)&TVoidDynBuffer) + 9, 'B', 9);
		((char*)&TVoidDynBuffer)[18] = '\0';
		_LOG(_T("Append with 9x'B' = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Shrink to 10B: %s"), TVoidDynBuffer.SetSize(10) ? _T("Success") : _T("Fail"));
		((char*)&TVoidDynBuffer)[9] = '\0';
		_LOG(_T("Terminate at byte 10 = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Extend to 12KB: %s"), TVoidDynBuffer.SetSize(12 * 1024) ? _T("Success") : _T("Fail"));
		_LOG(_T("Buffer pointer = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Extend to 14KB: %s"), TVoidDynBuffer.SetSize(14 * 1024) ? _T("Success") : _T("Fail"));
		_LOG(_T("Buffer pointer = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Shrink to 8KB: %s"), TVoidDynBuffer.SetSize(8 * 1024) ? _T("Success") : _T("Fail"));
		_LOG(_T("Buffer pointer = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Shrink to 4KB: %s"), TVoidDynBuffer.SetSize(4 * 1024) ? _T("Success") : _T("Fail"));
		_LOG(_T("Buffer pointer = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Shrink to 10B: %s"), TVoidDynBuffer.SetSize(10) ? _T("Success") : _T("Fail"));
		_LOG(_T("Buffer pointer = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Extend to 12KB: %s"), TVoidDynBuffer.SetSize(12 * 1024) ? _T("Success") : _T("Fail"));
		_LOG(_T("Buffer pointer = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Extend to 12MB: %s"), TVoidDynBuffer.SetSize(12 * 1024 * 1024) ? _T("Success") : _T("Fail"));
		_LOG(_T("Buffer pointer = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Extend to 10MB: %s"), TVoidDynBuffer.SetSize(10 * 1024 * 1024) ? _T("Success") : _T("Fail"));
		_LOG(_T("Buffer pointer = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Extend to 14MB: %s"), TVoidDynBuffer.SetSize(14 * 1024 * 1024) ? _T("Success") : _T("Fail"));
		_LOG(_T("Buffer pointer = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Shrink to 10B: %s"), TVoidDynBuffer.SetSize(10) ? _T("Success") : _T("Fail"));
		_LOG(_T("Buffer pointer = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		_LOG(_T("Extend to 8MB: %s"), TVoidDynBuffer.SetSize(8 * 1024 * 1024) ? _T("Success") : _T("Fail"));
		_LOG(_T("Buffer pointer = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));
	}

}

#include "Threading/SyncObjects.h"

struct Integer {
	int value;
	Integer() {}
	Integer(int i) : value(i) {}
	Integer(Integer const &v) : value(v.value) {}

	int operator++(int) { return value++; }
	int operator=(int i) { return value = i; }
	Integer& operator=(Integer const &v) { return value = v.value, *this; }

	bool CompareAndSwap(Integer const &comp, Integer &target) {
		bool Ret = value == comp.value;
		int xvalue = Ret ? value ^ target.value : 0;
		return value ^= xvalue, target.value ^= xvalue, Ret;
	}
};
bool operator==(Integer const &A, Integer const &B) {
	return A.value == B.value;
}

typedef TSyncObj<Integer> TSyncInteger;

void TestSyncObj_1(void) {
	_LOG(_T("*** Test SyncObj (Non-threading correctness)"));
	TSyncInteger A;

	_LOG(_T("--- Pickup (Successful)"));
	{
		auto SA(A.Pickup());
		_LOG(_T("Pickup : %s"), SA.toString().c_str());
		_LOG(_T("A : %d"), SA->value);
	}

	_LOG(_T("--- Pickup (Null access)"));
	{
		auto SA(A.NullAccessor());
		_LOG(_T("Pickup : %s"), SA.toString().c_str());
		_LOG(_T("Try operating unlocked object (Expect exception)"));
		try {
			_LOG(_T("A : %d"), SA->value);
			FAIL(_T("Should not reach"));
		} catch (Exception *e) {
			ManagedRef<Exception> E(e, CONSTRUCTION::HANDOFF);
			E->Show();
		}
	}

	_LOG(_T("--- Increment by one"));
	{
		auto SA(A.Pickup());
		_LOG(_T("A : %d"), SA->value);
		_LOG(_T("A++ : %d"), (*A.Pickup())++);
		_LOG(_T("A : %d"), SA->value);
	}

	_LOG(_T("--- Compare and Swap (Success)"));
	_LOG(_T("A = 50"));
	*A.Pickup() = 50;
	_LOG(_T("D = 55"));
	Integer D{ 55 };
	_LOG(_T("A <=50=> D : %d"), A.Pickup()->CompareAndSwap(50, D));
	_LOG(_T("A : %d"), A.Pickup()->value);
	_LOG(_T("D : %d"), D.value);

	_LOG(_T("--- Compare and Swap (Fail)"));
	_LOG(_T("D = 60"));
	D = 60;
	_LOG(_T("A : %d"), A.Pickup()->value);
	_LOG(_T("A <=50=> D : %d"), A.Pickup()->CompareAndSwap(50, D));
	_LOG(_T("A : %d"), A.Pickup()->value);
	_LOG(_T("D : %d"), D.value);
}

#include "Threading/WorkerThread.h"

void TestWorkerThread() {
	class TestRunnable : public TRunnable {
	protected:
		TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) override {
			_LOG(_T("Yee Hah!"));
			if (*Arg != nullptr) {
				_LOG(_T("Throwing exception..."));
				FAIL(_T("Test!"));
			}
			return { nullptr };
		}
	};

	_LOG(_T("*** Test WorkerThread (Normal)"));
	{
		MRWorkerThread A(TWorkerThread::Create(_T("TestA"), DefaultObjAllocator<TestRunnable>().Create()), CONSTRUCTION::HANDOFF);
		A->Start();
		A->WaitFor();
		_LOG(_T("Return data: %s"), TStringCast(A->ReturnData()).c_str());
		if (A->FatalException() != nullptr) {
			_LOG(_T("WARNING: Thread crashed - %s"), A->FatalException()->Why().c_str());
		}
	}

	_LOG(_T("*** Test WorkerThread (Exception during run)"));
	{
		MRWorkerThread B(TWorkerThread::Create(_T("TestB"), DefaultObjAllocator<TestRunnable>().Create()), CONSTRUCTION::HANDOFF);
		B->Start(TFixedBuffer(DefaultAllocator().Alloc(100)));
		B->WaitFor();
		_LOG(_T("Return data: %s"), TStringCast(B->ReturnData()).c_str());
		if (B->FatalException() != nullptr) {
			_LOG(_T("Thread crashed - %s"), B->FatalException()->Why().c_str());
		} else {
			FAIL(_T("Expect thread crash, found no fatal exception"));
		}
	}

	_LOG(_T("*** Test WorkerThread (Normal, Self-free)"));
	{
		TWorkerThread::Create(_T("TestC"), DefaultObjAllocator<TestRunnable>().Create(), true)->Start();
		TDelayWaitable WaitASec(500);
		WaitASec.WaitFor(FOREVER);
		_LOG(_T("Expect the worker thread has terminated and destroyed by now..."));
	}

	_LOG(_T("*** Test WorkerThread (Exception, Self-free)"));
	{
		TWorkerThread::Create(_T("TestD"), DefaultObjAllocator<TestRunnable>().Create(), true)->Start(DefaultAllocator().Alloc(100));
		TDelayWaitable WaitASec(500);
		WaitASec.WaitFor(FOREVER);
		_LOG(_T("Expect the worker thread has terminated and destroyed by now..."));
	}

	class TestSelfDestroyRunnable : public TRunnable {
	protected:
		TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) override {
			_LOG(_T("Yee Hah!"));
			DefaultObjAllocator<TWorkerThread>().Destroy(std::addressof(WorkerThread));
			return { nullptr };
		}
	};

	_LOG(_T("*** Test WorkerThread (Self-destroy)"));
	{
		MRWorkerThread E(TWorkerThread::Create(_T("TestE"), DefaultObjAllocator<TestSelfDestroyRunnable>().Create()), CONSTRUCTION::HANDOFF);
		E->Start();
		E->WaitFor();
		_LOG(_T("Return data: %s"), TStringCast(E->ReturnData()).c_str());
		if (E->FatalException() != nullptr) {
			_LOG(_T("Thread crashed - %s"), E->FatalException()->Why().c_str());
		} else {
			FAIL(_T("Expect thread crash, found no fatal exception"));
		}
	}

	_LOG(_T("*** Test WorkerThread (Terminate before run)"));
	{
		MRWorkerThread F(TWorkerThread::Create(_T("TestF"), DefaultObjAllocator<TestRunnable>().Create()), CONSTRUCTION::HANDOFF);
		F->SignalTerminate();
		F->WaitFor();
		_LOG(_T("Return data: %s"), TStringCast(F->ReturnData()).c_str());
		if (F->FatalException() != nullptr) {
			FAIL(_T("Thread crashed - %s"), F->FatalException()->Why().c_str());
		}
	}

	_LOG(_T("*** Test WorkerThread (State notification)"));
	{
		{
			auto ConstructEvent = TWorkerThread::GStateNotify(_T("TestConstructed"), TWorkerThread::State::Constructed,
				[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
					_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
				});
			auto InitializingEvent = TWorkerThread::GStateNotify(_T("TestInitializing"), TWorkerThread::State::Initialzing,
				[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
					_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
				});
			auto RunningEvent = TWorkerThread::GStateNotify(_T("TestRunning"), TWorkerThread::State::Running,
				[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
					_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
				});
			auto TerminatingEvent = TWorkerThread::GStateNotify(_T("TestTerminating"), TWorkerThread::State::Terminating,
				[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
					_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
				});
			auto TerminatedEvent = TWorkerThread::GStateNotify(_T("TestTerminated"), TWorkerThread::State::Terminated,
				[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
					_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
				});
			MRWorkerThread G(TWorkerThread::Create(_T("TestG"), DefaultObjAllocator<TestRunnable>().Create()), CONSTRUCTION::HANDOFF);
			G->SignalTerminate();
			G->WaitFor();
			_LOG(_T("Return data: %s"), TStringCast(G->ReturnData()).c_str());
			if (G->FatalException() != nullptr) {
				FAIL(_T("Thread crashed - %s"), G->FatalException()->Why().c_str());
			}
		}
		class TestDelayRunnable : public TRunnable {
		protected:
			TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) override {
				_LOG(_T("Yee Hah!"));
				Sleep(1000);
				return nullptr;
			}
		};
		{
			MRWorkerThread H(TWorkerThread::Create(_T("TestH"), DefaultObjAllocator<TestDelayRunnable>().Create()), CONSTRUCTION::HANDOFF);
			auto InitializingEvent = H->StateNotify(_T("TestInitializing"), TWorkerThread::State::Initialzing,
				[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
					_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
				});
			auto RunningEvent = H->GStateNotify(_T("TestRunning"), TWorkerThread::State::Running,
				[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
					_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
				});
			auto TerminatingEvent = H->GStateNotify(_T("TestTerminating"), TWorkerThread::State::Terminating,
				[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
					_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
				});
			H->Start();
			H->WaitFor(500);
			H->SignalTerminate();
			H->WaitFor();
			_LOG(_T("Return data: %s"), TStringCast(H->ReturnData()).c_str());
			if (H->FatalException() != nullptr) {
				FAIL(_T("Thread crashed - %s"), H->FatalException()->Why().c_str());
			}
		}
		{
			auto TerminatingEvent = TWorkerThread::GStateNotify(_T("TestTerminating"), TWorkerThread::State::Terminating,
				[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
					_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
				});
			MRWorkerThread I(TWorkerThread::Create(_T("TestI"), DefaultObjAllocator<TestDelayRunnable>().Create()), CONSTRUCTION::HANDOFF);
			auto TerminatedEvent = I->StateNotify(_T("TestTerminated"), TWorkerThread::State::Terminated,
				[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
					_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
				});
			I->Start();
			_LOG(_T("Going out-of-scope, expect signal terminate and wait for 1 second (no local event notification)..."));
		}
	}
}

void TestSyncObj_2(bool Robust) {
	if (Robust) {
		_LOG(_T("*** Test SyncObj (Threading correctness, robust version)"));
		{
			TSyncInteger X(0);
			ExtAllocator NullAlloc;
			_LOG(_T("X = %d"), X.Pickup()->value);

			class TestCount : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncInt) override {
					int COUNT = 1000000;
					TSyncInteger* Ctr = static_cast<TSyncInteger*>(*pSyncInt);
					for (int i = 0; i < COUNT; i++) {
						(*Ctr->Pickup())++;
					}
					_LOG(_T("Count of %d done!"), COUNT);
					return { nullptr };
				}
			};

			MRWorkerThread TestCount00(CONSTRUCTION::EMPLACE, _T("CounterThread00"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount01(CONSTRUCTION::EMPLACE, _T("CounterThread01"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount02(CONSTRUCTION::EMPLACE, _T("CounterThread02"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount03(CONSTRUCTION::EMPLACE, _T("CounterThread03"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount04(CONSTRUCTION::EMPLACE, _T("CounterThread04"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount05(CONSTRUCTION::EMPLACE, _T("CounterThread05"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount06(CONSTRUCTION::EMPLACE, _T("CounterThread06"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount07(CONSTRUCTION::EMPLACE, _T("CounterThread07"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount08(CONSTRUCTION::EMPLACE, _T("CounterThread08"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount09(CONSTRUCTION::EMPLACE, _T("CounterThread09"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0A(CONSTRUCTION::EMPLACE, _T("CounterThread0A"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0B(CONSTRUCTION::EMPLACE, _T("CounterThread0B"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0C(CONSTRUCTION::EMPLACE, _T("CounterThread0C"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0D(CONSTRUCTION::EMPLACE, _T("CounterThread0D"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0E(CONSTRUCTION::EMPLACE, _T("CounterThread0E"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0F(CONSTRUCTION::EMPLACE, _T("CounterThread0F"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount10(CONSTRUCTION::EMPLACE, _T("CounterThread10"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount11(CONSTRUCTION::EMPLACE, _T("CounterThread11"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount12(CONSTRUCTION::EMPLACE, _T("CounterThread12"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount13(CONSTRUCTION::EMPLACE, _T("CounterThread13"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount14(CONSTRUCTION::EMPLACE, _T("CounterThread14"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount15(CONSTRUCTION::EMPLACE, _T("CounterThread15"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount16(CONSTRUCTION::EMPLACE, _T("CounterThread16"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount17(CONSTRUCTION::EMPLACE, _T("CounterThread17"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount18(CONSTRUCTION::EMPLACE, _T("CounterThread18"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount19(CONSTRUCTION::EMPLACE, _T("CounterThread19"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1A(CONSTRUCTION::EMPLACE, _T("CounterThread1A"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1B(CONSTRUCTION::EMPLACE, _T("CounterThread1B"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1C(CONSTRUCTION::EMPLACE, _T("CounterThread1C"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1D(CONSTRUCTION::EMPLACE, _T("CounterThread1D"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1E(CONSTRUCTION::EMPLACE, _T("CounterThread1E"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1F(CONSTRUCTION::EMPLACE, _T("CounterThread1F"), DefaultObjAllocator<TestCount>().Create());

			_LOG(_T("--- Start Counting... (takes about 5 seconds)"));
			TestCount00->Start({ &X, NullAlloc }); TestCount01->Start({ &X, NullAlloc }); TestCount02->Start({ &X, NullAlloc }); TestCount03->Start({ &X, NullAlloc });
			TestCount04->Start({ &X, NullAlloc }); TestCount05->Start({ &X, NullAlloc }); TestCount06->Start({ &X, NullAlloc }); TestCount07->Start({ &X, NullAlloc });
			TestCount08->Start({ &X, NullAlloc }); TestCount09->Start({ &X, NullAlloc }); TestCount0A->Start({ &X, NullAlloc }); TestCount0B->Start({ &X, NullAlloc });
			TestCount0C->Start({ &X, NullAlloc }); TestCount0D->Start({ &X, NullAlloc }); TestCount0E->Start({ &X, NullAlloc }); TestCount0F->Start({ &X, NullAlloc });
			TestCount10->Start({ &X, NullAlloc }); TestCount11->Start({ &X, NullAlloc }); TestCount12->Start({ &X, NullAlloc }); TestCount13->Start({ &X, NullAlloc });
			TestCount14->Start({ &X, NullAlloc }); TestCount15->Start({ &X, NullAlloc }); TestCount16->Start({ &X, NullAlloc }); TestCount17->Start({ &X, NullAlloc });
			TestCount18->Start({ &X, NullAlloc }); TestCount19->Start({ &X, NullAlloc }); TestCount1A->Start({ &X, NullAlloc }); TestCount1B->Start({ &X, NullAlloc });
			TestCount1C->Start({ &X, NullAlloc }); TestCount1D->Start({ &X, NullAlloc }); TestCount1E->Start({ &X, NullAlloc }); TestCount1F->Start({ &X, NullAlloc });

			WaitMultiple({
				*TestCount00, *TestCount01, *TestCount02, *TestCount03,
				*TestCount04, *TestCount05, *TestCount06, *TestCount07,
				*TestCount08, *TestCount09, *TestCount0A, *TestCount0B,
				*TestCount0C, *TestCount0D, *TestCount0E, *TestCount0F,
				*TestCount10, *TestCount11, *TestCount12, *TestCount13,
				*TestCount14, *TestCount15, *TestCount16, *TestCount17,
				*TestCount18, *TestCount19, *TestCount1A, *TestCount1B,
				*TestCount1C, *TestCount1D, *TestCount1E, *TestCount1F
				}, true);

			_LOG(_T("--- Finished All Counting..."));
			_LOG(_T("X = %d"), X.Pickup()->value);
		}
		{
			TSyncInteger X(0);
			ExtAllocator NullAlloc;
			_LOG(_T("X = %d"), X.Pickup()->value);

			class TestCount : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncInt) override {
					int COUNT = 1000000;
					TSyncInteger* Ctr = static_cast<TSyncInteger*>(*pSyncInt);
					for (int i = 0; i < COUNT; i++) {
						(*Ctr->Pickup())++;
					}
					_LOG(_T("Count of %d done!"), COUNT);
					return { nullptr };
				}
			};

			MRWorkerThread TestCount00(CONSTRUCTION::EMPLACE, _T("CounterThread00"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount01(CONSTRUCTION::EMPLACE, _T("CounterThread01"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount02(CONSTRUCTION::EMPLACE, _T("CounterThread02"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount03(CONSTRUCTION::EMPLACE, _T("CounterThread03"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount04(CONSTRUCTION::EMPLACE, _T("CounterThread04"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount05(CONSTRUCTION::EMPLACE, _T("CounterThread05"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount06(CONSTRUCTION::EMPLACE, _T("CounterThread06"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount07(CONSTRUCTION::EMPLACE, _T("CounterThread07"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount08(CONSTRUCTION::EMPLACE, _T("CounterThread08"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount09(CONSTRUCTION::EMPLACE, _T("CounterThread09"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0A(CONSTRUCTION::EMPLACE, _T("CounterThread0A"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0B(CONSTRUCTION::EMPLACE, _T("CounterThread0B"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0C(CONSTRUCTION::EMPLACE, _T("CounterThread0C"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0D(CONSTRUCTION::EMPLACE, _T("CounterThread0D"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0E(CONSTRUCTION::EMPLACE, _T("CounterThread0E"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0F(CONSTRUCTION::EMPLACE, _T("CounterThread0F"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount10(CONSTRUCTION::EMPLACE, _T("CounterThread10"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount11(CONSTRUCTION::EMPLACE, _T("CounterThread11"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount12(CONSTRUCTION::EMPLACE, _T("CounterThread12"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount13(CONSTRUCTION::EMPLACE, _T("CounterThread13"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount14(CONSTRUCTION::EMPLACE, _T("CounterThread14"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount15(CONSTRUCTION::EMPLACE, _T("CounterThread15"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount16(CONSTRUCTION::EMPLACE, _T("CounterThread16"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount17(CONSTRUCTION::EMPLACE, _T("CounterThread17"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount18(CONSTRUCTION::EMPLACE, _T("CounterThread18"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount19(CONSTRUCTION::EMPLACE, _T("CounterThread19"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1A(CONSTRUCTION::EMPLACE, _T("CounterThread1A"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1B(CONSTRUCTION::EMPLACE, _T("CounterThread1B"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1C(CONSTRUCTION::EMPLACE, _T("CounterThread1C"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1D(CONSTRUCTION::EMPLACE, _T("CounterThread1D"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1E(CONSTRUCTION::EMPLACE, _T("CounterThread1E"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1F(CONSTRUCTION::EMPLACE, _T("CounterThread1F"), DefaultObjAllocator<TestCount>().Create());

			_LOG(_T("--- Start Counting... (takes about 5 seconds)"));
			TestCount00->Start({ &X, NullAlloc }); TestCount01->Start({ &X, NullAlloc }); TestCount02->Start({ &X, NullAlloc }); TestCount03->Start({ &X, NullAlloc });
			TestCount04->Start({ &X, NullAlloc }); TestCount05->Start({ &X, NullAlloc }); TestCount06->Start({ &X, NullAlloc }); TestCount07->Start({ &X, NullAlloc });
			TestCount08->Start({ &X, NullAlloc }); TestCount09->Start({ &X, NullAlloc }); TestCount0A->Start({ &X, NullAlloc }); TestCount0B->Start({ &X, NullAlloc });
			TestCount0C->Start({ &X, NullAlloc }); TestCount0D->Start({ &X, NullAlloc }); TestCount0E->Start({ &X, NullAlloc }); TestCount0F->Start({ &X, NullAlloc });
			TestCount10->Start({ &X, NullAlloc }); TestCount11->Start({ &X, NullAlloc }); TestCount12->Start({ &X, NullAlloc }); TestCount13->Start({ &X, NullAlloc });
			TestCount14->Start({ &X, NullAlloc }); TestCount15->Start({ &X, NullAlloc }); TestCount16->Start({ &X, NullAlloc }); TestCount17->Start({ &X, NullAlloc });
			TestCount18->Start({ &X, NullAlloc }); TestCount19->Start({ &X, NullAlloc }); TestCount1A->Start({ &X, NullAlloc }); TestCount1B->Start({ &X, NullAlloc });
			TestCount1C->Start({ &X, NullAlloc }); TestCount1D->Start({ &X, NullAlloc }); TestCount1E->Start({ &X, NullAlloc }); TestCount1F->Start({ &X, NullAlloc });

			WaitMultiple({
				*TestCount00, *TestCount01, *TestCount02, *TestCount03,
				*TestCount04, *TestCount05, *TestCount06, *TestCount07,
				*TestCount08, *TestCount09, *TestCount0A, *TestCount0B,
				*TestCount0C, *TestCount0D, *TestCount0E, *TestCount0F,
				*TestCount10, *TestCount11, *TestCount12, *TestCount13,
				*TestCount14, *TestCount15, *TestCount16, *TestCount17,
				*TestCount18, *TestCount19, *TestCount1A, *TestCount1B,
				*TestCount1C, *TestCount1D, *TestCount1E, *TestCount1F
				}, true);

			_LOG(_T("--- Finished All Counting..."));
			_LOG(_T("X = %d"), X.Pickup()->value);
		}
		{
			TSyncInteger X(0);
			ExtAllocator NullAlloc;
			_LOG(_T("X = %d"), X.Pickup()->value);

			class TestCount : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncInt) override {
					int COUNT = 1000000;
					TSyncInteger* Ctr = static_cast<TSyncInteger*>(*pSyncInt);
					for (int i = 0; i < COUNT; i++) {
						(*Ctr->Pickup())++;
					}
					_LOG(_T("Count of %d done!"), COUNT);
					return { nullptr };
				}
			};

			MRWorkerThread TestCount00(CONSTRUCTION::EMPLACE, _T("CounterThread00"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount01(CONSTRUCTION::EMPLACE, _T("CounterThread01"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount02(CONSTRUCTION::EMPLACE, _T("CounterThread02"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount03(CONSTRUCTION::EMPLACE, _T("CounterThread03"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount04(CONSTRUCTION::EMPLACE, _T("CounterThread04"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount05(CONSTRUCTION::EMPLACE, _T("CounterThread05"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount06(CONSTRUCTION::EMPLACE, _T("CounterThread06"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount07(CONSTRUCTION::EMPLACE, _T("CounterThread07"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount08(CONSTRUCTION::EMPLACE, _T("CounterThread08"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount09(CONSTRUCTION::EMPLACE, _T("CounterThread09"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0A(CONSTRUCTION::EMPLACE, _T("CounterThread0A"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0B(CONSTRUCTION::EMPLACE, _T("CounterThread0B"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0C(CONSTRUCTION::EMPLACE, _T("CounterThread0C"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0D(CONSTRUCTION::EMPLACE, _T("CounterThread0D"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0E(CONSTRUCTION::EMPLACE, _T("CounterThread0E"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount0F(CONSTRUCTION::EMPLACE, _T("CounterThread0F"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount10(CONSTRUCTION::EMPLACE, _T("CounterThread10"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount11(CONSTRUCTION::EMPLACE, _T("CounterThread11"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount12(CONSTRUCTION::EMPLACE, _T("CounterThread12"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount13(CONSTRUCTION::EMPLACE, _T("CounterThread13"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount14(CONSTRUCTION::EMPLACE, _T("CounterThread14"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount15(CONSTRUCTION::EMPLACE, _T("CounterThread15"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount16(CONSTRUCTION::EMPLACE, _T("CounterThread16"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount17(CONSTRUCTION::EMPLACE, _T("CounterThread17"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount18(CONSTRUCTION::EMPLACE, _T("CounterThread18"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount19(CONSTRUCTION::EMPLACE, _T("CounterThread19"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1A(CONSTRUCTION::EMPLACE, _T("CounterThread1A"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1B(CONSTRUCTION::EMPLACE, _T("CounterThread1B"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1C(CONSTRUCTION::EMPLACE, _T("CounterThread1C"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1D(CONSTRUCTION::EMPLACE, _T("CounterThread1D"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1E(CONSTRUCTION::EMPLACE, _T("CounterThread1E"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1F(CONSTRUCTION::EMPLACE, _T("CounterThread1F"), DefaultObjAllocator<TestCount>().Create());

			_LOG(_T("--- Start Counting... (takes about 5 seconds)"));
			TestCount00->Start({ &X, NullAlloc }); TestCount01->Start({ &X, NullAlloc }); TestCount02->Start({ &X, NullAlloc }); TestCount03->Start({ &X, NullAlloc });
			TestCount04->Start({ &X, NullAlloc }); TestCount05->Start({ &X, NullAlloc }); TestCount06->Start({ &X, NullAlloc }); TestCount07->Start({ &X, NullAlloc });
			TestCount08->Start({ &X, NullAlloc }); TestCount09->Start({ &X, NullAlloc }); TestCount0A->Start({ &X, NullAlloc }); TestCount0B->Start({ &X, NullAlloc });
			TestCount0C->Start({ &X, NullAlloc }); TestCount0D->Start({ &X, NullAlloc }); TestCount0E->Start({ &X, NullAlloc }); TestCount0F->Start({ &X, NullAlloc });
			TestCount10->Start({ &X, NullAlloc }); TestCount11->Start({ &X, NullAlloc }); TestCount12->Start({ &X, NullAlloc }); TestCount13->Start({ &X, NullAlloc });
			TestCount14->Start({ &X, NullAlloc }); TestCount15->Start({ &X, NullAlloc }); TestCount16->Start({ &X, NullAlloc }); TestCount17->Start({ &X, NullAlloc });
			TestCount18->Start({ &X, NullAlloc }); TestCount19->Start({ &X, NullAlloc }); TestCount1A->Start({ &X, NullAlloc }); TestCount1B->Start({ &X, NullAlloc });
			TestCount1C->Start({ &X, NullAlloc }); TestCount1D->Start({ &X, NullAlloc }); TestCount1E->Start({ &X, NullAlloc }); TestCount1F->Start({ &X, NullAlloc });

			WaitMultiple({
				*TestCount00, *TestCount01, *TestCount02, *TestCount03,
				*TestCount04, *TestCount05, *TestCount06, *TestCount07,
				*TestCount08, *TestCount09, *TestCount0A, *TestCount0B,
				*TestCount0C, *TestCount0D, *TestCount0E, *TestCount0F,
				*TestCount10, *TestCount11, *TestCount12, *TestCount13,
				*TestCount14, *TestCount15, *TestCount16, *TestCount17,
				*TestCount18, *TestCount19, *TestCount1A, *TestCount1B,
				*TestCount1C, *TestCount1D, *TestCount1E, *TestCount1F
				}, true);

			_LOG(_T("--- Finished All Counting..."));
			_LOG(_T("X = %d"), X.Pickup()->value);
		}
	} else {
		class TestSyncLock : public TRunnable {
		protected:
			TEvent StopEvent;
			TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) override {
				TSyncInteger *X = (TSyncInteger*)&Arg;
				auto SX = X->Pickup();
				StopEvent.WaitFor();
				return { nullptr };
			}
			void StopNotify(TWorkerThread &WorkerThread) override {
				StopEvent.Set();
			}
		};

		_LOG(_T("*** Test SyncObj (Threading correctness)"));
		TSyncInteger A;

		_LOG(_T("--- Pickup (Failure with timeout & abort)"));
		{
			MRWorkerThread X(TWorkerThread::Create(_T("TestSyncLock"), DefaultObjAllocator<TestSyncLock>().Create()), CONSTRUCTION::HANDOFF);
			X->Start({ &A, DummyAllocator() });
			// Wait for the thread to claim the lock
			_LOG(_T("Waiting for locking thread to start..."));
			Sleep(500);

			{
				_LOG(_T("Picking up with 1 sec timeout (expect timeout)"));
				auto SA(A.Pickup(1000));
				// Should timeout
				_LOG(_T("Pickup : %s"), SA.toString().c_str());
			}

			// Set an alarm go off after 1 sec
			TAlarmClock FakeAbort(TimeSpan(1000, TimeUnit::MSEC));
			{
				// Try pickup with alarm as abort event
				_LOG(_T("Picking up with 1 sec alarm abort (expect abort)"));
				try {
					auto SA(A.Pickup(FOREVER, &FakeAbort));
					// Should aboort
					_LOG(_T("Pickup : %s"), SA.toString().c_str());
				} catch (Exception *e) {
					ManagedRef<Exception> E(e, CONSTRUCTION::HANDOFF);
					E->Show();
				}
			}

			_LOG(_T("Waiting for locking thread to terminate..."));
			X->SignalTerminate();
			X->WaitFor();
		}

		_LOG(_T("--- Parallel and concurrent accesses"));
		{
			TSyncInteger X(0);
			ExtAllocator NullAlloc;
			_LOG(_T("X = %d"), X.Pickup()->value);

			class TestCount : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncInt) override {
					int COUNT = 100000;
					TSyncInteger* Ctr = static_cast<TSyncInteger*>(*pSyncInt);
					for (int i = 0; i < COUNT; i++) {
						(*Ctr->Pickup())++;
					}
					_LOG(_T("Count of %d done!"), COUNT);
					return { nullptr };
				}
			};

			MRWorkerThread TestCount0(CONSTRUCTION::EMPLACE, _T("CounterThread0"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount1(CONSTRUCTION::EMPLACE, _T("CounterThread1"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount2(CONSTRUCTION::EMPLACE, _T("CounterThread2"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount3(CONSTRUCTION::EMPLACE, _T("CounterThread3"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount4(CONSTRUCTION::EMPLACE, _T("CounterThread4"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount5(CONSTRUCTION::EMPLACE, _T("CounterThread5"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount6(CONSTRUCTION::EMPLACE, _T("CounterThread6"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount7(CONSTRUCTION::EMPLACE, _T("CounterThread7"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount8(CONSTRUCTION::EMPLACE, _T("CounterThread8"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCount9(CONSTRUCTION::EMPLACE, _T("CounterThread9"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCountA(CONSTRUCTION::EMPLACE, _T("CounterThreadA"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCountB(CONSTRUCTION::EMPLACE, _T("CounterThreadB"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCountC(CONSTRUCTION::EMPLACE, _T("CounterThreadC"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCountD(CONSTRUCTION::EMPLACE, _T("CounterThreadD"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCountE(CONSTRUCTION::EMPLACE, _T("CounterThreadE"), DefaultObjAllocator<TestCount>().Create());
			MRWorkerThread TestCountF(CONSTRUCTION::EMPLACE, _T("CounterThreadF"), DefaultObjAllocator<TestCount>().Create());

			_LOG(_T("--- Start Counting..."));
			TestCount0->Start({ &X, NullAlloc }); TestCount1->Start({ &X, NullAlloc }); TestCount2->Start({ &X, NullAlloc }); TestCount3->Start({ &X, NullAlloc });
			TestCount4->Start({ &X, NullAlloc }); TestCount5->Start({ &X, NullAlloc }); TestCount6->Start({ &X, NullAlloc }); TestCount7->Start({ &X, NullAlloc });
			TestCount8->Start({ &X, NullAlloc }); TestCount9->Start({ &X, NullAlloc });	TestCountA->Start({ &X, NullAlloc });	TestCountB->Start({ &X, NullAlloc });
			TestCountC->Start({ &X, NullAlloc }); TestCountD->Start({ &X, NullAlloc }); TestCountE->Start({ &X, NullAlloc }); TestCountF->Start({ &X, NullAlloc });

			WaitMultiple({ *TestCount0, *TestCount1, *TestCount2, *TestCount3,
				*TestCount4, *TestCount5, *TestCount6, *TestCount7,
				*TestCount8, *TestCount9, *TestCountA, *TestCountB,
				*TestCountC, *TestCountD, *TestCountE, *TestCountF }, true);

			_LOG(_T("--- Finished All Counting..."));
			_LOG(_T("X = %d"), X.Pickup()->value);
		}
	}
}

#include "Threading/SyncContainers.h"

#include <mutex>
#include <condition_variable>
#include <deque>

template <typename T>
class TQueue {
private:
	std::mutex              d_mutex;
	std::condition_variable d_condition;
	std::deque<T>           d_queue;
public:
	void push(T const& value) {
		{
			std::unique_lock<std::mutex> lock(this->d_mutex);
			d_queue.push_front(value);
		}
		d_condition.notify_one();
	}
	T pop() {
		std::unique_lock<std::mutex> lock(this->d_mutex);
		d_condition.wait(lock, [=] { return !this->d_queue.empty(); });
		T rc(std::move(this->d_queue.back()));
		d_queue.pop_back();
		return rc;
	}
};

void TestSyncQueue(bool Profiling) {
	if (Profiling) {
		_LOG(_T("*** Test SyncQueue (Performance profiling)"));
		{
			typedef TSyncBlockingDeque<int> TSyncIntQueue;
			class TestQueuePut : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
					int COUNT = 10000000;
					TimeStamp StartTime = TimeStamp::Now();
					for (int i = 0; i < COUNT; i++) Q.Push_Back(i);
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Enqueue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return { nullptr };
				}
			};

			class TestQueueGet : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
					int COUNT = 10000000;
					TimeStamp StartTime = TimeStamp::Now();
					int j = -1;
					for (int i = 0; i < COUNT; i++) {
						Q.Pop_Front(j);
						if (i != j) FAIL(_T("Expect %d, got %d"), i, j);
					}
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Dequeue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return nullptr;
				}
			};

			ExtAllocator NullAlloc;
			TSyncIntQueue Queue(_T("SyncIntQueue"));
			MRWorkerThread PutThread(CONSTRUCTION::EMPLACE, _T("QueuePutThread"), DefaultObjAllocator<TestQueuePut>().Create());
			MRWorkerThread GetThread(CONSTRUCTION::EMPLACE, _T("QueueGetThread"), DefaultObjAllocator<TestQueueGet>().Create());
			_LOG(_T("--- Starting Parallel Producer & Consumer..."));
			PutThread->Start({ &Queue, NullAlloc });
			GetThread->Start({ &Queue, NullAlloc });
			GetThread->WaitFor();
			_LOG(_T("--- Finished All Queue Operation..."));
		}
		{
			typedef TSyncBlockingDeque<int> TSyncIntQueue;
			class TestQueuePut : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
					int COUNT = 10000000;
					TimeStamp StartTime = TimeStamp::Now();
					for (int i = 0; i < COUNT; i++) Q.Push_Back(i);
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Enqueue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return { nullptr };
				}
			};

			class TestQueueGet : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
					int COUNT = 10000000;
					TimeStamp StartTime = TimeStamp::Now();
					int j = -1;
					for (int i = 0; i < COUNT; i++) {
						Q.Pop_Front(j);
						if (i != j) FAIL(_T("Expect %d, got %d"), i, j);
					}
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Dequeue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return nullptr;
				}
			};

			ExtAllocator NullAlloc;
			TSyncIntQueue Queue(_T("SyncIntQueue"));
			MRWorkerThread PutThread(CONSTRUCTION::EMPLACE, _T("QueuePutThread"), DefaultObjAllocator<TestQueuePut>().Create());
			MRWorkerThread GetThread(CONSTRUCTION::EMPLACE, _T("QueueGetThread"), DefaultObjAllocator<TestQueueGet>().Create());
			_LOG(_T("--- Starting Parallel Producer & Consumer..."));
			PutThread->Start({ &Queue, NullAlloc });
			GetThread->Start({ &Queue, NullAlloc });
			GetThread->WaitFor();
			_LOG(_T("--- Finished All Queue Operation..."));
		}
		{
			typedef TSyncBlockingDeque<int> TSyncIntQueue;
			class TestQueuePut : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
					int COUNT = 10000000;
					TimeStamp StartTime = TimeStamp::Now();
					for (int i = 0; i < COUNT; i++) Q.Push_Back(i);
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Enqueue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return { nullptr };
				}
			};

			class TestQueueGet : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
					int COUNT = 10000000;
					TimeStamp StartTime = TimeStamp::Now();
					int j = -1;
					for (int i = 0; i < COUNT; i++) {
						Q.Pop_Front(j);
						if (i != j) FAIL(_T("Expect %d, got %d"), i, j);
					}
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Dequeue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return nullptr;
				}
			};

			ExtAllocator NullAlloc;
			TSyncIntQueue Queue(_T("SyncIntQueue"));
			MRWorkerThread PutThread(CONSTRUCTION::EMPLACE, _T("QueuePutThread"), DefaultObjAllocator<TestQueuePut>().Create());
			MRWorkerThread GetThread(CONSTRUCTION::EMPLACE, _T("QueueGetThread"), DefaultObjAllocator<TestQueueGet>().Create());
			_LOG(_T("--- Starting Parallel Producer & Consumer..."));
			PutThread->Start({ &Queue, NullAlloc });
			GetThread->Start({ &Queue, NullAlloc });
			GetThread->WaitFor();
			_LOG(_T("--- Finished All Queue Operation..."));
		}
		{
			typedef TSyncBlockingDeque<int> TSyncIntQueue;
			class TestQueuePut : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
					int COUNT = 10000000;
					TimeStamp StartTime = TimeStamp::Now();
					for (int i = 0; i < COUNT; i++) Q.Push_Back(i);
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Enqueue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return { nullptr };
				}
			};

			class TestQueueGet : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
					int COUNT = 10000000;
					TimeStamp StartTime = TimeStamp::Now();
					int j = -1;
					for (int i = 0; i < COUNT; i++) {
						Q.Pop_Front(j);
						if (i != j) FAIL(_T("Expect %d, got %d"), i, j);
					}
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Dequeue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return nullptr;
				}
			};

			ExtAllocator NullAlloc;
			TSyncIntQueue Queue(_T("SyncIntQueue"));
			MRWorkerThread PutThread(CONSTRUCTION::EMPLACE, _T("QueuePutThread"), DefaultObjAllocator<TestQueuePut>().Create());
			MRWorkerThread GetThread(CONSTRUCTION::EMPLACE, _T("QueueGetThread"), DefaultObjAllocator<TestQueueGet>().Create());
			_LOG(_T("--- Starting Parallel Producer & Consumer..."));
			PutThread->Start({ &Queue, NullAlloc });
			GetThread->Start({ &Queue, NullAlloc });
			GetThread->WaitFor();
			_LOG(_T("--- Finished All Queue Operation..."));
		}
		{
			typedef TSyncBlockingDeque<int> TSyncIntQueue;
			class TestQueuePut : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
					int COUNT = 10000000;
					TimeStamp StartTime = TimeStamp::Now();
					for (int i = 0; i < COUNT; i++) Q.Push_Back(i);
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Enqueue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return { nullptr };
				}
			};

			class TestQueueGet : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
					int COUNT = 10000000;
					TimeStamp StartTime = TimeStamp::Now();
					int j = -1;
					for (int i = 0; i < COUNT; i++) {
						Q.Pop_Front(j);
						if (i != j) FAIL(_T("Expect %d, got %d"), i, j);
					}
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Dequeue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return nullptr;
				}
			};

			ExtAllocator NullAlloc;
			TSyncIntQueue Queue(_T("SyncIntQueue"));
			MRWorkerThread PutThread(CONSTRUCTION::EMPLACE, _T("QueuePutThread"), DefaultObjAllocator<TestQueuePut>().Create());
			MRWorkerThread GetThread(CONSTRUCTION::EMPLACE, _T("QueueGetThread"), DefaultObjAllocator<TestQueueGet>().Create());
			_LOG(_T("--- Starting Parallel Producer & Consumer..."));
			PutThread->Start({ &Queue, NullAlloc });
			GetThread->Start({ &Queue, NullAlloc });
			GetThread->WaitFor();
			_LOG(_T("--- Finished All Queue Operation..."));
		}
	} else {
		_LOG(_T("*** Test SyncQueue (Non-threading correctness)"));
		{
			TSyncBlockingDeque<int> TestQueue(_T("TestQueue1"));
			_LOG(_T("Push 123 : %d"), TestQueue.Push_Back(123));
			int A = 0;
			_LOG(_T("A = %d"), A);
			_LOG(_T("EmptyLock (Wait 0.5 seconds and expect failure)"));
			if (TestQueue.DrainAndLock(500)) {
				FAIL(_T("Should not reach"));
			} else {
				_LOG(_T("Failed to lock empty queue (expected)"));
			}

			_LOG(_T("Pop -> A : %d"), TestQueue.Pop_Front(A));
			_LOG(_T("A = %d"), A);
			_LOG(_T("Pop -> A (Wait 0.5 seconds and expect failure)"));
			if (TestQueue.Pop_Front(A, 500)) {
				FAIL(_T("Should not reach"));
			} else { _LOG(_T("Failed to dequeue (expected)")); }
			_LOG(_T("EmptyLock (Expect immediate success)"));
			if (TestQueue.DrainAndLock(0)) {
				_LOG(_T("Locked empty queue!"));
			} else {
				FAIL(_T("Failed to lock empty queue (unexpected)"));
			}
		}

		_LOG(_T("*** Test SyncQueue (Threading correctness)"));
		if (IsDebuggerPresent()) {
			_LOG(_T("!!! Because you have debugger attached, the performance is NOT accurate"));
#ifdef _DEBUG
			_LOG(_T("!!! Because you have compiled in debug mode, the performance is NOT accurate"));
#endif
		}
		{
			typedef TQueue<int> TSyncIntQueue;
			class TestQueuePut : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
#ifdef _DEBUG
					int COUNT = 5000000;
#else
					int COUNT = 50000000;
#endif
					TimeStamp StartTime = TimeStamp::Now();
					for (int i = 0; i < COUNT; i++) Q.push(i);
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Enqueue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return { nullptr };
				}
			};

			class TestQueueGet : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
#ifdef _DEBUG
					int COUNT = 5000000;
#else
					int COUNT = 50000000;
#endif
					TimeStamp StartTime = TimeStamp::Now();
					int j;
					for (int i = 0; i < COUNT; i++) {
						j = Q.pop();
						if (i != j) FAIL(_T("Expect %d, got %d"), i, j);
					}
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Dequeue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return nullptr;
				}
			};

			ExtAllocator NullAlloc;
			TSyncIntQueue Queue;
			MRWorkerThread PutThread(CONSTRUCTION::EMPLACE, _T("QueuePutThread"), DefaultObjAllocator<TestQueuePut>().Create());
			MRWorkerThread GetThread(CONSTRUCTION::EMPLACE, _T("QueueGetThread"), DefaultObjAllocator<TestQueueGet>().Create());
			_LOG(_T("--- Benchmarking Comparison Queue (Upper-bound)..."));
			PutThread->Start({ &Queue, NullAlloc });
			GetThread->Start({ &Queue, NullAlloc });
			GetThread->WaitFor();
			_LOG(_T("--- Finished All Queue Operation..."));

		}

		{
			typedef TSyncBlockingDeque<int> TSyncIntQueue;
			class TestQueuePut : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
#ifdef _DEBUG
					int COUNT = 5000000;
#else
					int COUNT = 50000000;
#endif
					TimeStamp StartTime = TimeStamp::Now();
					for (int i = 0; i < COUNT; i++) Q.Push_Back(i);
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Enqueue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return { nullptr };
				}
			};

			class TestQueueGet : public TRunnable {
			protected:
				TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
					TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);
#ifdef _DEBUG
					int COUNT = 5000000;
#else
					int COUNT = 50000000;
#endif
					TimeStamp StartTime = TimeStamp::Now();
					int j = -1;
					for (int i = 0; i < COUNT; i++) {
						Q.Pop_Front(j);
						if (i != j) FAIL(_T("Expect %d, got %d"), i, j);
					}
					TimeStamp EndTime = TimeStamp::Now();

					double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
					_LOG(_T("Dequeue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
					return nullptr;
				}
			};

			ExtAllocator NullAlloc;
			TSyncIntQueue Queue(_T("SyncIntQueue"));
			MRWorkerThread PutThread(CONSTRUCTION::EMPLACE, _T("QueuePutThread"), DefaultObjAllocator<TestQueuePut>().Create());
			MRWorkerThread GetThread(CONSTRUCTION::EMPLACE, _T("QueueGetThread"), DefaultObjAllocator<TestQueueGet>().Create());
			_LOG(_T("--- Starting Parallel Producer & Consumer..."));
			PutThread->Start({ &Queue, NullAlloc });
			GetThread->Start({ &Queue, NullAlloc });
			GetThread->WaitFor();
			_LOG(_T("--- Finished All Queue Operation..."));
			Queue.Deflate();

			MRWorkerThread PutThread2(CONSTRUCTION::EMPLACE, _T("QueuePutThread"), DefaultObjAllocator<TestQueuePut>().Create());
			MRWorkerThread GetThread2(CONSTRUCTION::EMPLACE, _T("QueueGetThread"), DefaultObjAllocator<TestQueueGet>().Create());
			_LOG(_T("--- Starting Parallel Producer & Consumer..."));
			PutThread2->Start({ &Queue, NullAlloc });
			GetThread2->Start({ &Queue, NullAlloc });
			{
				_LOG(_T("Sleep for 2 sec..."));
				TDelayWaitable WaitASec(2000);
				WaitASec.WaitFor(FOREVER);
			}
#ifdef __SDQ_ITERATORS
			{
				_LOG(_T("Trying to grab push-pop lock..."));
				TSyncIntQueue::MRLock PPLock(CONSTRUCTION::EMPLACE, Queue.Lock());
				_LOG(_T("Current queue length: %d"), Queue.Length());
				_LOG(_T("Iterating through the queie..."));
				int _CNT = 0;
				int _MIN = 50000000;
				int _MAX = 0;
				auto Iter = Queue.cbegin(PPLock);
				auto IterEnd = Queue.cend(PPLock);
				for (; Iter != IterEnd; Iter++) {
					_CNT++;
					_MIN = std::min(*Iter, _MIN);
					_MAX = std::max(*Iter, _MAX);
				}
				_LOG(_T("Went through %d items, min %d, max %d (diff %d)"), _CNT, _MIN, _MAX, _MAX - _MIN);
				_LOG(_T("Current queue length: %d"), Queue.Length());
				_LOG(_T("Releasing push-pop lock..."));
			}
#endif
			{
				_LOG(_T("Trying to grab empty lock..."));
				auto ELock = Queue.DrainAndLock();
				_LOG(_T("Current queue length: %d"), Queue.Length());
				_LOG(_T("Sleeping for 0.1 sec..."));
				TDelayWaitable WaitASec(100);
				WaitASec.WaitFor(FOREVER);
				Queue.Deflate();
				_LOG(_T("Releasing empty lock..."));
			}
			{
				_LOG(_T("Sleep for 2 sec..."));
				TDelayWaitable WaitASec(2000);
				WaitASec.WaitFor(FOREVER);
			}
#ifdef __SDQ_ITERATORS
			{
				_LOG(_T("Trying to grab push-pop lock..."));
				TSyncIntQueue::MRLock PPLock(CONSTRUCTION::EMPLACE, Queue.Lock());
				_LOG(_T("Current queue length: %d"), Queue.Length());
				_LOG(_T("Iterating through the queie..."));
				int _CNT = 0;
				int _MIN = 50000000;
				int _MAX = 0;
				auto Iter = Queue.cbegin(PPLock);
				auto IterEnd = Queue.cend(PPLock);
				for (; Iter != IterEnd; Iter++) {
					_CNT++;
					_MIN = std::min(*Iter, _MIN);
					_MAX = std::max(*Iter, _MAX);
				}
				_LOG(_T("Went through %d items, min %d, max %d (diff %d)"), _CNT, _MIN, _MAX, _MAX - _MIN);
				_LOG(_T("Current queue length: %d"), Queue.Length());
				_LOG(_T("Releasing push-pop lock..."));
			}
#endif
			{
				_LOG(_T("Trying to grab empty lock..."));
				auto ELock = Queue.DrainAndLock();
				_LOG(_T("Current queue length: %d"), Queue.Length());
				_LOG(_T("Sleeping for 0.1 sec..."));
				TDelayWaitable WaitASec(100);
				WaitASec.WaitFor(FOREVER);
				Queue.Deflate();
				_LOG(_T("Releasing empty lock..."));
			}
#ifdef __SDQ_MUTABLE_ITERATORS
			{
				_LOG(_T("Sleep for 2 sec..."));
				TDelayWaitable WaitASec(2000);
				WaitASec.WaitFor(FOREVER);
			}
			{
				_LOG(_T("Trying to grab push-pop lock..."));
				TSyncIntQueue::MRLock PPLock(CONSTRUCTION::EMPLACE, Queue.Lock());
				_LOG(_T("Current queue length: %d"), Queue.Length());
				{
					_LOG(_T("Trying to get mutable iterators..."));
					auto Iter = Queue.begin(PPLock);
					auto IterEnd = Queue.end(PPLock);
					if (Iter != IterEnd) {
						_LOG(_T("Remove item #%d from the queue (will trigger getter exception later)..."), *Iter);
						Iter = Queue.erase(Iter);
					} else {
						_LOG(_T("Unable to fully test mutable iterators, try another run may help"));
					}
					if (Iter != IterEnd) {
						_LOG(_T("Remove another item #%d from the queue (testing iterator cascade operations)..."), *Iter);
						Iter = Queue.erase(Iter);
					} else {
						_LOG(_T("Unable to fully test mutable iterators, try another run may help"));
					}
					_LOG(_T("Next item is #%d"), *Iter);
					_LOG(_T("Inserting item #%d at current location..."), 1);
					Iter = Queue.insert(Iter, 1);
					_LOG(_T("Next item is #%d"), *Iter);
					_LOG(_T("Current queue length: %d"), Queue.Length());
					{
						_LOG(_T("---- #1 Starting parallel iteration..."));
						class TestQueueIter : public TRunnable {
						protected:
							TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
								TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);

								_LOG(_T("Trying to grab push-pop lock..."));
								TSyncIntQueue::MRLock PPLock(CONSTRUCTION::EMPLACE, Q.Lock());
								_LOG(_T("Current queue length: %d"), Q.Length());
								_LOG(_T("Trying to get an const iterator (expect timeout in 0.5 sec)..."));
								auto Iter = Q.cbegin(PPLock, 500);

								if (!Iter.Valid()) {
									_LOG(_T("Unable to acquire iterator (expected)."));
								} else {
									FAIL(_T("Acquired iterator (concurrency violation)"));
								}
								return { nullptr };
							}
						};

						MRWorkerThread IterThread(CONSTRUCTION::EMPLACE, _T("QueueIterThread"), DefaultObjAllocator<TestQueueIter>().Create());
						IterThread->Start({ &Queue, NullAlloc });
						IterThread->WaitFor();
						auto IterExcept = IterThread->FatalException(true);
						if (IterExcept) throw IterExcept;
						_LOG(_T("---- #1 Parallel iteration finished..."));
					}
					_LOG(_T("Releasing iterator locks..."));
				}
				{
					_LOG(_T("---- #2 Starting serialized iteration..."));
					class TestQueueIter : public TRunnable {
					protected:
						TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
							TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);

							_LOG(_T("Trying to grab push-pop lock..."));
							TSyncIntQueue::MRLock PPLock(CONSTRUCTION::EMPLACE, Q.Lock());
							_LOG(_T("Current queue length: %d"), Q.Length());
							_LOG(_T("Trying to get an const iterator (expect immediate success)..."));
							auto Iter = Q.cbegin(PPLock, 500);

							if (Iter.Valid()) {
								_LOG(_T("Acquired iterator (expected)."));
							} else {
								FAIL(_T("Unable to acquire iterator"));
							}

							_LOG(_T("Hold iterator for 3 second..."));
							Sleep(3000);
							_LOG(_T("Releasing iterator..."));
							return { nullptr };
						}
					};

					MRWorkerThread IterThread(CONSTRUCTION::EMPLACE, _T("QueueIterThread"), DefaultObjAllocator<TestQueueIter>().Create());
					IterThread->Start({ &Queue, NullAlloc });
					Sleep(1000);

#ifdef __SDQ_CONCURRENT_CONST_ITERATORS
					{
						_LOG(_T("---- #2.1 Starting parallel const iteration..."));
						_LOG(_T("Trying to grab push-pop lock..."));
						TSyncIntQueue::MRLock PPLock(CONSTRUCTION::EMPLACE, Queue.Lock());
						_LOG(_T("Current queue length: %d"), Queue.Length());
						_LOG(_T("Trying to get an const iterator (expect immediate success)..."));
						auto Iter = Queue.cbegin(PPLock, 500);

						if (Iter.Valid()) {
							_LOG(_T("Acquired iterator (expected)."));
						} else {
							FAIL(_T("Unable to acquire iterator"));
						}
						_LOG(_T("---- #2.1 Parallel const iteration finished..."));
					}

					Sleep(500);
					{
						_LOG(_T("---- #2.2 Starting parallel mutable iterations..."));
						_LOG(_T("Trying to grab push-pop lock..."));
						TSyncIntQueue::MRLock PPLock(CONSTRUCTION::EMPLACE, Queue.Lock());
						_LOG(_T("Current queue length: %d"), Queue.Length());
						_LOG(_T("Trying to get a mutable iterator (expect timeout in 0.5 sec)..."));
						auto Iter = Queue.begin(PPLock, 500);

						if (!Iter.Valid()) {
							_LOG(_T("Unable to acquire iterator (expected)."));
						} else {
							FAIL(_T("Acquired iterator (concurrency violation)"));
						}

						_LOG(_T("Trying to get a mutable iterator (expect block for ~1 sec and succeed)..."));
						Iter = Queue.begin(PPLock, 3000);

						if (Iter.Valid()) {
							_LOG(_T("Acquired iterator (expected)."));
						} else {
							FAIL(_T("Unable to acquire iterator"));
						}
						_LOG(_T("---- #2.2 Parallel mutable iteration finished..."));
					}
#endif

					IterThread->WaitFor();
					auto IterExcept = IterThread->FatalException(true);
					if (IterExcept) throw IterExcept;
					_LOG(_T("---- #2 Serialized iteration finished..."));
				}
				_LOG(_T("Releasing push-pop lock (expect getting to crash soon)..."));
			}
			GetThread2->WaitFor();
			auto GetExcept = GetThread2->FatalException();
			if (GetExcept) {
				_LOG(_T("Getter crashed (expected):"));
				GetExcept->Show();
			} else FAIL(_T("Getter did not observe queue modification!"));
#else
			GetThread2->WaitFor();
#endif
			_LOG(_T("--- Finished All Queue Operation..."));
		}
	}
}
