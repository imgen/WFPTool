#include "stdafx.h"
#include "FilterApiProvider.h"
#include "WfpFilter.h"
#include "FilterApiErrors.h"
#include "NetworkAdapterUtils.h"

namespace wfptool
{
	FilterApiProvider::~FilterApiProvider()
	{
		RemoveAllFilters();
	}

	void FilterApiProvider::DefineFilter(
		vector<FilterGuidPair>& filters,
		function<WfpFilter* (const GUID*)> definition)	{
		auto guid = CreateFilterGuid();
		auto filter = definition(guid);
		filters.push_back(FilterGuidPair(filter, guid));
	}

	FilterHandle FilterApiProvider::AddBlockAllFilter()
	{
		if (_blockAllAndAllowLocalLoopbackFilterHandle)
		{
			__SetLastLastFilterError(FILTER_API_ERROR_FILTER_ALREADY_ENABLED);
			return 0;
		}

		return _blockAllAndAllowLocalLoopbackFilterHandle = CreateFilterGroupByFunc(
			[](vector<FilterGuidPair>& filters)
			{
				DefineFilter(
					filters,
					[](const GUID* filterGuid) {
					return new WfpFilter(
						*filterGuid,
						MakeName(L"block all ipv4 outbound filter"),
						FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
						1,
						FWP_ACTION_BLOCK);
				});

				DefineFilter(
					filters,
					[](const GUID* filterGuid) {
					return new WfpFilter(
						*filterGuid,
						MakeName(L"block all outbound ipv6 filter"),
						FWPM_LAYER_OUTBOUND_TRANSPORT_V6,
						1,
						FWP_ACTION_BLOCK);
				});

				DefineFilter(
					filters,
					[](const GUID* filterGuid) {
					auto filter = new WfpFilter(*filterGuid,
						MakeName(L"allow outbound local loopback filter"),
						FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
						0xF,
						FWP_ACTION_PERMIT);
					filter->AddCondition(
						FWPM_CONDITION_LOCAL_INTERFACE_TYPE,
						FWP_MATCH_EQUAL,
						FWP_UINT32,
						MIB_IF_TYPE_LOOPBACK);
					return filter;
				});

				DefineFilter(
					filters,
					[](const GUID* filterGuid) {
					auto filter = new WfpFilter(*filterGuid,
						MakeName(L"allow outbound local IPV6 loopback filter"),
						FWPM_LAYER_OUTBOUND_TRANSPORT_V6,
						0xF,
						FWP_ACTION_PERMIT);
					filter->AddCondition(
						FWPM_CONDITION_LOCAL_INTERFACE_TYPE,
						FWP_MATCH_EQUAL,
						FWP_UINT32,
						MIB_IF_TYPE_LOOPBACK);
					return filter;
				});

				DefineFilter(
					filters,
					[](const GUID* filterGuid) {
					auto filter = new WfpFilter(
						*filterGuid,
						MakeName(L"allow outbound DHCP fraffic filter"),
						FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
						0xF,
						FWP_ACTION_PERMIT);
					filter->AddCondition(
						FWPM_CONDITION_IP_PROTOCOL,
						FWP_MATCH_EQUAL,
						FWP_UINT8,
						IPPROTO_UDP);
					filter->AddCondition(
						FWPM_CONDITION_IP_LOCAL_PORT,
						FWP_MATCH_EQUAL,
						FWP_UINT16,
						DHCP_LOCAL_PORT);
					filter->AddCondition(
						FWPM_CONDITION_IP_REMOTE_PORT,
						FWP_MATCH_EQUAL,
						FWP_UINT16,
						DHCP_REMOTE_PORT);
					return filter;
				});
			});
	}

