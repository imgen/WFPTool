#include "stdafx.h"
#include "Mocks.h"

#pragma comment(lib,"rpcrt4.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"IPHlpApi.lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

static ULONG mockEngineHandle = 1, mockFilterEnumHandle = 1;
static DWORD mockErrorCode = NO_ERROR;
static function<DWORD(const string&)> errorCodeProvider = ProvideDefaultErrorCode;
static FWPM_PROVIDER mockProvider;
static FWPM_SUBLAYER mockSubLayer;
static UINT64 mockFilterId = 1;
static FWPM_FILTER** mockFilters;
static UINT32 mockFilterCount = 0;

static function<DWORD(HANDLE,
	const FWPM_FILTER*,
	PSECURITY_DESCRIPTOR,
	UINT64*)>
	addFilterCallback = ProvideDefaultAddFilterCallback;

static function<DWORD(HANDLE,
	const GUID*)> deleteFilterByKeyCallback;

DWORD ProvideDefaultErrorCode(const string& methodName)
{
	return NO_ERROR;
}

void SetMockErrorCodeProvider(function<DWORD(const string&)> provider)
{
	errorCodeProvider = provider;
}

void RestoreDefaultErrorCodeProvider()
{
	errorCodeProvider = ProvideDefaultErrorCode;
}

DWORD ProvideDefaultAddFilterCallback(
	HANDLE handle,
	const FWPM_FILTER* filter,
	PSECURITY_DESCRIPTOR sd,
	UINT64* id)
{
	return NO_ERROR;
}

void SetAddFilterCallback(
	function<DWORD(HANDLE, const FWPM_FILTER*, PSECURITY_DESCRIPTOR, UINT64*)> callback
	)
{
	addFilterCallback = callback;
}

void RestoreDefaultAddFilterCallback()
{
	addFilterCallback = ProvideDefaultAddFilterCallback;
}

DWORD ProvideDefaultDeleteFilterByKeyCallback(HANDLE handle, const GUID* key)
{
	return NO_ERROR;
}

void SetDeleteFilterByKeyCallback(function<DWORD(HANDLE, const GUID*)> callback)
{
	deleteFilterByKeyCallback = callback;
}

void RestoreDefaultDeleteFilterByKeyCallback()
{
	deleteFilterByKeyCallback = ProvideDefaultDeleteFilterByKeyCallback;
}

static void SetMockErrorCode(const string& methodName)
{
	mockErrorCode = errorCodeProvider(methodName);
}

void SetMockProvider(FWPM_PROVIDER provider)
{
	mockProvider = provider;
}

void SetMockSubLayer(FWPM_SUBLAYER subLayer)
{
	mockSubLayer = subLayer;
}

void SetMockFilters(FWPM_FILTER** filters, UINT32 filterCount)
{
	mockFilters = filters;
	mockFilterCount = filterCount;
}

void RestoreDefaultMockingBehavior()
{
	RestoreDefaultErrorCodeProvider();
	RestoreDefaultAddFilterCallback();
	RestoreDefaultDeleteFilterByKeyCallback();
	ZeroMemory(&mockProvider, sizeof(FWPM_PROVIDER));
	ZeroMemory(&mockSubLayer, sizeof(FWPM_SUBLAYER));
	SetMockFilters(nullptr, 0);
	SetMockFwpmFilterEnum(nullptr);
}

DWORD WINAPI FwpmEngineOpen(
	_In_opt_ const wchar_t                   *serverName,
	_In_           UINT32                    authnService,
	_In_opt_       SEC_WINNT_AUTH_IDENTITY_W *authIdentity,
	_In_opt_ const FWPM_SESSION0             *session,
	_Out_          HANDLE                    *engineHandle
	)
{
	*engineHandle = &mockEngineHandle;
	return NO_ERROR;
}

DWORD WINAPI FwpmEngineClose(_Inout_ HANDLE engineHandle)
{
	return NO_ERROR;
}

DWORD WINAPI FwpmTransactionBegin(
	_In_ HANDLE engineHandle,
	_In_ UINT32 flags
	)
{
	return NO_ERROR;
}

DWORD WINAPI FwpmTransactionCommit(_In_ HANDLE engineHandle)
{
	return NO_ERROR;
}

DWORD WINAPI FwpmTransactionAbort(_In_ HANDLE engineHandle)
{
	return NO_ERROR;
}

DWORD WINAPI FwpmProviderAdd(
	_In_ HANDLE engineHandle,
	_In_ const FWPM_PROVIDER0* provider,
	_In_opt_ PSECURITY_DESCRIPTOR sd
	)
{
	SetMockErrorCode("FwpmProviderAdd");
	if (mockErrorCode == NO_ERROR)
	{
		mockProvider = *provider;
	}
	return mockErrorCode;
}

