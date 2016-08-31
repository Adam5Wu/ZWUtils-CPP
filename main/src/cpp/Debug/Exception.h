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

/**
 * @addtogroup Utilities Basic Supporting Utilities
 * @file
 * @brief Exception support
 * @author Zhenyu Wu
 * @date Jan 06, 2015: Refactored from legacy ZWUtils
 **/

#ifndef ZWUtils_Exception_H
#define ZWUtils_Exception_H

#include "Misc/Global.h"
#include "Misc/TString.h"

#include "Debug.h"

template<typename T> class IObjAllocator;

/**
 * @ingroup Utilities
 * @brief Generic exception class
 *
 * Contains useful information about the exception: Source, Reason
 **/
class Exception {
	typedef Exception _this;
	friend class IObjAllocator < _this >;

protected:
	TString const rWhy = EmptyWText;

	template<typename... Params>
	void PopulateReason(PCTCHAR ReasonFmt, Params&&... xParams)
	{ if (ReasonFmt) STR_ERRMSGFMT(const_cast<TString*>(&Reason), ReasonFmt, xParams...); }

	template<typename... Params>
	Exception(TString const &xSource, PCTCHAR ReasonFmt, Params&&... xParams) : Source(xSource)
	{ PopulateReason(ReasonFmt, xParams...); }

	template<typename... Params>
	Exception(TString &&xSource, PCTCHAR ReasonFmt, Params&&... xParams) : Source(std::move(xSource))
	{ PopulateReason(ReasonFmt, xParams...); }

	virtual ~Exception(void) {}

public:
	TString const Source;
	TString const Reason = EmptyWText;

	/**
	 * Returns a description of the exception
	 * @note The caller does NOT have ownership of the returned string
	 **/
	virtual TString const& Why(void) const;

	/**
	 * Prints the description of the exception via @link LOG() DEBUG printing function @endlink
	 **/
	virtual void Show(void) const;

	/**
	 * Create an exception object with given source, reason, and an optional helper integer
	 *
	 * @note Use the @link FAIL() \p FAIL* @endlink macros to retrive source info automatically
	 **/
	template<typename... Params>
	static _this* Create(TString const &xSource, PCTCHAR ReasonFmt, Params&&... xParams);

	template<typename... Params>
	static _this* Create(TString &&xSource, PCTCHAR ReasonFmt, Params&&... xParams);
};

//! @ingroup Utilities
//! Raise an exception with a formatted string message
#define FAILS(src, fmt, ...)	throw Exception::Create(src, fmt VAWRAP(__VA_ARGS__));
#define FAIL(fmt, ...) {							\
	SOURCEMARK										\
	FAILS(std::move(__SrcMark), fmt, __VA_ARGS__);	\
}

#define LOGEXCEPTIONV(e,fmt,...)							\
DEBUGV_DO_OR({												\
	LOGV(fmt, __VA_ARGS__);									\
	e->Show();												\
}, {														\
	LOG(fmt _T(" - %s"), __VA_ARGS__, e->Why().c_str());	\
})

#define LOGEXCEPTIONVV(e,fmt,...)							\
DEBUGVV_DO_OR({												\
	LOGV(fmt, __VA_ARGS__);									\
	e->Show();												\
}, {														\
	LOG(fmt _T(" - %s"), __VA_ARGS__, e->Why().c_str());	\
})

#include <deque>

class STException : public Exception {
	typedef STException _this;

protected:
	static TString ExtractTopFrame(std::deque<TString const> &xStackTrace);

	template<typename... Params>
	STException(TString &&xSource, std::deque<TString const> &&xStackTrace, PCTCHAR ReasonFmt, Params&&... xParams) :
		Exception(std::move(xSource), ReasonFmt, xParams...), rStackTrace(std::move(xStackTrace)) {}

	template<typename... Params>
	STException(TString const &xSource, std::deque<TString const> && xStackTrace, PCTCHAR ReasonFmt, Params&&... xParams) :
		Exception(xSource, ReasonFmt, xParams...), rStackTrace(std::move(xStackTrace)) {}

	template<typename... Params>
	STException(std::deque<TString const> && xStackTrace, PCTCHAR ReasonFmt, Params&&... xParams) :
		Exception(ExtractTopFrame(xStackTrace), ReasonFmt, xParams...), rStackTrace(std::move(xStackTrace)) {}

public:
	std::deque<TString const> const rStackTrace;

	void Show(void) const override;

	static std::deque<TString const> TraceStack(int PopFrame = 1);

	/**
	 * Create an exception object with given source, reason, and an optional helper integer
	 *
	 * @note Use the @link FAIL() \p FAIL* @endlink macros to retrive source info automatically
	 **/
	template<typename... Params>
	static _this* Create(TString const &xSource, std::deque<TString const> && xStackTrace, PCTCHAR ReasonFmt, Params&&... xParams);

	template<typename... Params>
	static _this* Create(TString &&xSource, std::deque<TString const> && xStackTrace, PCTCHAR ReasonFmt, Params&&... xParams);

	template<typename... Params>
	static _this* Create(std::deque<TString const> && xStackTrace, PCTCHAR ReasonFmt, Params&&... xParams);
};

#define FAILST(fmt, ...) throw STException::Create(STException::TraceStack(), fmt VAWRAP(__VA_ARGS__));

#ifdef WINDOWS

class SEHException : public STException {
protected:
	static TString STR_ExceptCode(PEXCEPTION_RECORD ExcRecord);

	SEHException(TString const &xSource, PEXCEPTION_RECORD ExcRecord, std::deque<TString const> &&xStackTrace);
	SEHException(TString &&xSource, PEXCEPTION_RECORD ExcRecord, std::deque<TString const> &&xStackTrace);
	SEHException(PEXCEPTION_RECORD ExcRecord, std::deque<TString const> &&xStackTrace);

public:
	static void Translator(unsigned int ExcCode, PEXCEPTION_POINTERS ExcPtr);
};

bool ControlSEHTranslation(bool Enable);

#endif

#include "Memory/ObjAllocator.h"

template<typename... Params>
static Exception* Exception::Create(TString const &xSource, PCTCHAR ReasonFmt, Params&&... xParams) {
	return DefaultObjAllocator<Exception>().Create(RLAMBDANEW(Exception, xSource, ReasonFmt, xParams...));
}

template<typename... Params>
static Exception* Exception::Create(TString &&xSource, PCTCHAR ReasonFmt, Params&&... xParams) {
	return DefaultObjAllocator<Exception>().Create(RLAMBDANEW(Exception, std::move(xSource), ReasonFmt, xParams...));
}

template<typename... Params>
static STException* STException::Create(TString const &xSource, std::deque<TString const> && xStackTrace, PCTCHAR ReasonFmt, Params&&... xParams) {
	return DefaultObjAllocator<STException>().Create(RLAMBDANEW(STException, xSource, std::move(xStackTrace), ReasonFmt, xParams...));
}

template<typename... Params>
static STException* STException::Create(TString &&xSource, std::deque<TString const> && xStackTrace, PCTCHAR ReasonFmt, Params&&... xParams) {
	return DefaultObjAllocator<STException>().Create(RLAMBDANEW(STException, std::move(xSource), std::move(xStackTrace), ReasonFmt, xParams...));
}

template<typename... Params>
static STException* STException::Create(std::deque<TString const> && xStackTrace, PCTCHAR ReasonFmt, Params&&... xParams) {
	return DefaultObjAllocator<STException>().Create(RLAMBDANEW(STException, std::move(xStackTrace), ReasonFmt, xParams...));
}

#endif
