#pragma once
#ifndef WFP_ENGINE_H
#define WFP_ENGINE_H

#include <vector>
#include <functional>

using namespace std;

namespace wfptool
{
	wchar_t* MakeName(const wchar_t* str);

	class WfpEngine
	{
		friend class WfpEngineTests;

		static const size_t ERROR_MESSAGE_BUFFER_SIZE = 256 + 1;
		FWPM_SESSION _session;
		FWPM_PROVIDER _provider;
		FWPM_SUBLAYER _subLayer;

		static void LogAndThrowWfpEngineError(const wstring& errorMessage, UINT32 errorCode);
		static void LogAndThrowWfpEngineError(const wstring& methodName, const wstring& filter, unsigned long errorCode);
		void Initialize();
		void WorkWithWfpEngineTransaction(function<void(const HANDLE&)> worker) const;

		typedef struct _TRANSACTION_CONTEXT
		{
			HANDLE    EngineHandle;
			std::exception_ptr Exception;
		} TRANSACTION_CONTEXT, *PTRANSACTION_CONTEXT;

		void WorkWithWfpEngineTransaction_CppEH(function<void(const HANDLE&)>* worker, PTRANSACTION_CONTEXT ctx) const;
		void WorkWithWfpEngineTransaction_SEH(function<void(const HANDLE&)>* worker, PTRANSACTION_CONTEXT ctx) const;
		static void CleanupEngine(const HANDLE& engineHandle, UINT32 returnCode);
		void AddProviderAndSubLayer(const HANDLE& engineHandle) const;
		void AddFilter(const HANDLE& engineHandle, FWPM_FILTER& filter);
		static void RemoveFilter(const HANDLE& engineHandle, const FWPM_FILTER& filter);
		void RemoveProviderAndSubLayer(const HANDLE& engineHandle) const;

		static GUID* GetProviderKey();
	public:
		typedef vector<FWPM_FILTER*> FilterVector;

		WfpEngine();
		void ApplyFilter(FWPM_FILTER& filter);
		void ApplyFilters(const FilterVector& filters);
		void RemoveFilter(const FWPM_FILTER& filter);
		void RemoveFilters(const FilterVector& filters, bool removeProviderAndSubLayer = true);
		void RemoveAllFilters(bool removeProviderAndSubLayer) const;
		static void RemoveFiltersByProviderKey(const HANDLE& engineHandle, const GUID& providerKey);
	};
}

#endif