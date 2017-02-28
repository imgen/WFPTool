#include "stdafx.h"
#include "WfpEngineException.h"

WfpEngineException::WfpEngineException(char* errorMessage, UINT32 engineErrorCode)
	: WfpEngineException(errorMessage, engineErrorCode, NO_ERROR)
{
}

inline WfpEngineException::WfpEngineException(char* errorMessage, UINT32 engineErrorCode, UINT32 systemErrorCode)
	: exception(errorMessage), _errorCode(engineErrorCode), _sysErrorCode(systemErrorCode)
{

}


UINT32 WfpEngineException::GetErrorCode() const
{
	return _errorCode;
}

UINT32 WfpEngineException::GetSystemErrorCode() const
{
	return _sysErrorCode;
}