	FilterHandle FilterApiProvider::AddAllowTunnelFilter(const wstring& adapterNameOrId)
	{
		auto interfaceIndex = NetworkAdapterUtils::GetInterfaceIndexByAdapterNameOrGuid(adapterNameOrId);
		if (interfaceIndex < 0)
		{
			__SetLastLastFilterError(FILTER_API_ERROR_INVALID_ADAPTER_NAME_OR_ID);
			return 0;
		}

		return CreateFilterGroupByFunc([interfaceIndex](vector<FilterGuidPair>& filters)
		{
			DefineFilter(
				filters,
				[interfaceIndex](const GUID* filterGuid) {
				auto allowTunnelOutboundFilter = new WfpFilter(
					*filterGuid,
					MakeName(L"allow outbound tunnel filter"),
					FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
					0x0E,
					FWP_ACTION_PERMIT);
				allowTunnelOutboundFilter->AddCondition(
					FWPM_CONDITION_INTERFACE_INDEX,
					FWP_MATCH_EQUAL,
					FWP_UINT32,
					interfaceIndex);
				return allowTunnelOutboundFilter;
			});
		});
	}

	FilterHandle FilterApiProvider::AddBlockNonVpnDnsFilter(const wstring& adapterNameOrId)
	{
		auto interfaceIndex = NetworkAdapterUtils::GetInterfaceIndexByAdapterNameOrGuid(adapterNameOrId);
		if (interfaceIndex < 0)
		{
			__SetLastLastFilterError(FILTER_API_ERROR_INVALID_ADAPTER_NAME_OR_ID);
			return 0;
		}

		return CreateFilterGroupByFunc([interfaceIndex](vector<FilterGuidPair>& filters)
		{
			auto dnsServers = NetworkAdapterUtils::GetAllDnsServers();
			ULONG vpnDns;

			for (const auto& pair : dnsServers)
			{
				if (pair.first == interfaceIndex)
				{
					vpnDns = NetworkAdapterUtils::ConvertIpv4AddressToUlong(pair.second);
					break;
				}
			}

			for (const auto& pair : dnsServers)
			{
				auto dnsAddress = NetworkAdapterUtils::ConvertIpv4AddressToUlong(pair.second);

				if (dnsAddress == vpnDns)
				{
					continue;
				}
				
				DefineFilter(
					filters,
					[dnsAddress](const GUID* filterGuid) {
					auto filter = new WfpFilter(
						*filterGuid,
						MakeName(L"block ISP DNS UDP"),
						FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
						0x0F,
						FWP_ACTION_BLOCK);

					filter->AddMatchProtocolIpAndPortConditions(
						IPPROTO_UDP,
						dnsAddress,
						53);
					return filter;
				});

				DefineFilter(
					filters,
					[dnsAddress](const GUID* filterGuid) {
					auto filter = new WfpFilter(
						*filterGuid,
						MakeName(L"block ISP DNS TCP"),
						FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
						0x0F,
						FWP_ACTION_BLOCK);

					filter->AddMatchProtocolIpAndPortConditions(
						IPPROTO_TCP,
						dnsAddress,
						53);
					return filter;
				});
			}
		});
	}

	FilterHandle FilterApiProvider::AddAllowSpecificFilter(const string& ip, UINT8 protocol, int port)
	{
		ULONG address = NetworkAdapterUtils::ConvertIpv4AddressToUlong(ip);

		if (address == 0 &&
			protocol == 0 &&
			port == 0)
		{
			__SetLastLastFilterError(FILTER_API_ERROR_INVALID_ALLOW_SPECIFIC_FILTER);
			return 0;
		}

		auto allowSpecificOutboundFilterGuid = CreateFilterGuid();
		auto allowSpecificOutboundFilter = new WfpFilter(
			*allowSpecificOutboundFilterGuid,
			MakeName(L"allow specific outbound filter"),
			FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
			0xF,
			FWP_ACTION_PERMIT);
		allowSpecificOutboundFilter->AddMatchProtocolIpAndPortConditions(
			protocol,
			address,
			port);

		AddFilter(allowSpecificOutboundFilter, allowSpecificOutboundFilterGuid);

		return CreateFilterGroupByGuid(allowSpecificOutboundFilterGuid);
	}

#define DNS_RESOLVE_RETRY_TIMES (3)
#define DNS_RESOLVE_RETRY_DELAY_MS (1000)

