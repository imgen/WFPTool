#include "stdafx.h"
#include "WfpFilter.h"

WfpFilter::WfpFilter(const GUID& fiterGuid, wchar_t* filterName, const GUID& direction, UINT8 priority, UINT32 actionType)
{
	_filter = new FWPM_FILTER();
	ZeroMemory(_filter, sizeof(FWPM_FILTER));
	_filter->filterKey = fiterGuid;
	_filter->displayData.name = filterName;
	_filter->layerKey = direction;
	_filter->weight.type = FWP_UINT8;
	_filter->weight.uint8 = priority;
	_filter->action.type = actionType;
}

WfpFilter::~WfpFilter()
{
	delete _filter;
	_filter = nullptr;
}

void WfpFilter::AddCondition(const GUID& fieldKey, FWP_MATCH_TYPE matchType, FWP_DATA_TYPE conditionValueDataType, UINT64 conditionValue)
{
	FWPM_FILTER_CONDITION condition;
	condition.fieldKey = fieldKey;
	condition.matchType = matchType;
	condition.conditionValue.type = conditionValueDataType;
	if (conditionValueDataType == FWP_UINT8)
	{
		condition.conditionValue.uint8 = static_cast<UINT8>(conditionValue);
	}
	else if (conditionValueDataType == FWP_UINT16)
	{
		condition.conditionValue.uint16 = static_cast<UINT16>(conditionValue);
	}
	else if (conditionValueDataType == FWP_UINT32)
	{
		condition.conditionValue.uint32 = static_cast<UINT32>(conditionValue);
	}
	else if (conditionValueDataType == FWP_V4_ADDR_MASK)
	{
		auto addrAndMask = reinterpret_cast<FWP_V4_ADDR_AND_MASK *>(conditionValue);
		condition.conditionValue.v4AddrMask = addrAndMask;
	}
	AddCondition(condition);
}

void WfpFilter::AddCondition(FWPM_FILTER_CONDITION& condition)
{
	_conditions.push_back(condition);
}

void WfpFilter::AddMatchProtocolIpAndPortConditions(
	UINT8 protocol,
	unsigned long address,
	int port)
{
	if (protocol > 0)
	{
		AddCondition(
			FWPM_CONDITION_IP_PROTOCOL,
			FWP_MATCH_EQUAL,
			FWP_UINT8,
			protocol);
	}

	if (address > 0)
	{
		AddCondition(
			FWPM_CONDITION_IP_REMOTE_ADDRESS,
			FWP_MATCH_EQUAL,
			FWP_UINT32,
			address);
	}

	if (port > 0 &&
		(protocol == IPPROTO_TCP ||
		 protocol == IPPROTO_UDP))
	{
		AddCondition(
			FWPM_CONDITION_IP_REMOTE_PORT,
			FWP_MATCH_EQUAL,
			FWP_UINT16,
			port);
	}
}

FWPM_FILTER& WfpFilter::BuildRawFilter()
{
	_filter->filterCondition = _conditions.data();
	_filter->numFilterConditions = static_cast<UINT32>(_conditions.size());
	return *_filter;
}

FWPM_FILTER& WfpFilter::GetRawFilter() const
{
	return *_filter;
}

const GUID& WfpFilter::GetFilterKey()const
{
	return _filter->filterKey;
}

const wchar_t* WfpFilter::GetName()const
{
	return _filter->displayData.name;
}