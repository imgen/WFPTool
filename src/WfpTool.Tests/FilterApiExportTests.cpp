#include "stdafx.h"
#include "CppUnitTest.h"
#include "netioapi.h"
#include "../WfpTool/FilterApiExport.h"
#include "../WfpTool/FilterApiErrors.h"
#include "../WfpTool/NetworkAdapterUtils.h"
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

namespace wfptool
{
	TEST_CLASS(FilterApiExportTests)
	{
	public:
		TEST_METHOD_INITIALIZE(BeforeEachTestRun)
		{
			RestoreDefaultMockingBehavior();
			RemoveAllFilters2();
		}

		TEST_METHOD(TestFilterApiExport_AddBlockAllFilter)
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

			FilterHandle handle;
			AddBlockAllFilter2(&handle);

			// There is at least 3 filters,
			// one is for blocking all outbound ipv4
			// one is for blocking all outbound ipv6
			// one is for allow local loopback to allow communication between engine and UI
			Assert::IsTrue(callCount >= 3);
			Assert::IsTrue(filterVector.size() >= 3);

			const FWPM_FILTER* blockAllIpv4Filter = nullptr,
				* blockAllIpv6Filter = nullptr,
				* allowLocalLoopbackFilter = nullptr;
			for (auto& filter : filterVector)
			{
				if (filter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 &&
					filter->action.type == FWP_ACTION_BLOCK)
				{
					blockAllIpv4Filter = filter;
				}
				else if (filter->layerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6 &&
					filter->action.type == FWP_ACTION_BLOCK)
				{
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
			}

			Assert::IsNotNull(blockAllIpv4Filter);
			Assert::IsNotNull(blockAllIpv6Filter);
			Assert::IsNotNull(allowLocalLoopbackFilter);

			// Repeated call of AddAllowBlockAll filter should give error
			auto errorCode = AddBlockAllFilter2(&handle);
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			Assert::IsTrue(handle == 0);
			Assert::IsTrue(errorCode == FILTER_API_ERROR_FILTER_ALREADY_ENABLED);
		}

		TEST_METHOD(TestFilterApiExport_AddAllowDnsFilter)
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
			FilterHandle handle;
			auto errorCode = AddAllowDnsFilter2(&handle);
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			if (dnsServers.empty())
			{
				Assert::IsTrue(handle == 0);
				Assert::IsTrue(errorCode == FILTER_API_ERROR_NO_DNS_SERVERS_FOUND);
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
				Assert::AreEqual(expectedCallCount, callCount);
				Assert::AreEqual(expectedCallCount, static_cast<int>(filterVector.size()));
			}
		}

		TEST_METHOD(TestFilterApiExport_AddAllowTunnelFilter)
		{
			FilterHandle handle;
			NetworkAdapterUtils::EnumerateNetworkAdapters([&handle](const MIB_IF_ROW2& interfaceInfo)
				{
					auto interfaceAlias = interfaceInfo.Alias;
					auto errorCode = AddAllowTunnelFilter2(&handle, interfaceAlias);
					Assert::AreEqual(errorCode, GetLastFilterApiError());
					Assert::IsTrue(handle > 0);
					Assert::IsTrue(errorCode == NO_ERROR);
					return true;
				});

			auto invalidAdapterName = L"Invalid adapter";
			auto errorCode = AddAllowTunnelFilter2(&handle, invalidAdapterName);
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			Assert::IsTrue(handle == 0);
			Assert::IsTrue(errorCode == FILTER_API_ERROR_INVALID_ADAPTER_NAME_OR_ID);
		}

		TEST_METHOD(TestFilterApiExport_AddAllowSpecificFilter)
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

			FilterHandle handle;
			auto errorCode = AddAllowSpecificFilter2(&handle, "", 0, 0);
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			Assert::IsTrue(handle == 0);
			Assert::IsTrue(errorCode == FILTER_API_ERROR_INVALID_ALLOW_SPECIFIC_FILTER);
			Assert::AreEqual(callCount, 0);
			Assert::IsTrue(filterVector.size() == 0);

			char* ip = "192.168.2.1";
			errorCode = AddAllowSpecificFilter2(&handle, ip, 0, 0);
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			Assert::IsTrue(handle > 0);
			Assert::AreEqual(callCount, 1);
			Assert::IsTrue(filterVector.size() == 1);
			Assert::IsTrue(filterVector[0]->numFilterConditions == 1);

			filterVector.clear();
			errorCode = AddAllowSpecificFilter2(&handle, ip, IPPROTO_TCP, 0);
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			Assert::IsTrue(handle > 0);
			Assert::AreEqual(callCount, 2);
			Assert::IsTrue(filterVector.size() == 1);
			Assert::IsTrue(filterVector[0]->numFilterConditions == 2);