	FilterHandle FilterApiProvider::AddAllowSpecificHostnameFilter(const string& hostname, UINT8 protocol, int port)
	{
		auto&& addresses = NetworkAdapterUtils::ConvertIpv4HostnameToUlongAddresses(hostname);
		for (int i = 0; i < DNS_RESOLVE_RETRY_TIMES-1; i++) {
			Sleep(DNS_RESOLVE_RETRY_DELAY_MS);
			addresses = NetworkAdapterUtils::ConvertIpv4HostnameToUlongAddresses(hostname);
			 if (!addresses.empty())
				 break;
		}

		if (addresses.empty())
		{
			__SetLastLastFilterError(FILTER_API_ERROR_NO_IP_FOUND_FOR_GIVEN_HOSTNAE);
			return 0;
		}

		return CreateFilterGroupByFunc([&addresses, protocol, port](vector<FilterGuidPair>& filters)
		{
			for (auto address : addresses)
			{
				auto allowSpecificHostnameOutboundFilterGuid = CreateFilterGuid();
				auto allowSpecificHostnameOutboundFilter = new WfpFilter(
					*allowSpecificHostnameOutboundFilterGuid,
					MakeName(L"allow specific hostname outbound filter"),
					FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
					0xF,
					FWP_ACTION_PERMIT);
				allowSpecificHostnameOutboundFilter->AddMatchProtocolIpAndPortConditions(
					protocol,
					address,
					port);
				filters.push_back(FilterGuidPair(
					allowSpecificHostnameOutboundFilter,
					allowSpecificHostnameOutboundFilterGuid));
			}
		});
	}

	FilterHandle FilterApiProvider::AddAllowSubnet(UINT32 ip, UINT32 subnetMask)
	{
		return CreateFilterGroupByFunc([ip, subnetMask](vector<FilterGuidPair>& filters)
		{
			DefineFilter(filters,
				[ip, subnetMask](const GUID* filterGuid)
			{
				auto filter = new WfpFilter(
					*filterGuid,
					MakeName(L"allow private network"),
					FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
					0x0F,
					FWP_ACTION_PERMIT);

				auto addrAndMask = new FWP_V4_ADDR_AND_MASK;
				addrAndMask->addr = ip;
				addrAndMask->mask = subnetMask;
				filter->AddCondition(
					FWPM_CONDITION_IP_REMOTE_ADDRESS,
					FWP_MATCH_EQUAL,
					FWP_V4_ADDR_MASK,
					reinterpret_cast<UINT64>(addrAndMask));
				return filter;
			});
		});
	}

	FilterHandle FilterApiProvider::AddAllowDnsFilter()
	{
		return CreateFilterGroupByFunc([](vector<FilterGuidPair>& filters)
		{
			DefineFilter(
				filters,
				[](const GUID* filterGuid) {
				auto filter = new WfpFilter(
					*filterGuid,
					MakeName(L"allow tcp dns filter"),
					FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
					0x0E,
					FWP_ACTION_PERMIT);
				filter->AddMatchProtocolIpAndPortConditions(
					IPPROTO_TCP,
					0,
					DNS_SERVER_PORT);
				return filter;
			});
			DefineFilter(
				filters,
				[](const GUID* filterGuid) {
				auto filter = new WfpFilter(
					*filterGuid,
					MakeName(L"allow udp dns filter"),
					FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
					0x0E,
					FWP_ACTION_PERMIT);
				filter->AddMatchProtocolIpAndPortConditions(
					IPPROTO_UDP,
					0,
					DNS_SERVER_PORT);
				return filter;
			});
		});
	}

