#include "stdafx.h"
#include "CppUnitTest.h"
#include "../WfpTool/NetworkAdapterUtils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

namespace wfptool
{
	TEST_CLASS(NetworkAdapterUtilsTests)
	{
	public:
		TEST_METHOD(TestGetInterfaceIndexByAdapterNameOrGuid)
		{
			NetworkAdapterUtils::EnumerateNetworkAdapters(
				[](const MIB_IF_ROW2& interfaceInfo)
			{
				int interfaceIndex = NetworkAdapterUtils::GetInterfaceIndexByAdapterNameOrGuid(interfaceInfo.Description);
				Assert::IsTrue(interfaceIndex > 0);
				interfaceIndex = NetworkAdapterUtils::GetInterfaceIndexByAdapterNameOrGuid(interfaceInfo.Alias);
				Assert::IsTrue(interfaceIndex > 0);

				wchar_t* interfaceGuidString, *networkGuidString;
				UuidToString(&interfaceInfo.InterfaceGuid, &interfaceGuidString);
				UuidToString(&interfaceInfo.NetworkGuid, &networkGuidString);

				interfaceIndex = NetworkAdapterUtils::
					GetInterfaceIndexByAdapterNameOrGuid(interfaceGuidString);
				Assert::IsTrue(interfaceIndex > 0);
				interfaceIndex = NetworkAdapterUtils::
					GetInterfaceIndexByAdapterNameOrGuid(networkGuidString);
				Assert::IsTrue(interfaceIndex > 0);

				auto curlyBracedGuidString = wstring(L"{") + wstring(interfaceGuidString) + L"}";
				interfaceIndex = NetworkAdapterUtils::
					GetInterfaceIndexByAdapterNameOrGuid(curlyBracedGuidString);
				Assert::IsTrue(interfaceIndex > 0);

				return true;
			});
		}

		TEST_METHOD(TestGetAllDnsServers)
		{
			auto&& dnsServers = NetworkAdapterUtils::GetAllDnsServers();
			Assert::IsFalse(dnsServers.empty());
		}

		TEST_METHOD(TestConvertIpv4AddressToUlong)
		{
			string ipAddress = "64.233.187.99";
			ULONG expectedUlongIp = 1089059683;
			auto actualUlongIp = NetworkAdapterUtils::ConvertIpv4AddressToUlong(ipAddress);
			Assert::AreEqual(expectedUlongIp, actualUlongIp);
		}

		TEST_METHOD(TestConvertHostnameToIpv4Addresses)
		{
			string hostname = "www.bing.com";
			auto&& addresses = NetworkAdapterUtils::ConvertIpv4HostnameToUlongAddresses(hostname);
			Assert::IsFalse(addresses.empty());
		}
	};
}