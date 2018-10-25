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

// [Utilities] Exception support

#include "Exception.h"

#include "Logging.h"
#include "Memory/ObjAllocator.h"

#define ExceptSourceNone	_T("(unknown source)")
#define ExceptReasonNone	_T("(unknown reason)")
#define FExceptionMessage	_T("Exception @ [%s]: %s")

//#define __EXCEPTION_MEMDEBUG__

// --- Exception
LPSTR Exception::STR_STD_EXCEPTION_WHAT = "ZWUtils Exception";

Exception::Exception(_this &&xException) NOEXCEPT
	: Source(std::move(xException.Source))
	, Reason(std::move(xException.Reason)) {
#ifdef __EXCEPTION_MEMDEBUG__
	LOG(_T("------------- Exception copy constructed!"));
#endif
}

Exception::Exception(_this const &xException)
	: Source(xException.Source), Reason(xException.Reason) {
#ifdef __EXCEPTION_MEMDEBUG__
	LOG(_T("------------- Exception move constructed!"));
#endif
}

Exception::~Exception(void) {
#ifdef __EXCEPTION_MEMDEBUG__
	LOG(_T("------------- Exception freed!"));
#endif
}

Exception* Exception::MakeClone(IObjAllocator<void> &_Alloc) const {
	return DEFAULT_NEW(Exception, *this);
}

TString const& Exception::Why(void) const {
	if (rWhy.empty()) {
		PCTCHAR dSource = Source.empty() ? ExceptSourceNone : Source.c_str();
		PCTCHAR dReason = Reason.empty() ? ExceptReasonNone : Reason.c_str();
		STR_ERRMSGFMT(&rWhy, FExceptionMessage, dSource, dReason);
	}
	return rWhy;
}

// Display the error message via DEBUG output
void Exception::Show(void) const {
	_LOG(_T("%s"), Why().c_str());
}

// --- STDException
LPTSTR STDException::STR_STD_EXCEPTION_WRAP = _T("Wrapped std::exception");

STDException* STDException::MakeClone(IObjAllocator<void> &_Alloc) const {
	return DEFAULT_NEW(STDException, *this);
}

STDException* STDException::Wrap(std::exception &&xException) {
	auto STrace = STException::TraceStack(1);
	return DEFAULT_NEW(STDException, STrace.empty() ? TString() : std::move(STrace.front()), std::move(xException));
}

// --- STException
STException* STException::MakeClone(IObjAllocator<void> &_Alloc) const {
	return DEFAULT_NEW(STException, *this);
}

TString STException::ExtractTopFrame(std::deque<TString> &xStackTrace) {
	TString Ret;
	if (!xStackTrace.empty()) {
		Ret = std::move(xStackTrace.front());
		xStackTrace.pop_front();
	}
	return Ret;
}

void STException::Show(void) const {
	Exception::Show();
	ShowStack();
}

void STException::ShowStack(void) const {
	_LOG(_T("-------- Stack Trace --------"));
	for (auto &entry : rStackTrace)
		_LOG(_T("%s"), entry.c_str());
	_LOG(_T("-----------------------------"));
}

#define TraceFailureMessage	_T("(Failed to trace call stack)")

#ifdef WINDOWS

#include "Memory/Resource.h"

#include "StackWalker.h"

std::deque<TString> STException::TraceStack(int PopFrame) {
	CONTEXT CurContext;
	RtlCaptureContext(&CurContext);

	std::deque<TString> StrTrace;
	if (!LocalStackTrace(
		THandle::Dummy(GetCurrentThread()), CurContext,
		[&](TStackWalker::CallstackEntry const& Entry) {
			if (PopFrame-- < 0) StrTrace.emplace_back(TStackWalker::FormatEntry(Entry));
			return true;
		})) {
		StrTrace.emplace_back(TraceFailureMessage);
	}
	return StrTrace;
}

// --- SEHException

#define FSEHMessage	_T("%s @%p")
#define AccessOp_Read _T("READ")
#define AccessOp_Write _T("WRITE")
#define AccessOp_Execute _T("EXECUTE")
#define AccessOp_Unknown _T("UNKNOWN-OP")

