#pragma once

#ifndef FILTER_API_PROVIDER_H
#define FILTER_API_PROVIDER_H

#include "WfpFilter.h"
#include "WfpEngine.h"
#include "FilterApiExport.h"
#include <map>

namespace wfptool
{
	/**
	 * This class provides an api interface for filter operations.
	 * NOTE: It's not thread safe. Please keep that in mind, dear user of the class.
	 *
	 */
	class FilterApiProvider
	{
		friend class FilterApiProviderTests;

		typedef pair<WfpFilter*, const GUID*> FilterGuidPair;
		static const int DNS_SERVER_PORT = 53;

		FilterHandle _filterGroupHandleIndex = 0;
		WfpEngine _wfpEngine;
		map<const GUID*, WfpFilter*> _filters;
		map<FilterHandle, vector<const GUID*> > _filterGroups;
		FilterHandle _blockAllAndAllowLocalLoopbackFilterHandle = 0;

		void AddFilter(WfpFilter* filter, GUID* guid);
		void AddFilters(const vector<FilterGuidPair>& filters);
		FilterHandle CreateFilterGroup(const vector<FilterGuidPair>& filters);
		FilterHandle CreateFilterGroupByFilter(const FilterGuidPair& filter);
		FilterHandle CreateFilterGroupByFunc(function<void(vector<FilterGuidPair>&)> func);
		FilterHandle CreateFilterGroup(const vector<const GUID*>& filterGuids);
		FilterHandle CreateFilterGroupByGuid(const GUID* guid);
		static GUID* CreateFilterGuid();
		void RemoveFilter(const GUID* filterGuid);
		void Cleanup();

		static void DefineFilter(vector<FilterGuidPair>& filters, function<WfpFilter*(const GUID*)> definition);
	public:
		static const UINT32 DHCP_LOCAL_PORT = 68, DHCP_REMOTE_PORT = 67;

		~FilterApiProvider();
		FilterHandle AddBlockAllFilter();

		FilterHandle AddAllowTunnelFilter(const wstring& adapterId);

		FilterHandle AddBlockNonVpnDnsFilter(const wstring& adapterId);

		FilterHandle AddAllowSpecificFilter(const string& ip, UINT8 protocol, int port);

		FilterHandle AddAllowSpecificHostnameFilter(const string& hostname, UINT8 protocol, int port);

		FilterHandle AddAllowSubnet(UINT32 ip, UINT32 subnetMask);

		FilterHandle AddAllowDnsFilter();

		void RemoveFiltersByHandle(FilterHandle handle);

		void RemoveAllFilters();
	};
}

#endif
