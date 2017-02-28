#pragma once

#ifndef FILTER_API_EXPORT_H
#define FILTER_API_EXPORT_H

#include "minwindef.h"

#define CALL_CONVENTION __stdcall

#ifdef WFPTOOL_API_EXPORTS
#define DLLEXPORT
#else
#define DLLEXPORT
#endif

#ifdef __cplusplus
extern "C"
{
#endif

	typedef uintptr_t FilterHandle;

	void __SetLastLastFilterError(UINT32 error);

	void __ClearLastFilterError();

	DLLEXPORT UINT32 CALL_CONVENTION GetLastFilterApiError();

	DLLEXPORT FilterHandle CALL_CONVENTION AddBlockAllFilter();

	DLLEXPORT UINT32 CALL_CONVENTION AddBlockAllFilter2(FilterHandle* handle);

	DLLEXPORT FilterHandle CALL_CONVENTION AddAllowTunnelFilter(const wchar_t* adapterNameOrId);

	DLLEXPORT FilterHandle CALL_CONVENTION AddBlockNonVpnDnsFilter(const wchar_t* adapterNameOrId);
	
	DLLEXPORT UINT32 CALL_CONVENTION AddBlockNonVpnDnsFilter2(FilterHandle* handle, const wchar_t* adapterNameOrId);

	DLLEXPORT UINT32 CALL_CONVENTION AddAllowTunnelFilter2(FilterHandle* handle, const wchar_t* adapterNameOrId);

	DLLEXPORT FilterHandle CALL_CONVENTION AddAllowDnsFilter();

	DLLEXPORT UINT32 CALL_CONVENTION AddAllowDnsFilter2(FilterHandle* handle);

	DLLEXPORT FilterHandle CALL_CONVENTION AddAllowSpecificFilter(const char* ip, uintptr_t protocol, uintptr_t port);

	DLLEXPORT UINT32 CALL_CONVENTION AddAllowSpecificFilter2(FilterHandle* handle, const char* ip, uintptr_t protocol, uintptr_t port);

	DLLEXPORT FilterHandle CALL_CONVENTION AddAllowSpecificHostnameFilter(const char* hostname, uintptr_t protocol, uintptr_t port);

	DLLEXPORT UINT32 CALL_CONVENTION AddAllowSpecificHostnameFilter2(FilterHandle* handle, const char* hostname, uintptr_t protocol, uintptr_t port);

	DLLEXPORT FilterHandle CALL_CONVENTION AddAllowSubnet(uintptr_t ip, uintptr_t subnetMask);

	DLLEXPORT UINT32 CALL_CONVENTION AddAllowSubnet2(FilterHandle* handle, uintptr_t ip, uintptr_t subnetMask);

	DLLEXPORT BOOL CALL_CONVENTION RemoveFilter(FilterHandle handle);

	DLLEXPORT UINT32 CALL_CONVENTION RemoveFilter2(FilterHandle handle);

	DLLEXPORT BOOL CALL_CONVENTION RemoveAllFilters();

	DLLEXPORT UINT32 CALL_CONVENTION RemoveAllFilters2();
#ifdef __cplusplus
}
#endif

#endif