TString SEHException::STR_ExceptCode(PEXCEPTION_RECORD ExcRecord) {
	switch (ExcRecord->ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:
		{
			LPCTSTR SUBTYPE = AccessOp_Unknown;
			switch (ExcRecord->ExceptionInformation[0]) {
				case 0: SUBTYPE = AccessOp_Read; break;
				case 1: SUBTYPE = AccessOp_Write; break;
				case 8: SUBTYPE = AccessOp_Execute; break;
			}
			return TStringCast(_T("ACCESS VIOLATION, ") << SUBTYPE << _T(" OF ADDRESS ")
							   << (PVOID)ExcRecord->ExceptionInformation[1]);
		}
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			return TStringCast(_T("ARRAY BOUND EXCEEDED"));
		case EXCEPTION_BREAKPOINT:
			return TStringCast(_T("BREAKPOINT"));
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			return TStringCast(_T("DATATYPE MISALIGNMENT"));
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			return TStringCast(_T("FLOATING-POINT OPERAND IS DENORMAL"));
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			return TStringCast(_T("FLOATING-POINT DIVISION BY ZERO"));
		case EXCEPTION_FLT_INEXACT_RESULT:
			return TStringCast(_T("FLOATING-POINT RESULT IS INEXACT"));
		case EXCEPTION_FLT_INVALID_OPERATION:
			return TStringCast(_T("FLOATING-POINT INVALID OPERATION"));
		case EXCEPTION_FLT_OVERFLOW:
			return TStringCast(_T("FLOATING-POINT VALUE OVERFLOW"));
		case EXCEPTION_FLT_STACK_CHECK:
			return TStringCast(_T("FLOATING-POINT STACK ERROR"));
		case EXCEPTION_FLT_UNDERFLOW:
			return TStringCast(_T("FLOATING-POINT VALUE UNDERFLOW"));
		case EXCEPTION_GUARD_PAGE:
			return TStringCast(_T("GUARD PAGE ACCESS"));
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			return TStringCast(_T("ILLEGAL INSTRUCTION"));
		case EXCEPTION_IN_PAGE_ERROR:
		{
			LPCTSTR SUBTYPE = AccessOp_Unknown;
			switch (ExcRecord->ExceptionInformation[0]) {
				case 0: SUBTYPE = AccessOp_Read; break;
				case 1: SUBTYPE = AccessOp_Write; break;
				case 8: SUBTYPE = AccessOp_Execute; break;
			}
			return TStringCast(_T("PAGE-IN ERROR, ") << SUBTYPE << _T(" OF ADDRESS ")
							   << (PVOID)ExcRecord->ExceptionInformation[1] << _T(" DUE TO ")
							   << std::hex << ExcRecord->ExceptionInformation[2]);
		}
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			return TStringCast(_T("INTEGER DIVISION BY ZERO"));
		case EXCEPTION_INT_OVERFLOW:
			return TStringCast(_T("INTEGER VALUE OVERFLOW"));
		case EXCEPTION_INVALID_DISPOSITION:
			return TStringCast(_T("INVALID EXCEPTION DISPOSITION"));
		case EXCEPTION_INVALID_HANDLE:
			return TStringCast(_T("INVALID HANDLE"));
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			return TStringCast(_T("EXCEPTION IS NON-CONTINUABLE"));
		case EXCEPTION_PRIV_INSTRUCTION:
			return TStringCast(_T("PRIVILEGED INSTRUCTION"));
		case EXCEPTION_SINGLE_STEP:
			return TStringCast(_T("SINGLE STEP TRAP"));
		case EXCEPTION_STACK_OVERFLOW:
			return TStringCast(_T("STACK OVERFLOW"));
		default:
			return TStringCast(std::hex << ExcRecord->ExceptionCode << _T('(') << ExcRecord->ExceptionFlags << _T(')'));
	}
}

SEHException::SEHException(TString const &xSource, PEXCEPTION_RECORD ExcRecord, std::deque<TString> &&xStackTrace) :
	STException(xSource, std::move(xStackTrace), FSEHMessage, STR_ExceptCode(ExcRecord).c_str(), ExcRecord->ExceptionAddress) {
	// Do Nothing
}

SEHException::SEHException(TString &&xSource, PEXCEPTION_RECORD ExcRecord, std::deque<TString> &&xStackTrace) :
	STException(std::move(xSource), std::move(xStackTrace), FSEHMessage, STR_ExceptCode(ExcRecord).c_str(), ExcRecord->ExceptionAddress) {
	// Do Nothing
}

SEHException::SEHException(PEXCEPTION_RECORD ExcRecord, std::deque<TString> &&xStackTrace) :
	STException(std::move(xStackTrace), FSEHMessage, STR_ExceptCode(ExcRecord).c_str(), ExcRecord->ExceptionAddress) {
	// Do Nothing
}

void SEHException::Translator(unsigned int ExcCode, PEXCEPTION_POINTERS ExcPtr) {
	std::deque<TString> StrTrace;
	CONTEXT ExcContext = *ExcPtr->ContextRecord;
	if (!LocalStackTrace(
		THandle::Dummy(GetCurrentThread()), ExcContext,
		[&](TStackWalker::CallstackEntry const &Entry) {
			return StrTrace.emplace_back(TStackWalker::FormatEntry(Entry)), true;
		})) {
		StrTrace.emplace_back(TraceFailureMessage);
	}
	throw SEHException(_T("<SEH Exception Translator>"), ExcPtr->ExceptionRecord, std::move(StrTrace));
}

bool SEHTranslation = false;

bool ControlSEHTranslation(bool Enable) {
	if (Enable) {
		if (!SEHTranslation) {
			LOG(_T("! Enabling SEH translation..."));
			return _set_se_translator(SEHException::Translator), SEHTranslation = true;
		}
	} else {
		if (SEHTranslation) {
			LOG(_T("! Disabling SEH translation..."));
			return _set_se_translator(nullptr), SEHTranslation = false, true;
		}
	}
	return false;
}

#endif