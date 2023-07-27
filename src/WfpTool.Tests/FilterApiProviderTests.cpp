#include "stdafx.h"
#include "CppUnitTest.h"
#include "netioapi.h"
#include "../WfpTool/FilterApiProvider.h"
#include "../WfpTool/FilterApiErrors.h"
#include "../WfpTool/NetworkAdapterUtils.h"
#include <list>
#include <algorithm>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

namespace wfptool
{
	static DWORD WINAPI FwpmFilterEnum_EnumMultiple(
		_In_ HANDLE engineHandle,
		_In_ HANDLE enumHandle,
		_In_ UINT32 numEntriesRequested,
		_Outptr_result_buffer_(*numEntriesReturned) FWPM_FILTER0*** entries,
		_Out_ UINT32* numEntriesReturned
		);

	static struct
	{
		list<int>  Calls;
		vector<FWPM_FILTER*> Filters;
		unsigned int LastPosition;

		void Init()
		{
			Filters.clear();
			Calls.clear();
			LastPosition = 0;
			SetMockFwpmFilterEnum(FwpmFilterEnum_EnumMultiple);
		}

		static void Reset()
		{
			SetMockFwpmFilterEnum(nullptr);
		}
	} FwpmFilterEnum_EnumMultiple_Ctx;


	static DWORD WINAPI FwpmFilterEnum_EnumMultiple(
		_In_ HANDLE engineHandle,
		_In_ HANDLE enumHandle,
		_In_ UINT32 numEntriesRequested,
		_Outptr_result_buffer_(*numEntriesReturned) FWPM_FILTER0*** entries,
		_Out_ UINT32* numEntriesReturned
		)
	{

		*numEntriesReturned = (FwpmFilterEnum_EnumMultiple_Ctx.Filters.size() - FwpmFilterEnum_EnumMultiple_Ctx.LastPosition);
		if (*numEntriesReturned > numEntriesRequested)
			*numEntriesReturned = numEntriesRequested;

		;
		*entries = FwpmFilterEnum_EnumMultiple_Ctx.Filters.data() + FwpmFilterEnum_EnumMultiple_Ctx.LastPosition;
		FwpmFilterEnum_EnumMultiple_Ctx.LastPosition += *numEntriesReturned;

		SetMockFilters(*entries, *numEntriesReturned);

		return NO_ERROR;
	}

	static ULONG GetDnsAddressFromFilter(const FWPM_FILTER*& filter)
	{
		return filter->filterCondition[1].conditionValue.uint32;
	}

