#pragma once

#ifndef WFP_FITER_H
#define WFP_FITER_H

#include <vector>

using namespace std;

class WfpFilter
{
	FWPM_FILTER* _filter;
	vector<FWPM_FILTER_CONDITION> _conditions;

public:
	WfpFilter(const GUID& fiterGuid, wchar_t* filterName, const GUID& direction, UINT8 priority, UINT32 actionType);
	virtual ~WfpFilter();

	void AddCondition(
		const GUID& fieldKey,
		FWP_MATCH_TYPE matchType,
		FWP_DATA_TYPE conditionValueDataType,
		UINT64 conditionValue);
	void AddCondition(FWPM_FILTER_CONDITION& condition);
	void AddMatchProtocolIpAndPortConditions(
		UINT8 protocol,
		unsigned long address,
		int port);
	FWPM_FILTER& BuildRawFilter();
	FWPM_FILTER& GetRawFilter()const;
	const GUID& GetFilterKey()const;
	const wchar_t* GetName() const;
};

#endif