DWORD WINAPI FwpmProviderGetByKey(
	_In_ HANDLE engineHandle,
	_In_ const GUID* key,
	_Outptr_ FWPM_PROVIDER0** provider
	)
{
	SetMockErrorCode("FwpmProviderGetByKey");
	if (mockErrorCode == NO_ERROR)
	{
		*provider = &mockProvider;
	}
	return mockErrorCode;
}

DWORD WINAPI FwpmProviderDeleteByKey(
	_In_ HANDLE engineHandle,
	_In_ const GUID* key)
{
	SetMockErrorCode("FwpmProviderDeleteByKey");
	return mockErrorCode;
}

DWORD WINAPI FwpmSubLayerAdd(
	_In_ HANDLE engineHandle,
	_In_ const FWPM_SUBLAYER0* subLayer,
	_In_opt_ PSECURITY_DESCRIPTOR sd
	)
{
	SetMockErrorCode("FwpmSubLayerAdd");
	if (mockErrorCode == NO_ERROR)
	{
		mockSubLayer = *subLayer;
	}
	return mockErrorCode;
}

DWORD WINAPI FwpmSubLayerGetByKey(
	_In_ HANDLE engineHandle,
	_In_ const GUID* key,
	_Outptr_ FWPM_SUBLAYER0** subLayer
	)
{
	SetMockErrorCode("FwpmSubLayerGetByKey");
	if (mockErrorCode == NO_ERROR)
	{
		*subLayer = &mockSubLayer;
	}
	return mockErrorCode;
}

DWORD WINAPI FwpmSubLayerDeleteByKey(
	_In_ HANDLE engineHandle,
	_In_ const GUID* key
	)
{
	SetMockErrorCode("FwpmSubLayerDeleteByKey");
	return mockErrorCode;
}

DWORD WINAPI FwpmFilterAdd(
	_In_ HANDLE engineHandle,
	_In_ const FWPM_FILTER0* filter,
	_In_opt_ PSECURITY_DESCRIPTOR sd,
	_Out_opt_ UINT64* id
	)
{
	return addFilterCallback(engineHandle, filter, sd, id);
}

DWORD WINAPI FwpmFilterDeleteByKey(
	_In_ HANDLE engineHandle,
	_In_ const GUID* key
	)
{
	return deleteFilterByKeyCallback(engineHandle, key);
}

DWORD WINAPI FwpmFilterCreateEnumHandle(
	_In_ HANDLE engineHandle,
	_In_opt_ const FWPM_FILTER_ENUM_TEMPLATE0* enumTemplate,
	_Out_ HANDLE* enumHandle
	)
{
	*enumHandle = &mockFilterEnumHandle;
	return NO_ERROR;
}


DWORD WINAPI default_FwpmFilterEnum(
	_In_ HANDLE engineHandle,
	_In_ HANDLE enumHandle,
	_In_ UINT32 numEntriesRequested,
	_Outptr_result_buffer_(*numEntriesReturned) FWPM_FILTER0*** entries,
	_Out_ UINT32* numEntriesReturned
	)
{
	*numEntriesReturned = mockFilterCount;
	*entries = mockFilters;
	return NO_ERROR;
}

TFwpmFilterEnum  mock_FwpmFilterEnum = default_FwpmFilterEnum;

void SetMockFwpmFilterEnum(TFwpmFilterEnum mock)
{
	if (mock)
		mock_FwpmFilterEnum = mock;
	else
		mock_FwpmFilterEnum = default_FwpmFilterEnum;
}

DWORD WINAPI FwpmFilterEnum(
	_In_ HANDLE engineHandle,
	_In_ HANDLE enumHandle,
	_In_ UINT32 numEntriesRequested,
	_Outptr_result_buffer_(*numEntriesReturned) FWPM_FILTER0*** entries,
	_Out_ UINT32* numEntriesReturned
	)
{
	return mock_FwpmFilterEnum(engineHandle, enumHandle, numEntriesRequested, entries, numEntriesReturned);
}

DWORD WINAPI FwpmFilterDestroyEnumHandle(
	_In_ HANDLE engineHandle,
	_Inout_ HANDLE enumHandle
	)
{
	return NO_ERROR;
}

void WINAPI FwpmFreeMemory(
	_Inout_ void **p
	)
{
	Assert::IsTrue(*p == reinterpret_cast<VOID*>(mockFilters), L"FwpmFreeMemory called on something wrong");
	*p = mockFilters = nullptr;
}
