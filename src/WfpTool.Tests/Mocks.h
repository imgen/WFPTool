#pragma once

#ifndef MOCKS_H
#define MOCKS_H

#include <windows.h>
#include <Fwpmtypes.h>
#include <functional>

using namespace std;

/**Error code provider*/
DWORD ProvideDefaultErrorCode(const string& methodName);

void SetMockErrorCodeProvider(function<DWORD(const string&)> provider);

void RestoreDefaultErrorCodeProvider();

/**Add filter callback*/
DWORD ProvideDefaultAddFilterCallback(
	HANDLE handle,
	const FWPM_FILTER* filter,
	PSECURITY_DESCRIPTOR sd,
	UINT64* id);

void SetAddFilterCallback(
	function<DWORD(HANDLE,
		const FWPM_FILTER*,
		PSECURITY_DESCRIPTOR,
		UINT64*)> callback
	);

void RestoreDefaultAddFilterCallback();

/**Delete filter by key callback*/
DWORD ProvideDefaultDeleteFilterByKeyCallback(HANDLE handle, const GUID* key);

void SetDeleteFilterByKeyCallback(
	function<DWORD(HANDLE,
		const GUID*)>  callback
	);

void RestoreDefaultDeleteFilterByKeyCallback();

/**Set mock objects*/
void SetMockProvider(FWPM_PROVIDER provider);

void SetMockSubLayer(FWPM_SUBLAYER subLayer);

void SetMockFilters(FWPM_FILTER** filters, UINT32 filterCount);

void RestoreDefaultMockingBehavior();

typedef DWORD(WINAPI * TFwpmFilterEnum)(
	_In_ HANDLE engineHandle,
	_In_ HANDLE enumHandle,
	_In_ UINT32 numEntriesRequested,
	_Outptr_result_buffer_(*numEntriesReturned) FWPM_FILTER0*** entries,
	_Out_ UINT32* numEntriesReturned
	);


void SetMockFwpmFilterEnum(TFwpmFilterEnum mock);

#endif
