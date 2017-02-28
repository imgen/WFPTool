#include "stdafx.h"
#include "WfpApp.h"
#include <iostream>
#include <string>
#include "NetworkAdapterUtils.h"
#include "FilterApiExport.h"
#include "FilterApiErrors.h"

using namespace std;
using namespace wfptool;

#pragma comment(lib,"rpcrt4.lib")
#pragma comment(lib,"fwpuclnt.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"IPHlpApi.lib")

static const wstring DEFAULT_VPN_ADAPTER_NAME = L"TAP-Windows Adapter V9";

static void TestFilterApi();

int main(int argc, char *argv[])
{
	TestFilterApi();
	return 0;
	cout << "Got " << argc << " arguments" << endl;
	for (int i = 0; i < argc; i++)
	{
		cout << i << " -> " << argv[i] << endl;
	}

	char* ipAddr = argv[1];
	int port = WfpApp::DEFAULT_ALLOW_PORT;

	if (argc > 2)
	{
		port = stoi(argv[2]);
	}
	int interfaceIndex;
	if (argc > 3)
	{
		interfaceIndex = stoi(argv[3]);
	}
	else
	{
		interfaceIndex = NetworkAdapterUtils::GetInterfaceIndexByAdapterNameOrGuid(DEFAULT_VPN_ADAPTER_NAME);

		if (interfaceIndex > 0)
		{
			wcout << "Found the interface index for adapter with name " << DEFAULT_VPN_ADAPTER_NAME
				<< " to be " << interfaceIndex << endl;
		}
		auto&& allDnsServers = NetworkAdapterUtils::GetAllDnsServers();
		cout << "Found " << allDnsServers.size() << " DNS servers: " << endl;
		for (auto& dnsServer: allDnsServers)
		{
			cout << "Interface Index: " << dnsServer.first << ", DNS Address: " << dnsServer.second << endl;
		}

		cout << endl;

		ULONG binaryAddress = NetworkAdapterUtils::ConvertIpv4AddressToUlong("192.168.2.2");
		cout << "Binary address for 192.168.2.2 is " << binaryAddress << endl;
	}

	WfpApp app(ipAddr, port, interfaceIndex);
	cout << "Press any key to apply filters" << endl;
	system("pause");
	try
	{
		app.ApplyFilters();
	}
	catch (exception ex)
	{
		app.RemoveFilters();
		app.ApplyFilters();
	}
	cout << "Now press any key to remove filters and exit" << endl;
	system("pause");
	app.RemoveFilters();

	return 0;
}

void TestFilterApi()
{
	NetworkAdapterUtils::GetInterfaceIndexByAdapterNameOrGuid(L"{A5F29032-4459-4526-BD85-B2D94CA7DF7D}");

	auto&& addresses = NetworkAdapterUtils::ConvertIpv4HostnameToUlongAddresses("www.google.com");
	if (addresses.size() == 0)
	{
		cout << "Oops, something went way off because google.com is not resolvable!" << endl;
	}

	RemoveAllFilters();
	auto filterHandle = AddBlockAllFilter();
	auto error = GetLastFilterApiError();
	if (error != NO_ERROR)
	{
		cout << "Oops, you are not supposed to get error code " << error << endl;
	}
	auto blockAllFilterHandle = filterHandle;
	if (filterHandle == 0)
	{
		cout << "Oops, handle should be larger than zero, don't you think?" << endl;
	}
	filterHandle = AddBlockAllFilter();
	if (filterHandle > 0)
	{
		cout << "Oops, something has gone wrong. The handle should be 0" << endl;
	}
	error = GetLastFilterApiError();
	if (error != FILTER_API_ERROR_FILTER_ALREADY_ENABLED)
	{
		cout << "Oops, you are supposed to get error code " << FILTER_API_ERROR_FILTER_ALREADY_ENABLED << endl;
	}
	RemoveFilter(filterHandle);
	error = GetLastFilterApiError();
	if (error != FILTER_API_ERROR_INVALID_FILTER_HANDLE)
	{
		cout << "Oops, you are supposed to get error code " << FILTER_API_ERROR_INVALID_FILTER_HANDLE << endl;
	}

	RemoveFilter(blockAllFilterHandle);
	error = GetLastFilterApiError();
	if (error != NO_ERROR)
	{
		cout << "Oops, you are not supposed to get error code " << error << endl;
	}

	filterHandle = AddAllowDnsFilter();
	error = GetLastFilterApiError();
	if (error != NO_ERROR)
	{
		cout << "Oops, you are not supposed to get error code " << error << endl;
	}
	RemoveFilter(filterHandle);
	error = GetLastFilterApiError();
	if (error != NO_ERROR)
	{
		cout << "Oops, you are not supposed to get error code " << error << endl;
	}

	filterHandle = AddAllowTunnelFilter(L"WFPTool");
	error = GetLastFilterApiError();
	if (error != NO_ERROR)
	{
		cout << "Oops, you are not supposed to get error code " << error << endl;
	}
	RemoveFilter(filterHandle);
	error = GetLastFilterApiError();
	if (error != NO_ERROR)
	{
		cout << "Oops, you are not supposed to get error code " << error << endl;
	}

	filterHandle = AddAllowSpecificFilter("192.168.2.1", IPPROTO_TCP, 443);
	error = GetLastFilterApiError();
	if (error != NO_ERROR)
	{
		cout << "Oops, you are not supposed to get error code " << error << endl;
	}
	RemoveFilter(filterHandle);
	error = GetLastFilterApiError();
	if (error != NO_ERROR)
	{
		cout << "Oops, you are not supposed to get error code " << error << endl;
	}

	filterHandle = AddAllowSpecificHostnameFilter("www.google.com", IPPROTO_TCP, 443);
	error = GetLastFilterApiError();
	if (error != NO_ERROR)
	{
		cout << "Oops, you are not supposed to get error code " << error << endl;
	}
	RemoveFilter(filterHandle);
	error = GetLastFilterApiError();
	if (error != NO_ERROR)
	{
		cout << "Oops, you are not supposed to get error code " << error << endl;
	}
}