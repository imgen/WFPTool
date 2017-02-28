#include "stdafx.h"
#include "WfpApp.h"
#include "WfpFilter.h"
#include "NetworkAdapterUtils.h"

namespace wfptool
{
	// {59C02808-D61E-44A4-AF12-7DA45E36A73B}
	DEFINE_GUID(blockAllOutboundFilterGuid,
		0x59c02808, 0xd61e, 0x44a4, 0xaf, 0x12, 0x7d, 0xa4, 0x5e, 0x36, 0xa7, 0x3b);
	// {D1FE362B-33A3-4E8F-A489-4D833CF4F8A7}
	DEFINE_GUID(allowOneOutboundFilterGuid,
		0xd1fe362b, 0x33a3, 0x4e8f, 0xa4, 0x89, 0x4d, 0x83, 0x3c, 0xf4, 0xf8, 0xa7);
	// {B2BE8D34-4855-4CA6-AE1A-8AE049AB38EF}
	DEFINE_GUID(allowOutboundInterfaceFilterGuid,
		0xb2be8d34, 0x4855, 0x4ca6, 0xae, 0x1a, 0x8a, 0xe0, 0x49, 0xab, 0x38, 0xef);
	// {8B70FBE3-73C4-486B-AF37-AE8F648CC356}
	DEFINE_GUID(allowLocalLoopback,
		0x8b70fbe3, 0x73c4, 0x486b, 0xaf, 0x37, 0xae, 0x8f, 0x64, 0x8c, 0xc3, 0x56);

	WfpApp::WfpApp(string ipAddress, int port, int interfaceIndex)
	{
		_ipAddress = ipAddress;
		_port = port;
		_interfaceIndex = interfaceIndex;
		InitializeFilters();
	}


	WfpEngine::FilterVector WfpApp::ConvertToRawFilters()
	{
		_rawFilters.clear();
		for (auto wfpFilter : _filters)
		{
			_rawFilters.push_back(&wfpFilter->BuildRawFilter());
		}
		return _rawFilters;
	}


	void WfpApp::ApplyFilters()
	{
		_wfpEngine.ApplyFilters(ConvertToRawFilters());
	}

	void WfpApp::RemoveFilters(bool removeProviderAndSubLayer) const
	{
		if (!_filters.empty())
		{
			_wfpEngine.RemoveAllFilters(removeProviderAndSubLayer);
		}
	}

	WfpApp::~WfpApp()
	{
		for (auto filter : _filters)
		{
			delete filter;
		}

		_filters.clear();
	}

	void WfpApp::InitializeFilters()
	{
		ULONG addr = NetworkAdapterUtils::ConvertIpv4AddressToUlong(_ipAddress);

		WfpFilter* filter = new WfpFilter(
			blockAllOutboundFilterGuid,
			MakeName(L"block all outbound filter"),
			FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
			1,
			FWP_ACTION_BLOCK);
		_filters.push_back(filter);

		filter = new WfpFilter(
			allowOneOutboundFilterGuid,
			MakeName(L"allow outbound endpoint filter"),
			FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
			0xF,
			FWP_ACTION_PERMIT);

		filter->AddMatchProtocolIpAndPortConditions(
			IPPROTO_TCP,
			addr,
			_port);
		_filters.push_back(filter);

		filter = new WfpFilter(
			allowLocalLoopback,
			MakeName(L"allow outbound local loopback filter"),
			FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
			0xF,
			FWP_ACTION_PERMIT);
		filter->AddCondition(FWPM_CONDITION_LOCAL_INTERFACE_TYPE, FWP_MATCH_EQUAL, FWP_UINT32, MIB_IF_TYPE_LOOPBACK);
		_filters.push_back(filter);

		if (_interfaceIndex >= 0)
		{
			filter = new WfpFilter(
				allowOutboundInterfaceFilterGuid,
				MakeName(L"allow outbound interface index filter"),
				FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
				0xF,
				FWP_ACTION_PERMIT);
			filter->AddCondition(FWPM_CONDITION_INTERFACE_INDEX, FWP_MATCH_EQUAL, FWP_UINT32, _interfaceIndex);
			_filters.push_back(filter);
		}
	}
}