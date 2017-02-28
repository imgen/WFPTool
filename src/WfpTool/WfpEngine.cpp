#include "stdafx.h"
#include "WfpEngine.h"
#include "WfpEngineException.h"
#include <iostream>
#include <sstream>

namespace wfptool
{
	static const wstring TOOL_NAME = L"WFP Tool - ";

	wchar_t* MakeName(const wchar_t* str)
	{
		return _wcsdup((TOOL_NAME + str).c_str());
	}

	// {6736D0F4-396F-4F48-B8A4-07875C2DC51D}
	DEFINE_GUID(providerGuid,
		0x6736d0f4, 0x396f, 0x4f48, 0xb8, 0xa4, 0x7, 0x87, 0x5c, 0x2d, 0xc5, 0x1d);
	// {1E95FDAE-6A96-4FFF-9AA3-85A900179874}
	DEFINE_GUID(subLayerGuid,
		0x1e95fdae, 0x6a96, 0x4fff, 0x9a, 0xa3, 0x85, 0xa9, 0x0, 0x17, 0x98, 0x74);

	void WfpEngine::LogAndThrowWfpEngineError(const wstring& errorMessage, UINT32 errorCode)
	{
		wstringstream ss;
		ss << errorMessage << endl << "Error code: " << hex << errorCode << endl;

		wcout << ss.str();
		size_t numberOfBytes;
		char errorBuf[ERROR_MESSAGE_BUFFER_SIZE];
		wcstombs_s(&numberOfBytes, errorBuf, ERROR_MESSAGE_BUFFER_SIZE, errorMessage.c_str(), ERROR_MESSAGE_BUFFER_SIZE / 2);
		throw WfpEngineException(errorBuf, errorCode);
	}

	void WfpEngine::LogAndThrowWfpEngineError(const wstring& methodName, const wstring& filterName, unsigned long errorCode)
	{
		wstringstream errorSs;
		errorSs << methodName << " for filter " <<
			filterName <<
			" failed";
		LogAndThrowWfpEngineError(errorSs.str(), errorCode);
	}

	WfpEngine::WfpEngine()
	{
		Initialize();
	}

	void WfpEngine::Initialize()
	{
		ZeroMemory(&_session,
			sizeof(FWPM_SESSION));
		ZeroMemory(&_provider,
			sizeof(FWPM_PROVIDER));
		ZeroMemory(&_subLayer,
			sizeof(FWPM_SUBLAYER));

		_session.displayData.name = MakeName(L"User Mode Session");

		_provider.displayData.name = MakeName(L"Provider");
		_provider.providerKey = providerGuid;
		//_provider.flags = FWPM_PROVIDER_FLAG_PERSISTENT;

		_subLayer.displayData.name = MakeName(L"SubLayer");
		_subLayer.subLayerKey = subLayerGuid;
		_subLayer.providerKey = &_provider.providerKey;
		//_subLayer.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
	}



	void WfpEngine::WorkWithWfpEngineTransaction(function<void(const HANDLE&)> worker) const
	{

		// we have to handle both SEH and C++ exceptions in this function

		// C++ exceptions are stored in TRANSACTION_CONTEXT to be rethrown here

		PTRANSACTION_CONTEXT ctx = new TRANSACTION_CONTEXT;
		WorkWithWfpEngineTransaction_SEH(&worker, ctx);

		std::exception_ptr p = ctx->Exception;

		delete ctx;

		if (p)
		{
			std::rethrow_exception(p);
		}

	}



	void WfpEngine::WorkWithWfpEngineTransaction_SEH(function<void(const HANDLE&)> *worker, PTRANSACTION_CONTEXT ctx) const
	{
		// this function won't throw C++ exceptions or SEH exceptions
		__try
		{
			WorkWithWfpEngineTransaction_CppEH(worker, ctx);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			auto eCode = GetExceptionCode();
			cout << "ERROR! [SEH handler] exception code " << eCode << endl;
			CleanupEngine(ctx->EngineHandle, 1);
		}
	}

