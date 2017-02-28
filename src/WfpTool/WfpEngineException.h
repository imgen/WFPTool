#pragma once

#ifndef WFP_ENGINE_EXCEPTION_H
#define WFP_ENGINE_EXCEPTION_H

#include "minwindef.h"
#include <exception>
#include "FilterApiErrors.h"

using namespace std;

class WfpEngineException : public exception
{
	UINT32 _errorCode;
	UINT32 _sysErrorCode;

public:
	WfpEngineException(char* errorMessage, UINT32 engineErrorCode);
	WfpEngineException(char* errorMessage, UINT32 engineErrorCode, UINT32 systemErrorCode);
	UINT32 GetErrorCode() const;
	UINT32 GetSystemErrorCode() const;

	static void ThrowIfSystemError(UINT32 systemErrorCode)
	{
		if (NO_ERROR != systemErrorCode)
			throw new WfpEngineException("Internal error", FILTER_API_ERROR_INTERNAL, systemErrorCode);
	}

};

#endif