	void FilterApiProvider::AddFilter(WfpFilter* filter, GUID* guid)
	{
		_wfpEngine.ApplyFilter(filter->BuildRawFilter());
		_filters[guid] = filter;
	}

	void FilterApiProvider::AddFilters(const vector<FilterGuidPair>& filters)
	{
		WfpEngine::FilterVector rawFilters;
		for (auto pair : filters)
		{
			rawFilters.push_back(&pair.first->BuildRawFilter());
		}

		_wfpEngine.ApplyFilters(rawFilters);

		for (auto pair : filters)
		{
			_filters[pair.second] = pair.first;
		}
	}

	FilterHandle FilterApiProvider::CreateFilterGroup(const vector<FilterGuidPair>& filters)
	{
		vector<const GUID*> filterGuids;
		for (auto& f : filters)
		{
			filterGuids.push_back(f.second);
		}
		return CreateFilterGroup(filterGuids);
	}

	FilterHandle FilterApiProvider::CreateFilterGroupByFilter(const FilterGuidPair& filter)
	{
		return CreateFilterGroup({ filter });
	}

	FilterHandle FilterApiProvider::CreateFilterGroupByFunc(function<void(vector<FilterGuidPair>&)> func)
	{
		vector<FilterGuidPair> filters;
		func(filters);
		AddFilters(filters);
		return CreateFilterGroup(filters);
	}

	FilterHandle FilterApiProvider::CreateFilterGroup(const vector<const GUID*>& filterGuids)
	{
		_filterGroupHandleIndex++;
		_filterGroups[_filterGroupHandleIndex] = filterGuids;
		return _filterGroupHandleIndex;
	}

	FilterHandle FilterApiProvider::CreateFilterGroupByGuid(const GUID* guid)
	{
		return CreateFilterGroup({guid});
	}

	GUID* FilterApiProvider::CreateFilterGuid()
	{
		GUID* guid = new GUID();
		UuidCreate(guid);
		return guid;
	}

	void FilterApiProvider::RemoveFilter(const GUID* filterGuid)
	{
		if (filterGuid == nullptr)
		{
			return;
		}
		if (_filters.count(filterGuid) == 1)
		{
			auto filter = _filters[filterGuid];
			_filters.erase(filterGuid);
			delete filterGuid;
			delete filter;
		}
	}

	void FilterApiProvider::RemoveFiltersByHandle(FilterHandle handle)
	{
		if (handle <= 0)
		{
			__SetLastLastFilterError(FILTER_API_ERROR_INVALID_FILTER_HANDLE);
			return;
		}
		if (_filterGroups.count(handle) == 0)
		{
			__SetLastLastFilterError(FILTER_API_ERROR_FILTER_NOT_FOUND);
			return;
		}

		auto& filterGuids = _filterGroups[handle];
		vector<FWPM_FILTER*> filtersToBeRemoved;
		for (auto guid : filterGuids)
		{
			auto filter = _filters[guid];
			filtersToBeRemoved.push_back(&filter->GetRawFilter());
		}

		_wfpEngine.RemoveFilters(filtersToBeRemoved, false);
		for (auto guid : filterGuids)
		{
			RemoveFilter(guid);
		}

		filterGuids.clear();
		_filterGroups.erase(handle);

		if (handle == _blockAllAndAllowLocalLoopbackFilterHandle)
		{
			_blockAllAndAllowLocalLoopbackFilterHandle = 0;
		}
	}

	void FilterApiProvider::RemoveAllFilters()
	{
		_wfpEngine.RemoveAllFilters(true);
		Cleanup();
	}

	void FilterApiProvider::Cleanup()
	{
		for (auto& pair : _filterGroups)
		{
			auto& guids = pair.second;
			for (auto guid : guids)
			{
				RemoveFilter(guid);
			}
			guids.clear();
		}

		_filterGroups.clear();
		_blockAllAndAllowLocalLoopbackFilterHandle = 0;
	}
}