	void WfpEngine::WorkWithWfpEngineTransaction_CppEH(function<void(const HANDLE&)>* worker, PTRANSACTION_CONTEXT ctx) const
	{
		HANDLE& engineHandle = ctx->EngineHandle;
		try
		{
			UINT32 returnCode = FwpmEngineOpen(nullptr,
				RPC_C_AUTHN_DEFAULT,
				nullptr,
				&_session,
				&engineHandle);
			if (returnCode != NO_ERROR)
			{
				LogAndThrowWfpEngineError(L"FwpmEngineOpen failed", returnCode);
			}

			returnCode = FwpmTransactionBegin(engineHandle, 0);
			if (returnCode != NO_ERROR)
			{
				LogAndThrowWfpEngineError(L"FwpmTransactionBegin failed", returnCode);
			}

			(*worker)(engineHandle);

			returnCode = FwpmTransactionCommit(engineHandle);
			if (returnCode != NO_ERROR)
			{
				LogAndThrowWfpEngineError(L"FwpmTransactionCommit failed", returnCode);
			}

			CleanupEngine(engineHandle, returnCode);
		}
		catch (WfpEngineException)
		{
			CleanupEngine(engineHandle, 1);
			// save exception into context to tell callers up the stack to rethrow it
			ctx->Exception = current_exception();
		}
	}

	void WfpEngine::CleanupEngine(const HANDLE & engineHandle, UINT32 returnCode)
	{
		if (engineHandle != nullptr)
		{
			if (returnCode != NO_ERROR)
			{
				FwpmTransactionAbort(engineHandle);
			}

			FwpmEngineClose(engineHandle);
		}
	}

	void WfpEngine::AddProviderAndSubLayer(const HANDLE& engineHandle) const
	{
		FWPM_PROVIDER* provider;
		UINT32 returnCode = FwpmProviderGetByKey(engineHandle, &providerGuid, &provider);
		if (returnCode == FWP_E_PROVIDER_NOT_FOUND)
		{
			returnCode = FwpmProviderAdd(engineHandle, &_provider, nullptr);
			if (returnCode != NO_ERROR)
			{
				LogAndThrowWfpEngineError(L"FwpmProviderAdd failed", returnCode);
			}
		}

		FWPM_SUBLAYER* subLayer;
		returnCode = FwpmSubLayerGetByKey(engineHandle, &subLayerGuid, &subLayer);
		if (returnCode == FWP_E_SUBLAYER_NOT_FOUND)
		{
			returnCode = FwpmSubLayerAdd(engineHandle, &_subLayer, nullptr);
			if (returnCode != NO_ERROR)
			{
				LogAndThrowWfpEngineError(L"FwpmSubLayerAdd failed", returnCode);
			}
		}
	}

	void WfpEngine::AddFilter(const HANDLE& engineHandle, FWPM_FILTER& filter)
	{
		filter.providerKey = &_provider.providerKey;
		filter.subLayerKey = subLayerGuid;
		UINT32 returnCode = FwpmFilterAdd(engineHandle,
			&filter,
			nullptr,
			&filter.filterId);
		if (returnCode != NO_ERROR)
		{
			LogAndThrowWfpEngineError(L"FwpmFilterAdd", filter.displayData.name, returnCode);
		}
	}

	void WfpEngine::ApplyFilters(const FilterVector& filters)
	{
		WorkWithWfpEngineTransaction([this, &filters](const HANDLE& engineHandle)
		{
			AddProviderAndSubLayer(engineHandle);

			for(auto filter: filters)
			{
				AddFilter(engineHandle, *filter);
			}
		});
	}

	void WfpEngine::ApplyFilter(FWPM_FILTER& filter)
	{
		WorkWithWfpEngineTransaction([this, &filter](const HANDLE& engineHandle)
		{
			AddProviderAndSubLayer(engineHandle);
			AddFilter(engineHandle, filter);
		});
	}

	void WfpEngine::RemoveFilter(const HANDLE& engineHandle, const FWPM_FILTER& filter)
	{
		int returnCode;
		returnCode = FwpmFilterDeleteByKey(engineHandle, &filter.filterKey);
		if (returnCode != NO_ERROR && returnCode != FWP_E_FILTER_NOT_FOUND)
		{
			LogAndThrowWfpEngineError(L"FwpmFilterDeleteByKey", filter.displayData.name, returnCode);
		}
	}

