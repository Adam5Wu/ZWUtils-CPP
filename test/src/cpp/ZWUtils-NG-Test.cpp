/*
Copyright (c) 2005 - 2016, Zhenyu Wu; 2012 - 2016, NEC Labs America Inc.
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
void TestSyncPrems();
void TestDynBuffer();
void TestManagedObj();
void TestSyncObj();
void TestSize();
void TestTiming();
void TestWorkerThread();
void TestSyncQueue();

#ifdef WINDOWS
int _tmain(int argc, _TCHAR* argv[])
#endif
{
	ControlSEHTranslation(true);

	_LOG(_T("%s"), __REL_FILE__);
	try {
		if (argc != 2)
			FAIL(_T("Require 1 parameter: <TestType> = 'ALL' | ")
			_T("'Exception' / 'ErrCode' / 'StringConv' / 'SyncPrems' / 'DynBuffer' / ")
			_T("'ManagedObj' / 'SyncObj' / 'Size' / 'Timing' / 'WorkerThread' / 'SyncQueue'"));

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
		if (TestAll || (_tcsicmp(argv[1], _T("SyncPrems")) == 0)) {
			TestSyncPrems();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("DynBuffer")) == 0)) {
			TestDynBuffer();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("ManagedObj")) == 0)) {
			TestManagedObj();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("SyncObj")) == 0)) {
			TestSyncObj();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("Size")) == 0)) {
			TestSize();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("Timing")) == 0)) {
			TestTiming();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("WorkerThread")) == 0)) {
			TestWorkerThread();
		}
		if (TestAll || (_tcsicmp(argv[1], _T("SyncQueue")) == 0)) {
			TestSyncQueue();
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
	_LOG(_T("Converting: 'This is a test...'"));
	WString Test1{L"This is a test..."};
	CString Test1C = WStringtoUTF8(Test1);
	WString Test1R = UTF8toWString(Test1C);
	if (Test1.compare(Test1R) != 0)
		FAIL(_T("String failed to round-trip!"));

	setlocale(LC_ALL, "chs");
	_LOG(_T("Converting: '这是一个测试...'"));
	WString Test2{L"这是一个测试..."};
	CString Test2C = WStringtoUTF8(Test2);
	TString Test2R = UTF8toWString(Test2C);
	if (Test2.compare(Test2R) != 0)
		FAIL(_T("String failed to round-trip!"));
}

#include "Threading/SyncElements.h"

void TestSyncPrems() {
	_LOG(_T("*** Test Synchronization Premises"));
	TInterlockedArchInt A{0};
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

		TVoidDynBuffer.SetSize(10);
		_LOG(_T("Increase to 10B: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));
		memset(&TVoidDynBuffer, 'A', 9);
		((char*)&TVoidDynBuffer)[9] = '\0';
		_LOG(_T("Filled with 'A' = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		TVoidDynBuffer.SetSize(2048);
		_LOG(_T("Extend to 2KB: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));
		memset(((char*)&TVoidDynBuffer) + 9, 'B', 9);
		((char*)&TVoidDynBuffer)[18] = '\0';
		_LOG(_T("Append with 9x'B' = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));

		TVoidDynBuffer.SetSize(10);
		_LOG(_T("Shrink to 10B: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));
		((char*)&TVoidDynBuffer)[9] = '\0';
		_LOG(_T("Terminate at byte 10 = %p (%s)"), &TVoidDynBuffer, TStringCast((char*)&TVoidDynBuffer).c_str());
		_LOG(_T("Buffer status: %s"), TVoidDynBuffer.Allocated() ? _T("Allocated") : _T("Unallocated"));
	}

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
		virtual TString toString(void) const { return{_T("TestPObj")}; }
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
		virtual TString toString(void) const { return{_T("TestCObj")}; }
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
bool operator==(Integer const &A, Integer const &B)
{ return A.value == B.value; }

typedef TSyncObj<Integer> TSyncInteger;

void TestSyncObj(void) {
	_LOG(_T("*** Test SyncObj (Non-threading correctness)"));
	TSyncInteger A;

	_LOG(_T("--- Pickup (Successful)"));
	{
		auto SA(A.Pickup());
		_LOG(_T("Pickup : %s"), SA.toString().c_str());
		_LOG(_T("A : %d"), SA->value);
	}

	_LOG(_T("--- Pickup (Failure)"));
	{
		TDelayWaitable FakeAbort(0);
		auto SA(A.Pickup(std::addressof(FakeAbort)));
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
	Integer D{55};
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

void TestSize(void) {
	_LOG(_T("*** Test Size"));
	_LOG(_T("1 MB: %s"), ToString(1, SizeUnit::MB).c_str());
	_LOG(_T(" - byte unit: %s"), ToString(1, SizeUnit::MB, SizeUnit::BYTE).c_str());
	_LOG(_T(" - KB unit: %s"), ToString(1, SizeUnit::MB, SizeUnit::KB).c_str());

	long long S1G1K = Convert(1, SizeUnit::GB, SizeUnit::BYTE) + Convert(1, SizeUnit::KB, SizeUnit::BYTE);
	_LOG(_T("1 GB + 1KB: %s"), ToString(S1G1K, SizeUnit::BYTE).c_str());
	_LOG(_T(" - MB unit: %s"), ToString(S1G1K, SizeUnit::BYTE, SizeUnit::MB, SizeUnit::MB).c_str());
	_LOG(_T(" - GB-MB unit: %s"), ToString(S1G1K, SizeUnit::BYTE, SizeUnit::GB, SizeUnit::MB).c_str());

	long long S1G1KN1B = S1G1K - 1;
	_LOG(_T("1 GB + 1KB - 1B: %s"), ToString(S1G1KN1B, SizeUnit::BYTE).c_str());
	_LOG(_T(" - MB unit: %s"), ToString(S1G1KN1B, SizeUnit::BYTE, SizeUnit::MB, SizeUnit::MB).c_str());
	_LOG(_T(" - GB-MB unit: %s"), ToString(S1G1KN1B, SizeUnit::BYTE, SizeUnit::GB, SizeUnit::MB).c_str());
	_LOG(_T(" - GB-MB unit, abbreviate: %s"), ToString(S1G1KN1B, SizeUnit::BYTE, SizeUnit::GB, SizeUnit::MB, true).c_str());
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
	_LOG(_T(" = millisecond resolution, abbreviate: %s"), TS3S.toString(TimeUnit::MSEC, true).c_str());
	_LOG(_T(" = minute resolution: %s"), TS3S.toString(TimeUnit::MIN).c_str());
	_LOG(_T(" = minute-millisecond resolution: %s"), TS3S.toString(TimeUnit::MIN, TimeUnit::MSEC).c_str());

	TimeSpan TS3M = TimeSpan(3, TimeUnit::MIN);
	_LOG(_T("3 minute timespan: %s"), TS3M.toString().c_str());
	_LOG(_T(" = millisecond resolution: %s"), TS3M.toString(TimeUnit::MSEC, false).c_str());
	_LOG(_T(" = second-millisecond resolution: %s"), TS3M.toString(TimeUnit::MIN, TimeUnit::MSEC).c_str());

	_LOG(_T("--- Time arithmetics"));
	TimeSpan TS3D = TimeSpan(3, TimeUnit::DAY);
	auto TS3D3M = TS3D + TS3M;
	_LOG(_T("3 days + 3 minute timespan: %s"), TS3D3M.toString().c_str());
	_LOG(_T(" = hour-millisecond resolution: %s"), TS3D3M.toString(TimeUnit::HR, TimeUnit::MSEC).c_str());
	_LOG(_T(" = day-millisecond resolution: %s"), TS3D3M.toString(TimeUnit::DAY, TimeUnit::MSEC).c_str());
	_LOG(_T(" = day-hour resolution: %s"), TS3D3M.toString(TimeUnit::DAY, TimeUnit::HR).c_str());
	_LOG(_T(" = day-hour resolution, abbreviate: %s"), TS3D3M.toString(TimeUnit::DAY, TimeUnit::HR, true).c_str());

	auto TS3D3MN3S = TS3D3M - TS3S;
	_LOG(_T("3 days + 3 minute - 3 seconds second timespan: %s"), TS3D3MN3S.toString().c_str());
	_LOG(_T(" = hour-millisecond resolution: %s"), TS3D3MN3S.toString(TimeUnit::HR, TimeUnit::MSEC).c_str());
	_LOG(_T(" = day-millisecond resolution: %s"), TS3D3MN3S.toString(TimeUnit::DAY, TimeUnit::MSEC).c_str());
	_LOG(_T(" = day-hour resolution: %s"), TS3D3MN3S.toString(TimeUnit::DAY, TimeUnit::HR).c_str());
	_LOG(_T(" = day-hour resolution, abbreviate: %s"), TS3D3MN3S.toString(TimeUnit::DAY, TimeUnit::HR, true).c_str());

	_LOG(_T("Current time + 3 seconds: %s"), Now.Offset(TS3S).toString().c_str());
	_LOG(_T("Current time - 3 minute: %s"), Now.Offset(-TS3M).toString().c_str());
	_LOG(_T("Current time + 3 days: %s"), Now.Offset(TS3D).toString().c_str());
	_LOG(_T("Current time + 3 days + 3 minute - 3 seconds: %s"), Now.Offset(TS3D3MN3S).toString().c_str());
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
			return TFixedBuffer(nullptr);
		}
	};

	_LOG(_T("*** Test WorkerThread (Normal)"));
	{
		ManagedRef<TWorkerThread> A(TWorkerThread::Create(_T("TestA"), *DefaultObjAllocator<TestRunnable>().Create()), CONSTRUCTION::HANDOFF);
		A->Start();
		A->WaitFor();
		_LOG(_T("Return data: %s"), TStringCast(A->ReturnData()).c_str());
		if (A->FatalException() != nullptr) {
			_LOG(_T("WARNING: Thread crashed - %s"), A->FatalException()->Why().c_str());
		}
	}

	_LOG(_T("*** Test WorkerThread (Exception during run)"));
	{
		ManagedRef<TWorkerThread> B(TWorkerThread::Create(_T("TestB"), *DefaultObjAllocator<TestRunnable>().Create()), CONSTRUCTION::HANDOFF);
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
		TWorkerThread::Create(_T("TestC"), *DefaultObjAllocator<TestRunnable>().Create(), true)->Start();
		TDelayWaitable WaitASec(500);
		WaitASec.WaitFor(FOREVER);
		_LOG(_T("Expect the worker thread has terminated and destroyed by now..."));
	}

	_LOG(_T("*** Test WorkerThread (Exception, Self-free)"));
	{
		TWorkerThread::Create(_T("TestD"), *DefaultObjAllocator<TestRunnable>().Create(), true)->Start(DefaultAllocator().Alloc(100));
		TDelayWaitable WaitASec(500);
		WaitASec.WaitFor(FOREVER);
		_LOG(_T("Expect the worker thread has terminated and destroyed by now..."));
	}

	class TestSelfDestroyRunnable : public TRunnable {
	protected:
		TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &Arg) override {
			_LOG(_T("Yee Hah!"));
			DefaultObjAllocator<TWorkerThread>().Destroy(std::addressof(WorkerThread));
			return TFixedBuffer(nullptr);
		}
	};

	_LOG(_T("*** Test WorkerThread (Self-destroy)"));
	{
		ManagedRef<TWorkerThread> E(TWorkerThread::Create(_T("TestE"), *DefaultObjAllocator<TestSelfDestroyRunnable>().Create()), CONSTRUCTION::HANDOFF);
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
		ManagedRef<TWorkerThread> F(TWorkerThread::Create(_T("TestF"), *DefaultObjAllocator<TestRunnable>().Create()), CONSTRUCTION::HANDOFF);
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
			auto ConstructEvent = TWorkerThread::StateNotify(_T("TestConstructed"), TWorkerThread::State::Constructed,
															 [](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
				_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
			});
			auto InitializingEvent = TWorkerThread::StateNotify(_T("TestInitializing"), TWorkerThread::State::Initialzing,
																[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
				_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
			});
			auto RunningEvent = TWorkerThread::StateNotify(_T("TestRunning"), TWorkerThread::State::Running,
														   [](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
				_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
			});
			auto TerminatingEvent = TWorkerThread::StateNotify(_T("TestTerminating"), TWorkerThread::State::Terminating,
															   [](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
				_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
			});
			auto TerminatedEvent = TWorkerThread::StateNotify(_T("TestTerminated"), TWorkerThread::State::Terminated,
															  [](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
				_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
			});
			ManagedRef<TWorkerThread> G(TWorkerThread::Create(_T("TestG"), *DefaultObjAllocator<TestRunnable>().Create()), CONSTRUCTION::HANDOFF);
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
			auto InitializingEvent = TWorkerThread::StateNotify(_T("TestInitializing"), TWorkerThread::State::Initialzing,
																[](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
				_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
			});
			auto RunningEvent = TWorkerThread::StateNotify(_T("TestRunning"), TWorkerThread::State::Running,
														   [](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
				_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
			});
			auto TerminatingEvent = TWorkerThread::StateNotify(_T("TestTerminating"), TWorkerThread::State::Terminating,
															   [](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
				_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
			});
			ManagedRef<TWorkerThread> H(TWorkerThread::Create(_T("TestH"), *DefaultObjAllocator<TestDelayRunnable>().Create()), CONSTRUCTION::HANDOFF);
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
			auto TerminatingEvent = TWorkerThread::StateNotify(_T("TestTerminating"), TWorkerThread::State::Terminating,
															   [](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
				_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
			});
			auto TerminatedEvent = TWorkerThread::StateNotify(_T("TestTerminated"), TWorkerThread::State::Terminated,
															  [](TWorkerThread &WT, TWorkerThread::State const &State) throw() {
				_LOG(_T("- Worker thread '%s', State [%s]"), WT.Name.c_str(), TWorkerThread::STR_State(State));
			});
			ManagedRef<TWorkerThread> I(TWorkerThread::Create(_T("TestI"), *DefaultObjAllocator<TestDelayRunnable>().Create()), CONSTRUCTION::HANDOFF);
			I->Start();
			_LOG(_T("Going out-of-scope, expect signal terminate and wait for 1 second..."));
		}
	}

	_LOG(_T("*** Test WorkerThread and SyncObj (Threading correctness)"));
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
				return TFixedBuffer(nullptr);
			}
		};

		ManagedRef<TWorkerThread> TestCount0(CONSTRUCTION::EMPLACE, _T("CounterThread0"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCount1(CONSTRUCTION::EMPLACE, _T("CounterThread1"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCount2(CONSTRUCTION::EMPLACE, _T("CounterThread2"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCount3(CONSTRUCTION::EMPLACE, _T("CounterThread3"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCount4(CONSTRUCTION::EMPLACE, _T("CounterThread4"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCount5(CONSTRUCTION::EMPLACE, _T("CounterThread5"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCount6(CONSTRUCTION::EMPLACE, _T("CounterThread6"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCount7(CONSTRUCTION::EMPLACE, _T("CounterThread7"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCount8(CONSTRUCTION::EMPLACE, _T("CounterThread8"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCount9(CONSTRUCTION::EMPLACE, _T("CounterThread9"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCountA(CONSTRUCTION::EMPLACE, _T("CounterThreadA"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCountB(CONSTRUCTION::EMPLACE, _T("CounterThreadB"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCountC(CONSTRUCTION::EMPLACE, _T("CounterThreadC"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCountD(CONSTRUCTION::EMPLACE, _T("CounterThreadD"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCountE(CONSTRUCTION::EMPLACE, _T("CounterThreadE"), *DefaultObjAllocator<TestCount>().Create());
		ManagedRef<TWorkerThread> TestCountF(CONSTRUCTION::EMPLACE, _T("CounterThreadF"), *DefaultObjAllocator<TestCount>().Create());

		_LOG(_T("--- Start Counting..."));
		TestCount0->Start({&X, NullAlloc}); TestCount1->Start({&X, NullAlloc}); TestCount2->Start({&X, NullAlloc}); TestCount3->Start({&X, NullAlloc});
		TestCount4->Start({&X, NullAlloc}); TestCount5->Start({&X, NullAlloc}); TestCount6->Start({&X, NullAlloc}); TestCount7->Start({&X, NullAlloc});
		TestCount8->Start({&X, NullAlloc}); TestCount9->Start({&X, NullAlloc});	TestCountA->Start({&X, NullAlloc});	TestCountB->Start({&X, NullAlloc});
		TestCountC->Start({&X, NullAlloc}); TestCountD->Start({&X, NullAlloc}); TestCountE->Start({&X, NullAlloc}); TestCountF->Start({&X, NullAlloc});

		WaitMultiple({*TestCount0, *TestCount1, *TestCount2, *TestCount3,
					 *TestCount4, *TestCount5, *TestCount6, *TestCount7,
					 *TestCount8, *TestCount9, *TestCountA, *TestCountB,
					 *TestCountC, *TestCountD, *TestCountE, *TestCountF}, true);

		_LOG(_T("--- Finished All Counting..."));
		_LOG(_T("X = %d"), X.Pickup()->value);
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
		this->d_condition.notify_one();
	}
	T pop() {
		std::unique_lock<std::mutex> lock(this->d_mutex);
		this->d_condition.wait(lock, [=] { return !this->d_queue.empty(); });
		T rc(std::move(this->d_queue.back()));
		this->d_queue.pop_back();
		return rc;
	}
};

void TestSyncQueue(void) {
	_LOG(_T("*** Test SyncQueue (Non-threading correctness)"));
	{
		TSyncBlockingDeque<int> TestQueue(_T("TestQueue1"));
		_LOG(_T("Push 123 : %d"), TestQueue.Push_Back(123));
		int A = 0;
		_LOG(_T("A = %d"), A);
		_LOG(_T("EmptyLock (Wait 0.5 seconds and expect failure)"));
		if (TestQueue.EmptyLock(500)) {
			FAIL(_T("Should not reach"));
		} else _LOG(_T("Failed to lock empty queue (expected)"));

		_LOG(_T("Pop -> A : %d"), TestQueue.Pop_Front(A));
		_LOG(_T("A = %d"), A);
		_LOG(_T("Pop -> A (Wait 0.5 seconds and expect failure)"));
		if (TestQueue.Pop_Front(A, 500)) {
			FAIL(_T("Should not reach"));
		} else { _LOG(_T("Failed to dequeue (expected)")); }
		_LOG(_T("EmptyLock (Expect immediate success)"));
		if (TestQueue.EmptyLock(500)) {
			_LOG(_T("Locked empty queue!"));
		} else { FAIL(_T("Failed to lock empty queue (unexpected)")); }
	}

	_LOG(_T("*** Test SyncQueue (Threading correctness)"));
	if (IsDebuggerPresent()) {
		_LOG(_T("!!! Because you have debugger attached, the performance is NOT accurate"));
	}
	{
		typedef TSyncBlockingDeque<int> TSyncIntQueue;
		class TestQueuePut : public TRunnable {
		protected:
			TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
				TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);

				int COUNT = 5000000;
				TimeStamp StartTime = TimeStamp::Now();
				for (int i = 0; i < COUNT; i++) Q.Push_Back(i);
				TimeStamp EndTime = TimeStamp::Now();

				double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
				_LOG(_T("Enqueue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
				return TFixedBuffer(nullptr);
			}
		};

		class TestQueueGet : public TRunnable {
		protected:
			TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
				TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);

				int COUNT = 5000000;
				TimeStamp StartTime = TimeStamp::Now();
				int j;
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
		ManagedRef<TWorkerThread> PutThread(CONSTRUCTION::EMPLACE, _T("QueuePutThread"), *DefaultObjAllocator<TestQueuePut>().Create());
		ManagedRef<TWorkerThread> GetThread(CONSTRUCTION::EMPLACE, _T("QueueGetThread"), *DefaultObjAllocator<TestQueueGet>().Create());
		_LOG(_T("--- Starting Parallel Producer & Consumer..."));
		PutThread->Start({&Queue, NullAlloc});
		GetThread->Start({&Queue, NullAlloc});
		GetThread->WaitFor();
		_LOG(_T("--- Finished All Queue Operation..."));
		Queue.AdjustSize();

		ManagedRef<TWorkerThread> PutThread2(CONSTRUCTION::EMPLACE, _T("QueuePutThread"), *DefaultObjAllocator<TestQueuePut>().Create());
		ManagedRef<TWorkerThread> GetThread2(CONSTRUCTION::EMPLACE, _T("QueueGetThread"), *DefaultObjAllocator<TestQueueGet>().Create());
		_LOG(_T("--- Starting Parallel Producer & Consumer..."));
		PutThread2->Start({&Queue, NullAlloc});
		GetThread2->Start({&Queue, NullAlloc});
		{
			_LOG(_T("Sleeping for 0.1 sec..."));
			TDelayWaitable WaitASec(100);
			WaitASec.WaitFor(FOREVER);
		}
		{
			_LOG(_T("Trying to grab empty lock..."));
			auto ELock = Queue.EmptyLock();
			_LOG(_T("Current queue length: %d"), Queue.Length());
			_LOG(_T("Sleep for 2 sec..."));
			TDelayWaitable WaitASec(2000);
			WaitASec.WaitFor(FOREVER);
			Queue.AdjustSize();
			_LOG(_T("Releasing empty lock..."));
		}
		{
			_LOG(_T("Sleeping for 0.1 sec..."));
			TDelayWaitable WaitASec(100);
			WaitASec.WaitFor(FOREVER);
		}
		{
			_LOG(_T("Trying to grab empty lock..."));
			auto ELock = Queue.EmptyLock();
			_LOG(_T("Current queue length: %d"), Queue.Length());
			_LOG(_T("Sleep for 2 sec..."));
			TDelayWaitable WaitASec(2000);
			WaitASec.WaitFor(FOREVER);
			Queue.AdjustSize();
			_LOG(_T("Releasing empty lock..."));
		}
		GetThread2->WaitFor();
		_LOG(_T("--- Finished All Queue Operation..."));
	}

	{
		typedef TQueue<int> TSyncIntQueue;
		class TestQueuePut : public TRunnable {
		protected:
			TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
				TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);

				int COUNT = 5000000;
				TimeStamp StartTime = TimeStamp::Now();
				for (int i = 0; i < COUNT; i++) Q.push(i);
				TimeStamp EndTime = TimeStamp::Now();

				double TimeSpan = (double)EndTime.From(StartTime).GetValue(TimeUnit::NSEC) / (unsigned long long)TimeUnit::SEC;
				_LOG(_T("Enqueue done! (%d ops in %.2f sec, %.2f ops/sec)"), COUNT, TimeSpan, COUNT / TimeSpan);
				return TFixedBuffer(nullptr);
			}
		};

		class TestQueueGet : public TRunnable {
		protected:
			TFixedBuffer Run(TWorkerThread &WorkerThread, TFixedBuffer &pSyncIntQueue) override {
				TSyncIntQueue& Q = *static_cast<TSyncIntQueue*>(*pSyncIntQueue);

				int COUNT = 5000000;
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
		ManagedRef<TWorkerThread> PutThread(CONSTRUCTION::EMPLACE, _T("QueuePutThread"), *DefaultObjAllocator<TestQueuePut>().Create());
		ManagedRef<TWorkerThread> GetThread(CONSTRUCTION::EMPLACE, _T("QueueGetThread"), *DefaultObjAllocator<TestQueueGet>().Create());
		_LOG(_T("--- Benchmarking Comparison Queue..."));
		PutThread->Start({&Queue, NullAlloc});
		GetThread->Start({&Queue, NullAlloc});
		GetThread->WaitFor();
		_LOG(_T("--- Finished All Queue Operation..."));

	}
}
