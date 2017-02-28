#include "stdafx.h"
#include "NetworkAdapterUtils.h"
#include "Netioapi.h"
#include "iphlpapi.h"
#include <algorithm>
#include "WfpEngineException.h"

namespace wfptool
{
	static const size_t IP_V4_ADDRESS_BUFFER_SIZE = 16;

	static string GetIpV4AddressFromSockAddrStruct(struct sockaddr* sa)
	{
		char buffer[IP_V4_ADDRESS_BUFFER_SIZE];
		inet_ntop(AF_INET,
			&(reinterpret_cast<struct sockaddr_in*>(sa)->sin_addr),
			buffer,
			IP_V4_ADDRESS_BUFFER_SIZE);
		return buffer;
	}

	void NetworkAdapterUtils::EnumerateNetworkAdapters(function<bool(const MIB_IF_ROW2&)> processor)
	{
		PMIB_IF_TABLE2 interfaceInfos;
		auto retval = GetIfTable2Ex(MibIfTableNormal, &interfaceInfos);
		WfpEngineException::ThrowIfSystemError(retval);

		for (UINT32 i = 0; i < interfaceInfos->NumEntries; i++)
		{
			auto interfaceInfo = interfaceInfos->Table[i];

			bool continueToEnumerate = processor(interfaceInfo);
			if (!continueToEnumerate)
			{
				break;
			}
		}
		FreeMibTable(interfaceInfos);
	}

	long NetworkAdapterUtils::GetInterfaceIndexByAdapterNameOrGuid(const wstring& adapterNameOrGuid)
	{
		long interfaceIndex = -1;
		auto adapterNameOrGuidAsCharArray = adapterNameOrGuid.c_str();
		auto adapterNameOrGuid2 = adapterNameOrGuid;
		auto adapterNameOrGuidAsCharArray2 = adapterNameOrGuid2.c_str();
		if (*adapterNameOrGuidAsCharArray2 == L'{')
		{
			// ReSharper disable once CppAssignedValueIsNeverUsed
			adapterNameOrGuidAsCharArray2++;
		}
		EnumerateNetworkAdapters([&adapterNameOrGuid,
			&interfaceIndex,
			&adapterNameOrGuidAsCharArray,
			&adapterNameOrGuidAsCharArray2](const MIB_IF_ROW2& interfaceInfo)
		{
			wchar_t* interfaceGuidString, *networkGuidString;
			UuidToString(&interfaceInfo.InterfaceGuid, &interfaceGuidString);
			UuidToString(&interfaceInfo.NetworkGuid, &networkGuidString);

			if (wcscmp(adapterNameOrGuidAsCharArray, interfaceInfo.Description) == 0 ||
				wcscmp(adapterNameOrGuidAsCharArray, interfaceInfo.Alias) == 0)
			{
				interfaceIndex = interfaceInfo.InterfaceIndex;
				return false;
			}

			if (_wcsnicmp(adapterNameOrGuidAsCharArray2, interfaceGuidString, wcslen(interfaceGuidString)) == 0 ||
				_wcsnicmp(adapterNameOrGuidAsCharArray2, networkGuidString, wcslen(networkGuidString)) == 0)
			{
				interfaceIndex = interfaceInfo.InterfaceIndex;
				return false;
			}
			return true;
		});

		return interfaceIndex;
	}

	set<pair<ULONG, string>> NetworkAdapterUtils::GetAllDnsServers()
	{
		set<pair<ULONG, string>> dnsServers;
		const int maxTries = 3;
		const int workingBufferSize = 15000;
		PIP_ADAPTER_ADDRESSES addresses;
		ULONG flags = GAA_FLAG_INCLUDE_ALL_INTERFACES |
			GAA_FLAG_INCLUDE_TUNNEL_BINDINGORDER;

		PIP_ADAPTER_ADDRESSES currentAddresses;
		ULONG family = AF_INET;

		int tryCount = 0;
		UINT32 returnCode;
		ULONG bufferLength = workingBufferSize;
		bool isBufferOverflow;
		do
		{
			addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES> (
				new unsigned char[bufferLength]);
			returnCode = GetAdaptersAddresses(
				family, flags, nullptr, addresses, &bufferLength);
			isBufferOverflow = returnCode == ERROR_BUFFER_OVERFLOW;
			if (isBufferOverflow)
			{
				delete[] addresses;
				addresses = nullptr;
			}
		} while (tryCount < maxTries && isBufferOverflow);

		if (returnCode == NO_ERROR)
		{
			currentAddresses = addresses;
			while(currentAddresses != nullptr)
			{
				ULONG interfaceId = currentAddresses->IfIndex;
				auto dnsServerPointer = currentAddresses->FirstDnsServerAddress;
				while(dnsServerPointer != nullptr)
				{
					auto dns = dnsServerPointer->Address;
					auto dnsServer = GetIpV4AddressFromSockAddrStruct(dns.lpSockaddr);
					dnsServers.insert(
						make_pair(interfaceId, dnsServer)
					);
					dnsServerPointer = dnsServerPointer->Next;
				}

				currentAddresses = currentAddresses->Next;
			}

			delete[] addresses;
		}

		return dnsServers;
	}

	ULONG NetworkAdapterUtils::ConvertIpv4AddressToUlong(const string& ip)
	{
		if (ip.empty())
		{
			return 0;
		}
		ULONG addr;
		inet_pton(AF_INET, ip.c_str(), &addr);
		addr = htonl(addr);
		return addr;
	}

	set<ULONG> NetworkAdapterUtils::ConvertIpv4HostnameToUlongAddresses(const string& hostname)
	{
		set<ULONG> hostnameIps;
		if (hostname.empty())
		{
			return hostnameIps;
		}

		WSADATA wsaData;
		int returnCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (returnCode != NO_ERROR)
		{
			return hostnameIps;
		}

		struct addrinfo hints;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		struct addrinfo *result = nullptr;

		returnCode = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
		if (returnCode != NO_ERROR)
		{
			return hostnameIps;
		}

		struct addrinfo *addrPointer = result;
		for (; addrPointer != nullptr; addrPointer = addrPointer->ai_next)
		{
			if (addrPointer->ai_family == AF_INET)
			{
				auto addressAsString = GetIpV4AddressFromSockAddrStruct(addrPointer->ai_addr);
				auto address = ConvertIpv4AddressToUlong(addressAsString);
				hostnameIps.insert(address);
			}
		}

		return hostnameIps;
	}
}