	void WfpEngine::RemoveProviderAndSubLayer(const HANDLE& engineHandle) const
	{
		FWPM_SUBLAYER* subLayer;
		UINT32 returnCode = FwpmSubLayerGetByKey(engineHandle, &_subLayer.subLayerKey, &subLayer);
		if (returnCode == NO_ERROR)
		{
			returnCode = FwpmSubLayerDeleteByKey(engineHandle, &_subLayer.subLayerKey);
			if (returnCode != NO_ERROR)
			{
				LogAndThrowWfpEngineError(L"FwpmSubLayerDeleteByKey failed", returnCode);
			}
		}

		FWPM_PROVIDER* provider;
		returnCode = FwpmProviderGetByKey(engineHandle, &_provider.providerKey, &provider);
		if (returnCode == NO_ERROR)
		{
			returnCode = FwpmProviderDeleteByKey(engineHandle, &_provider.providerKey);
			if (returnCode != NO_ERROR)
			{
				LogAndThrowWfpEngineError(L"FwpmProviderDeleteByKey failed", returnCode);
			}
		}
	}

	GUID* WfpEngine::GetProviderKey()
	{
		return const_cast<GUID*>(&providerGuid);
	}

	void WfpEngine::RemoveFilters(const FilterVector & filters, bool removeProviderAndSubLayer)
	{
		WorkWithWfpEngineTransaction([this, &filters, removeProviderAndSubLayer](const HANDLE& engineHandle)
		{
			for (auto filter : filters)
			{
				RemoveFilter(engineHandle, *filter);
			}

			if (removeProviderAndSubLayer)
			{
				RemoveProviderAndSubLayer(engineHandle);
			}
		});
	}

	void WfpEngine::RemoveFilter(const FWPM_FILTER& filter)
	{
		WorkWithWfpEngineTransaction([this, &filter](const HANDLE& engineHandle)
		{
			RemoveFilter(engineHandle, filter);
		});
	}

	void WfpEngine::RemoveAllFilters(bool removeProviderAndSubLayer) const
	{
		WorkWithWfpEngineTransaction([this, removeProviderAndSubLayer](const HANDLE& engineHandle)
		{
			RemoveFiltersByProviderKey(engineHandle, _provider.providerKey);
			if (removeProviderAndSubLayer)
			{
				RemoveProviderAndSubLayer(engineHandle);
			}
		});
	}

	void WfpEngine::RemoveFiltersByProviderKey(const HANDLE& engineHandle, const GUID& providerKey)
	{
		HANDLE enumHandle = nullptr;
		FwpmFilterCreateEnumHandle(engineHandle, nullptr, &enumHandle);

		int filterCount = 100;
		UINT32 filterEntriesCount;
		FWPM_FILTER** filters;
		while (true) 
		{
			auto returnCode = FwpmFilterEnum(engineHandle, enumHandle, filterCount, &filters, &filterEntriesCount);
			if (returnCode != NO_ERROR)
			{
				LogAndThrowWfpEngineError(L"FwpmFilterEnum failed", returnCode);
			}

			cout << "Got a total of " << filterEntriesCount << " filters" << endl;
			if (filters != nullptr && filterEntriesCount > 0)
			{
				for (UINT i = 0; i < filterEntriesCount; i++)
				{
					auto& filter = filters[i];
					if (filter && filter->providerKey && *(filter->providerKey) == providerKey)
					{
						returnCode = FwpmFilterDeleteByKey(engineHandle, &filter->filterKey);
						if (returnCode != NO_ERROR)
						{
							LogAndThrowWfpEngineError(L"FwpmFilterDeleteById failed", returnCode);
						}
						wchar_t* displayDataName = filter->displayData.name;

						wcout << "Deleted filter with name ";
						if (displayDataName)
							wcout << displayDataName;
						wcout << endl;
					}
				}
			}

			FwpmFreeMemory(reinterpret_cast<void**>(&filters));

			if (filterEntriesCount < filterCount)
			{
				break;
			}
		}

		FwpmFilterDestroyEnumHandle(engineHandle, enumHandle);
	}
}