			filterVector.clear();
			errorCode = AddAllowSpecificFilter2(&handle, ip, IPPROTO_TCP, 443);
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			Assert::IsTrue(handle > 0);
			Assert::AreEqual(callCount, 3);
			Assert::IsTrue(filterVector.size() == 1);
			Assert::IsTrue(filterVector[0]->numFilterConditions == 3);
		}

		TEST_METHOD(TestFilterApiExport_AddAllowSpecificHostnameFilter)
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

			FilterHandle handle;
			auto errorCode = AddAllowSpecificHostnameFilter2(&handle, "", 0, 0);
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			Assert::IsTrue(handle == 0);
			Assert::IsTrue(errorCode == FILTER_API_ERROR_NO_IP_FOUND_FOR_GIVEN_HOSTNAE);
			Assert::AreEqual(callCount, 0);
			Assert::IsTrue(filterVector.size() == 0);

			char* hostName = "www.bing.com";
			errorCode = AddAllowSpecificHostnameFilter2(&handle, hostName, 0, 0);
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			auto&& ips = NetworkAdapterUtils::ConvertIpv4HostnameToUlongAddresses(hostName);
			if (ips.empty())
			{
				Assert::IsTrue(handle == 0);
				Assert::IsTrue(errorCode == FILTER_API_ERROR_NO_IP_FOUND_FOR_GIVEN_HOSTNAE);
				Assert::AreEqual(callCount, 0);
				Assert::IsTrue(filterVector.size() == 0);
			}
			else
			{
				Assert::IsTrue(handle > 0);
				Assert::IsTrue(callCount == 2);
				Assert::IsTrue(filterVector.size() == 2);
				for (auto filter : filterVector)
				{
					Assert::IsTrue(filter->numFilterConditions == 1);
				}

				callCount = 0;
				filterVector.clear();
				errorCode = AddAllowSpecificFilter2(&handle, hostName, IPPROTO_TCP, 0);
				Assert::AreEqual(errorCode, GetLastFilterApiError());
				Assert::IsTrue(handle > 0);
				Assert::IsTrue(callCount == 1);
				Assert::IsTrue(filterVector.size() == 1);
				for (auto filter : filterVector)
				{
					Assert::IsTrue(filter->numFilterConditions == 2);
				}

				callCount = 0;
				filterVector.clear();
				errorCode = AddAllowSpecificFilter2(&handle, hostName, IPPROTO_TCP, 443);
				Assert::AreEqual(errorCode, GetLastFilterApiError());
				Assert::IsTrue(handle > 0);
				Assert::IsTrue(callCount == 1);
				Assert::IsTrue(filterVector.size() == 1);
				for (auto filter : filterVector)
				{
					Assert::IsTrue(filter->numFilterConditions == 3);
				}
			}
		}

		TEST_METHOD(TestFilterApiExport_RemoveFilter)
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

			auto errorCode = RemoveFilter2(0);
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			Assert::IsTrue(errorCode ==
				FILTER_API_ERROR_INVALID_FILTER_HANDLE);

			errorCode = RemoveFilter2(1);
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			Assert::IsTrue(errorCode == FILTER_API_ERROR_FILTER_NOT_FOUND);

			FilterHandle handle;
			AddBlockAllFilter2(&handle);
			auto filterSize = filterVector.size();
			Assert::IsTrue(filterSize > 0);

			callCount = 0;
			SetDeleteFilterByKeyCallback([&callCount, &filterVector](HANDLE engineHandle, const GUID* filterKey)
				{
					callCount++;
					for (int i = 0; i < filterVector.size(); i++)
					{
						if (filterVector[i]->filterKey == *filterKey)
						{
							filterVector.erase(filterVector.begin() + i);
						}
					}
					return NO_ERROR;
				});
			errorCode = RemoveFilter2(handle);
			Assert::AreEqual(errorCode, GetLastFilterApiError());

			Assert::IsTrue(callCount == filterSize);
			Assert::IsTrue(filterVector.size() == 0);
		}

		TEST_METHOD(TestFilterApiExport_RemoveAllFilters)
		{
			vector<FWPM_FILTER*> filterVector;
			int callCount = 0;
			SetAddFilterCallback([&filterVector, &callCount](HANDLE engineHandle,
				const FWPM_FILTER* filter,
				PSECURITY_DESCRIPTOR sd,
				UINT64* id)
				{
					callCount++;
					filterVector.push_back(const_cast<FWPM_FILTER*>(filter));
					return NO_ERROR;
				});

			FilterHandle handle;
			AddBlockAllFilter2(&handle);
			AddAllowSpecificFilter2(&handle, "192.168.3.5", IPPROTO_TCP, 443);
			auto filterSize = filterVector.size();

			// here we need to make a copy so that it won't be affected by the changes
			// of the original vector
			vector<FWPM_FILTER*> filterVector2 = filterVector;
			SetMockFilters(filterVector2.data(), filterVector.size());
			callCount = 0;
			SetDeleteFilterByKeyCallback([&callCount, &filterVector](HANDLE engineHandle, const GUID* filterKey)
				{
					callCount++;
					for (int i = 0; i < filterVector.size(); i++)
					{
						if (filterVector[i]->filterKey == *filterKey)
						{
							filterVector.erase(filterVector.begin() + i);
							return NO_ERROR;
						}
					}
					return NO_ERROR;
				});
			auto errorCode = RemoveAllFilters2();
			Assert::AreEqual(errorCode, GetLastFilterApiError());
			Assert::IsTrue(callCount == filterSize);
			Assert::IsTrue(filterVector.size() == 0);

			SetMockFilters(nullptr, 0);
		}
	};
}