	TEST_CLASS(FilterApiProviderTests)
	{
	public:
		TEST_METHOD_INITIALIZE(BeforeEachTestRun)
		{
			RestoreDefaultMockingBehavior();
		}

		TEST_METHOD(TestFilterApiProvider_AddBlockAllFilter)
		{
			vector<const FWPM_FILTER*> filterVector;
			int callCount = 0;
			SetAddFilterCallback([&filterVector, &callCount](HANDLE engineHandle,
				const FWPM_FILTER* filter,
				PSECURITY_DESCRIPTOR sd,
				UINT64* id)
			{
				callCount++;
				filterVector.push_back(filter);
				return NO_ERROR;
			});

			FilterApiProvider provider;
			auto handle = provider.AddBlockAllFilter();

			// There is at least 3 filters,
			// one is for blocking all outbound ipv4
			// one is for blocking all outbound ipv6
			// one is for allow local loopback to allow communication between engine and UI
			Assert::IsTrue(handle > 0);
			Assert::AreEqual(5, callCount, L"callCount");
			size_t filterVectorSize = filterVector.size();
			Assert::AreEqual((size_t)5, filterVectorSize, L"filterVector.size()");

			const FWPM_FILTER* blockAllIpv4Filter = nullptr,
				*blockAllIpv6Filter = nullptr,
				*allowLocalLoopbackFilter = nullptr,
				*allowLocalLoopbackFilterv6 = nullptr,
				*allowDhcpFilter = nullptr;
			for(auto& filter: filterVector)
			{
				if (filter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 &&
					filter->action.type == FWP_ACTION_BLOCK)
				{
					blockAllIpv4Filter = filter;
					Assert::IsTrue(filter->numFilterConditions == 0);
				}
				else if (filter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6 &&
					filter->action.type == FWP_ACTION_BLOCK)
				{
					Assert::IsTrue(filter->numFilterConditions == 0);
					blockAllIpv6Filter = filter;
				}

				else if (filter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 &&
					filter->action.type == FWP_ACTION_PERMIT &&
					filter->numFilterConditions == 1 &&
					filter->filterCondition[0].fieldKey == FWPM_CONDITION_LOCAL_INTERFACE_TYPE &&
					filter->filterCondition[0].matchType == FWP_MATCH_EQUAL &&
					filter->filterCondition[0].conditionValue.uint32 == MIB_IF_TYPE_LOOPBACK)
				{
					allowLocalLoopbackFilter = filter;
				}
				else if (filter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6 &&
					filter->action.type == FWP_ACTION_PERMIT &&
					filter->numFilterConditions == 1 &&
					filter->filterCondition[0].fieldKey == FWPM_CONDITION_LOCAL_INTERFACE_TYPE &&
					filter->filterCondition[0].matchType == FWP_MATCH_EQUAL &&
					filter->filterCondition[0].conditionValue.uint32 == MIB_IF_TYPE_LOOPBACK)
				{
					allowLocalLoopbackFilterv6 = filter;
				}
				else {
					if (filter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 &&
						filter->action.type == FWP_ACTION_PERMIT &&
						filter->numFilterConditions == 3 &&

						filter->filterCondition[0].fieldKey == FWPM_CONDITION_IP_PROTOCOL &&
						filter->filterCondition[0].matchType == FWP_MATCH_EQUAL &&
						filter->filterCondition[0].conditionValue.uint8 == IPPROTO_UDP &&

						filter->filterCondition[1].fieldKey == FWPM_CONDITION_IP_LOCAL_PORT	&&
						filter->filterCondition[1].matchType == FWP_MATCH_EQUAL &&
						filter->filterCondition[1].conditionValue.uint8 == FilterApiProvider::DHCP_LOCAL_PORT &&

						filter->filterCondition[2].fieldKey == FWPM_CONDITION_IP_REMOTE_PORT &&
						filter->filterCondition[2].matchType == FWP_MATCH_EQUAL &&
						filter->filterCondition[2].conditionValue.uint16 == FilterApiProvider::DHCP_REMOTE_PORT
						)
					{
						allowDhcpFilter = filter;
					}
				}
			}

			Assert::IsNotNull(blockAllIpv4Filter);
			Assert::IsNotNull(blockAllIpv6Filter);
			Assert::IsNotNull(allowLocalLoopbackFilter);
			Assert::IsNotNull(allowLocalLoopbackFilterv6);
			Assert::IsNotNull(allowDhcpFilter);

			// Repeated call of AddAllowBlockAll filter should give error
			handle = provider.AddBlockAllFilter();

			Assert::IsTrue(handle == 0);
			Assert::IsTrue(GetLastFilterApiError() == FILTER_API_ERROR_FILTER_ALREADY_ENABLED);
		}

		TEST_METHOD(TestFilterApiProvider_AddAllowDnsFilter)
		{
			vector<const FWPM_FILTER*> filterVector;
			int callCount = 0;
			SetAddFilterCallback([&filterVector, &callCount](HANDLE handle,
				const FWPM_FILTER* filter,
				PSECURITY_DESCRIPTOR sd,
				UINT64* id)
			{
				callCount++;
				filterVector.push_back(filter);
				return NO_ERROR;
			});

			auto&& dnsServers = NetworkAdapterUtils::GetAllDnsServers();
			FilterApiProvider provider;
			auto handle = provider.AddAllowDnsFilter();
			if (dnsServers.empty())
			{
				Assert::IsTrue(handle == 0);
				Assert::IsTrue(GetLastFilterApiError() == FILTER_API_ERROR_NO_DNS_SERVERS_FOUND);
				Assert::AreEqual(callCount, 0);
				Assert::IsTrue(filterVector.empty());
			}
			else
			{
				Assert::IsTrue(handle > 0);
				// Since we create two filters for each dns servers,
				// so the expected call count is double the count of
				// DNS servers
				int expectedCallCount = 2;
				Assert::IsTrue(callCount == expectedCallCount);
				Assert::IsTrue(filterVector.size() == expectedCallCount);
			}
		}

		TEST_METHOD(TestFilterApiProvider_AddBlockNonVpnDnsFilter)
		{
			vector<const FWPM_FILTER*> filterVector;
			int callCount = 0;
			SetAddFilterCallback([&filterVector, &callCount](HANDLE handle,
				const FWPM_FILTER* filter,
				PSECURITY_DESCRIPTOR sd,
				UINT64* id)
			{
				callCount++;
				filterVector.push_back(filter);
				return NO_ERROR;
			});
			auto&& dnsServers = NetworkAdapterUtils::GetAllDnsServers();
			FilterApiProvider provider;
			NetworkAdapterUtils::EnumerateNetworkAdapters(
				[&provider, &dnsServers, &filterVector, &callCount](const MIB_IF_ROW2& interfaceInfo)
				{
					callCount = 0;
					filterVector.clear();
					auto& interfaceAlias = interfaceInfo.Alias;
					auto interfaceIndex = NetworkAdapterUtils::
						GetInterfaceIndexByAdapterNameOrGuid(interfaceAlias);
					auto handle = provider.AddBlockNonVpnDnsFilter(interfaceAlias);
					Assert::IsTrue(handle > 0);
					int nonVpnDnsServerCount = 0;
					for (const auto& pair: dnsServers)
					{
						auto dnsAddress = NetworkAdapterUtils::ConvertIpv4AddressToUlong(pair.second);
						if (interfaceIndex != pair.first)
						{
							nonVpnDnsServerCount++;
							auto match = find_if(filterVector.begin(), filterVector.end(), [dnsAddress](const FWPM_FILTER* filter)
							{
								return GetDnsAddressFromFilter(filter) == dnsAddress;
							});
							Assert::IsTrue(match != filterVector.end());
						}
						else
						{
							auto match = find_if(filterVector.begin(), filterVector.end(), [dnsAddress](const FWPM_FILTER* filter)
							{
								return GetDnsAddressFromFilter(filter) == dnsAddress;
							});
							Assert::IsTrue(match == filterVector.end());
						}
						Assert::IsTrue(handle > 0);
					}
					// Since we create two filters for each dns servers,
					// so the expected call count is double the count of
					// DNS servers
					int expectedCallCount = nonVpnDnsServerCount * 2;
					Assert::IsTrue(callCount == expectedCallCount);
					Assert::IsTrue(filterVector.size() == expectedCallCount);
					return true;
				});

			wstring invalidAdapterName = L"Invalid adapter";
			auto handle = provider.AddBlockNonVpnDnsFilter(invalidAdapterName);
			Assert::IsTrue(handle == 0);
			Assert::IsTrue(GetLastFilterApiError() == FILTER_API_ERROR_INVALID_ADAPTER_NAME_OR_ID);
		}

		TEST_METHOD(TestFilterApiProvider_AddAllowTunnelFilter)
		{
			FilterApiProvider provider;
			NetworkAdapterUtils::EnumerateNetworkAdapters([&provider](const MIB_IF_ROW2& interfaceInfo)
			{
				auto& interfaceAlias = interfaceInfo.Alias;
				auto handle = provider.AddAllowTunnelFilter(interfaceAlias);
				Assert::IsTrue(handle > 0);
				return true;
			});

			wstring invalidAdapterName = L"Invalid adapter";
			auto handle = provider.AddAllowTunnelFilter(invalidAdapterName);
			Assert::IsTrue(handle == 0);
			Assert::IsTrue(GetLastFilterApiError() == FILTER_API_ERROR_INVALID_ADAPTER_NAME_OR_ID);
		}

		TEST_METHOD(TestFilterApiProvider_AddAllowSpecificFilter)
		{
			vector<const FWPM_FILTER*> filterVector;
			int callCount = 0;
			SetAddFilterCallback([&filterVector, &callCount](HANDLE handle,
				const FWPM_FILTER* filter,
				PSECURITY_DESCRIPTOR sd,
				UINT64* id)
			{
				callCount++;
				filterVector.push_back(filter);
				return NO_ERROR;
			});

			FilterApiProvider provider;
			auto handle = provider.AddAllowSpecificFilter("", 0, 0);
			Assert::IsTrue(handle == 0);
			Assert::IsTrue(GetLastFilterApiError() == FILTER_API_ERROR_INVALID_ALLOW_SPECIFIC_FILTER);
			Assert::AreEqual(callCount, 0);
			Assert::IsTrue(filterVector.size() == 0);

			string ip = "192.168.2.1";
			handle = provider.AddAllowSpecificFilter(ip, 0, 0);
			Assert::IsTrue(handle > 0);
			Assert::AreEqual(callCount, 1);
			Assert::IsTrue(filterVector.size() == 1);
			Assert::IsTrue(filterVector[0]->numFilterConditions == 1);

			filterVector.clear();
			handle = provider.AddAllowSpecificFilter(ip, IPPROTO_TCP, 0);
			Assert::IsTrue(handle > 0);
			Assert::AreEqual(callCount, 2);
			Assert::IsTrue(filterVector.size() == 1);
			Assert::IsTrue(filterVector[0]->numFilterConditions == 2);

			filterVector.clear();
			handle = provider.AddAllowSpecificFilter(ip, IPPROTO_TCP, 443);
			Assert::IsTrue(handle > 0);
			Assert::AreEqual(callCount, 3);
			Assert::IsTrue(filterVector.size() == 1);
			Assert::IsTrue(filterVector[0]->numFilterConditions == 3);
		}

		TEST_METHOD(TestFilterApiProvider_AddAllowSpecificHostnameFilter)
		{
			vector<const FWPM_FILTER*> filterVector;
			int callCount = 0;
			SetAddFilterCallback([&filterVector, &callCount](HANDLE handle,
				const FWPM_FILTER* filter,
				PSECURITY_DESCRIPTOR sd,
				UINT64* id)
			{
				callCount++;
				filterVector.push_back(filter);
				return NO_ERROR;
			});

			FilterApiProvider provider;
			auto handle = provider.AddAllowSpecificHostnameFilter("", 0, 0);
			Assert::IsTrue(handle == 0);
			Assert::IsTrue(GetLastFilterApiError() == FILTER_API_ERROR_NO_IP_FOUND_FOR_GIVEN_HOSTNAE);
			Assert::AreEqual(callCount, 0);
			Assert::IsTrue(filterVector.size() == 0);

			string hostName = "www.bing.com";
			handle = provider.AddAllowSpecificFilter(hostName, 0, 0);
			auto&& ips = NetworkAdapterUtils::ConvertIpv4HostnameToUlongAddresses(hostName);
			if (ips.empty())
			{
				Assert::IsTrue(handle == 0);
				Assert::IsTrue(GetLastFilterApiError() == FILTER_API_ERROR_NO_IP_FOUND_FOR_GIVEN_HOSTNAE);
				Assert::AreEqual(callCount, 0);
				Assert::IsTrue(filterVector.size() == 0);
			}
			else
			{
				Assert::IsTrue(handle > 0);
				Assert::IsTrue(callCount == 1);
				Assert::IsTrue(filterVector.size() == 1);
				for (auto filter : filterVector)
				{
					Assert::IsTrue(filter->numFilterConditions == 1);
				}

				callCount = 0;
				filterVector.clear();
				handle = provider.AddAllowSpecificFilter(hostName, IPPROTO_TCP, 0);
				Assert::IsTrue(handle > 0);
				Assert::IsTrue(callCount == 1);
				Assert::IsTrue(filterVector.size() == 1);
				for(auto filter: filterVector)
				{
					Assert::IsTrue(filter->numFilterConditions == 2);
				}

				callCount = 0;
				filterVector.clear();
				handle = provider.AddAllowSpecificFilter(hostName, IPPROTO_TCP, 443);
				Assert::IsTrue(handle > 0);
				Assert::IsTrue(callCount == 1);
				Assert::IsTrue(filterVector.size() == 1);
				for (auto filter : filterVector)
				{
					Assert::IsTrue(filter->numFilterConditions == 3);
				}
			}
		}

		TEST_METHOD(TestFilterApiProvider_AddAllowSubnetFilter)
		{
			vector<const FWPM_FILTER*> filterVector;
			int callCount = 0;
			SetAddFilterCallback([&filterVector, &callCount](HANDLE handle,
				const FWPM_FILTER* filter,
				PSECURITY_DESCRIPTOR sd,
				UINT64* id)
			{
				callCount++;
				filterVector.push_back(filter);
				return NO_ERROR;
			});

			FilterApiProvider provider;
			auto handle = provider.AddAllowSubnet(167772160, 4278190080);
			Assert::IsTrue(handle > 0);
			Assert::AreEqual(callCount, 1);
			Assert::IsTrue(filterVector.size() > 0);
		}

		TEST_METHOD(TestRemoveFiltersByHandle)
		{
			FilterApiProvider provider;
			provider.RemoveFiltersByHandle(0);
			Assert::IsTrue(GetLastFilterApiError() ==
				FILTER_API_ERROR_INVALID_FILTER_HANDLE);

			provider.RemoveFiltersByHandle(1);
			Assert::IsTrue(GetLastFilterApiError() ==
				FILTER_API_ERROR_FILTER_NOT_FOUND);

			auto handle = provider.AddBlockAllFilter();
			auto filterSize = provider._filters.size();
			Assert::IsTrue(filterSize > 0);

			int callCount = 0;
			SetDeleteFilterByKeyCallback([&callCount](HANDLE engineHandle, const GUID* filterKey)
			{
				callCount++;
				return NO_ERROR;
			});
			provider.RemoveFiltersByHandle(handle);

			Assert::IsTrue(callCount == filterSize);
			Assert::IsTrue(provider._filters.size() == 0);
		}

		TEST_METHOD(TestFilterApiProvider_RemoveAllFilters)
		{
			int callCount = 0;
			SetDeleteFilterByKeyCallback([&callCount](HANDLE engineHandle, const GUID* filterKey)
			{
				callCount++;
				return NO_ERROR;
			});

			FilterApiProvider provider;
			provider.AddBlockAllFilter();
			provider.AddAllowSpecificFilter("192.168.3.5", IPPROTO_TCP, 443);
			auto filterSize = provider._filters.size();

			vector<FWPM_FILTER*> filters;
			for(auto& pair: provider._filters)
			{
				filters.push_back(&pair.second->GetRawFilter());
			}
			SetMockFilters(filters.data(), filters.size());
			provider.RemoveAllFilters();
			Assert::IsTrue(callCount == filterSize);
			Assert::IsTrue(provider._filters.size() == 0);

			SetMockFilters(nullptr, 0);
		}

		TEST_METHOD(TestFilterApiProvider_RemoveAllFilters_MultipleEnums)
		{
			try
			{
				int callCount = 0;
				SetDeleteFilterByKeyCallback([&callCount](HANDLE engineHandle, const GUID* filterKey)
				{
					callCount++;
					return NO_ERROR;
				});

				FilterApiProvider provider;
				provider.AddBlockAllFilter();
				provider.AddAllowSpecificFilter("192.168.3.5", IPPROTO_TCP, 443);

				FwpmFilterEnum_EnumMultiple_Ctx.Init();

				for (auto& pair : provider._filters)
				{
					FwpmFilterEnum_EnumMultiple_Ctx.Filters.push_back(&pair.second->GetRawFilter());
					// also add 150 ones with different GUID
					for (int i = 0; i < 150; i++)
					{
						FWPM_FILTER0 * copy = new FWPM_FILTER0();
						GUID* guid = new GUID();
						*guid = *pair.second->GetRawFilter().providerKey;
						guid->Data1++;
						*copy = pair.second->GetRawFilter();
						copy->providerKey = guid;
						FwpmFilterEnum_EnumMultiple_Ctx.Filters.push_back(copy);

					}
				}
				provider.RemoveAllFilters();
				Assert::AreEqual(FwpmFilterEnum_EnumMultiple_Ctx.Filters.size(), FwpmFilterEnum_EnumMultiple_Ctx.LastPosition);
				Assert::IsTrue(provider._filters.size() == 0);

				SetMockFilters(nullptr, 0);
			}
			catch (exception e)
			{
				FwpmFilterEnum_EnumMultiple_Ctx.Reset();
				throw;
			}
			FwpmFilterEnum_EnumMultiple_Ctx.Reset();
		}
	};
}