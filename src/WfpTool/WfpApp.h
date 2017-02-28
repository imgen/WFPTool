#pragma once
#ifndef WFP_APP_H
#define WFP_APP_H

#include "WfpFilter.h"
#include "WfpEngine.h"

namespace wfptool
{
	class WfpApp
	{
		string _ipAddress;
		int _port;
		int _interfaceIndex;
		WfpEngine _wfpEngine;
		vector<WfpFilter*> _filters;
		WfpEngine::FilterVector _rawFilters;

		WfpEngine::FilterVector ConvertToRawFilters();

	public:
		static const int DEFAULT_ALLOW_PORT = 443;
		WfpApp(string ipAddress, int port = DEFAULT_ALLOW_PORT, int interfaceIndex = -1);
		~WfpApp();
		void InitializeFilters();
		void ApplyFilters();
		void RemoveFilters(bool removeProviderAndSubLayer = true) const;
	};
}

#endif