#include "stdafx.h"
#include "FilterApiExport.h"
#include "FilterApiProvider.h"
#include "WfpEngineException.h"
#include <functional>
#include <mutex>

using namespace wfptool;

static FilterApiProvider filterApiProvider;

static UINT32 lastFilterApiError = FILTER_API_NO_ERROR;

static mutex filterApiMutex;

template<class TReturn>
static TReturn WithErrorHandlingAndLock(function<TReturn()> func,
	bool returnErrorCode = false)
{
	TReturn returnValue = 0;
	try
	{
		filterApiMutex.lock();
		__ClearLastFilterError();
		returnValue = func();
		filterApiMutex.unlock();
		return returnValue;
	}
	catch (WfpEngineException ex)
	{
		UINT32 errCode = ex.GetErrorCode();
		if (errCode == FILTER_API_ERROR_INTERNAL)
		{
			errCode = ex.GetSystemErrorCode();
		}
		__SetLastLastFilterError(errCode);
		if (returnErrorCode)
		{
			returnValue = errCode;
		}
		filterApiMutex.unlock();
		return returnValue;
	}
}

static void WithErrorHandlingAndLock(function<void()> func)
{
	WithErrorHandlingAndLock(static_cast<function<int()>>([&func]()
		{
			func();
			return 0;
		}));
}

void __SetLastLastFilterError(UINT32 error)
{
	lastFilterApiError = error;
}

void __ClearLastFilterError()
{
	lastFilterApiError = FILTER_API_NO_ERROR;
}

DLLEXPORT UINT32 CALL_CONVENTION GetLastFilterApiError()
{
	return lastFilterApiError;
}

DLLEXPORT FilterHandle CALL_CONVENTION AddBlockAllFilter()
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([]() -> FilterHandle
		{
			return filterApiProvider.AddBlockAllFilter();
		}));
}

DLLEXPORT UINT32 CALL_CONVENTION AddBlockAllFilter2(FilterHandle* handle)
{
	return WithErrorHandlingAndLock(static_cast<function<UINT32()>>([&handle]() -> FilterHandle
	{
		*handle = filterApiProvider.AddBlockAllFilter();
		return GetLastFilterApiError();
	}), true);
}

DLLEXPORT FilterHandle CALL_CONVENTION AddAllowTunnelFilter(const wchar_t* adapterNameOrId)
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([&adapterNameOrId]() -> FilterHandle
		{
			return filterApiProvider.AddAllowTunnelFilter(adapterNameOrId);
		}));
}

DLLEXPORT UINT32 CALL_CONVENTION AddAllowTunnelFilter2(FilterHandle* handle, const wchar_t* adapterNameOrId)
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([&]() -> FilterHandle
	{
		*handle = filterApiProvider.AddAllowTunnelFilter(adapterNameOrId);
		return GetLastFilterApiError();
	}), true);
}

DLLEXPORT FilterHandle CALL_CONVENTION AddBlockNonVpnDnsFilter(const wchar_t* adapterNameOrId)
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([&adapterNameOrId]() -> FilterHandle
	{
		return filterApiProvider.AddBlockNonVpnDnsFilter(adapterNameOrId);
	}));
}

DLLEXPORT UINT32 CALL_CONVENTION AddBlockNonVpnDnsFilter2(FilterHandle* handle, const wchar_t* adapterNameOrId)
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([&]() -> FilterHandle
	{
		*handle = filterApiProvider.AddBlockNonVpnDnsFilter(adapterNameOrId);
		return GetLastFilterApiError();
	}), true);
}

DLLEXPORT FilterHandle CALL_CONVENTION AddAllowDnsFilter()
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([]() -> FilterHandle
		{
			return filterApiProvider.AddAllowDnsFilter();
		}));
}

DLLEXPORT UINT32 CALL_CONVENTION AddAllowDnsFilter2(FilterHandle* handle)
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([&handle]() -> FilterHandle
		{
			*handle = filterApiProvider.AddAllowDnsFilter();
			return GetLastFilterApiError();
		}), true);
}

DLLEXPORT FilterHandle CALL_CONVENTION AddAllowSpecificFilter(const char* ip, uintptr_t protocol, uintptr_t port)
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([&]() -> FilterHandle
		{
			if (ip == nullptr)
			{
				ip = "";
			}
			return filterApiProvider.AddAllowSpecificFilter(ip, protocol, port);
		}));
}

DLLEXPORT UINT32 CALL_CONVENTION AddAllowSpecificFilter2(FilterHandle* handle, const char* ip, uintptr_t protocol, uintptr_t port)
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([&]() -> FilterHandle
	{
		if (ip == nullptr)
		{
			ip = "";
		}
		*handle = filterApiProvider.AddAllowSpecificFilter(ip, protocol, port);
		return GetLastFilterApiError();
	}), true);
}

DLLEXPORT FilterHandle CALL_CONVENTION AddAllowSpecificHostnameFilter(const char* hostname, uintptr_t protocol, uintptr_t port)
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([&]() -> FilterHandle
	{
		if (hostname == nullptr)
		{
			hostname = "";
		}
		return filterApiProvider.AddAllowSpecificHostnameFilter(hostname, protocol, port);
	}));
}

DLLEXPORT UINT32 CALL_CONVENTION AddAllowSpecificHostnameFilter2(FilterHandle* handle, const char* hostname, uintptr_t protocol, uintptr_t port)
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([&]() -> FilterHandle
	{
		if (hostname == nullptr)
		{
			hostname = "";
		}
		*handle = filterApiProvider.AddAllowSpecificHostnameFilter(hostname, protocol, port);
		return GetLastFilterApiError();
	}), true);
}

DLLEXPORT FilterHandle CALL_CONVENTION AddAllowSubnet(uintptr_t ip, uintptr_t subnetMask)
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([&]() -> FilterHandle
	{
		return filterApiProvider.AddAllowSubnet(ip, subnetMask);
	}));
}

DLLEXPORT UINT32 CALL_CONVENTION AddAllowSubnet2(FilterHandle* handle, uintptr_t ip, uintptr_t subnetMask)
{
	return WithErrorHandlingAndLock(static_cast<function<FilterHandle()>>([&]() -> FilterHandle
	{
		*handle = filterApiProvider.AddAllowSubnet(ip, subnetMask);
		return GetLastFilterApiError();
	}), true);
}


DLLEXPORT BOOL CALL_CONVENTION RemoveFilter(FilterHandle handle)
{
	return WithErrorHandlingAndLock(static_cast<function<BOOL()>>([&handle]()
		{
			filterApiProvider.RemoveFiltersByHandle(handle);
			return GetLastFilterApiError() == NO_ERROR? TRUE : FALSE;
		}));
}

DLLEXPORT UINT32 CALL_CONVENTION RemoveFilter2(FilterHandle handle)
{
	return WithErrorHandlingAndLock(static_cast<function<BOOL()>>([&handle]()
	{
		filterApiProvider.RemoveFiltersByHandle(handle);
		return GetLastFilterApiError();
	}), true);
}

DLLEXPORT BOOL CALL_CONVENTION RemoveAllFilters()
{
	return WithErrorHandlingAndLock(static_cast<function<BOOL()>>([&]()
		{
			filterApiProvider.RemoveAllFilters();
			return GetLastFilterApiError() == NO_ERROR ? TRUE : FALSE;
		}));
}

DLLEXPORT UINT32 CALL_CONVENTION RemoveAllFilters2()
{
	return WithErrorHandlingAndLock(static_cast<function<BOOL()>>([&]()
	{
		filterApiProvider.RemoveAllFilters();
		return GetLastFilterApiError();
	}), true);
}

