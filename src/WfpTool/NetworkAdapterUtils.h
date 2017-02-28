#pragma once

#ifndef NETWORK_ADAPTER_UTILS_H
#define NETWORK_ADAPTER_UTILS_H
#include <string>
#include <functional>
#include <set>

using namespace std;

namespace wfptool
{
	class NetworkAdapterUtils
	{
		friend class NetworkAdapterUtilsTests;
		friend class FilterApiProviderTests;
		friend class FilterApiExportTests;
		static void EnumerateNetworkAdapters(function<bool(const MIB_IF_ROW2&)> processor);

	public:
		static long GetInterfaceIndexByAdapterNameOrGuid(const wstring& adapterNameOrGuid);
		static set<pair<ULONG, string>> GetAllDnsServers();
		static ULONG ConvertIpv4AddressToUlong(const string& ip);
		static set<ULONG> ConvertIpv4HostnameToUlongAddresses(const string& hostname);
	};
}

#endif
