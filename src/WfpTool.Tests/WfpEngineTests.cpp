#include "stdafx.h"
#include "CppUnitTest.h"
#include "../WfpTool/WfpEngine.h"
#include "../WfpTool/WfpEngineException.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

namespace wfptool
{
	TEST_CLASS(WfpEngineTests)
	{
		static void SetNotFoundErrorProviderForAddingProviderAndSubLayer()
		{
			SetMockErrorCodeProvider([](const string& methodName) -> DWORD
			{
				if (methodName == "FwpmProviderGetByKey")
				{
					return FWP_E_PROVIDER_NOT_FOUND;
				}
				else if (methodName == "FwpmSubLayerGetByKey")
				{
					return FWP_E_SUBLAYER_NOT_FOUND;
				}
				return ProvideDefaultErrorCode(methodName);
			});
		}

	public:
		TEST_METHOD_INITIALIZE(BeforeEachTestRun)
		{
			RestoreDefaultMockingBehavior();
		}

		TEST_METHOD(TestApplyFilter)
		{
			WfpEngine engine;
			vector<const FWPM_FILTER*> filterVector;
			UINT64 filterId = 0;
			int callCount = 0;
			SetAddFilterCallback([&filterVector, &filterId, &callCount](HANDLE handle,
				const FWPM_FILTER* filter,
				PSECURITY_DESCRIPTOR sd,
				UINT64* id)
			{
				callCount++;
				filterVector.push_back(filter);
				filterId++;
				*id = filterId;
				return NO_ERROR;
			});

			FWPM_FILTER testFilter;
			GUID guid;
			UuidCreate(&guid);
			testFilter.filterKey = guid;
			testFilter.displayData.name = MakeName(L"test filter");
			testFilter.layerKey = FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
			testFilter.action.type = FWP_ACTION_BLOCK;
			testFilter.weight.type = FWP_UINT8;
			testFilter.weight.uint8 = 1;

			engine.ApplyFilter(testFilter);

			Assert::IsTrue(testFilter.filterId == 1);

			Assert::IsTrue(&testFilter == filterVector.at(0));

			Assert::AreEqual(callCount, 1);

			SetNotFoundErrorProviderForAddingProviderAndSubLayer();

			engine.ApplyFilter(testFilter);

			Assert::IsTrue(testFilter.filterId == 2);

			Assert::IsTrue(&testFilter == filterVector.at(1));

			Assert::AreEqual(callCount, 2);

			SetAddFilterCallback([&callCount](HANDLE handle,
				const FWPM_FILTER* filter,
				PSECURITY_DESCRIPTOR sd,
				UINT64* id)
			{
				callCount++;
				return FWP_E_ALREADY_EXISTS;
			});

			Assert::ExpectException<WfpEngineException, function<void()>>
				([&engine, &testFilter]
			{
				engine.ApplyFilter(testFilter);
			},
			L"Error code returned should cause exception");

			Assert::AreEqual(callCount, 3);
		}

		TEST_METHOD(TestApplyFilters)
		{
			WfpEngine engine;
			vector<const FWPM_FILTER*> filterVector;
			UINT64 filterId = 0;
			int callCount = 0;
			SetAddFilterCallback([&filterVector, &filterId, &callCount](HANDLE handle,
				const FWPM_FILTER* filter,
				PSECURITY_DESCRIPTOR sd,
				UINT64* id)
			{
				callCount++;
				filterVector.push_back(filter);
				filterId++;
				*id = filterId;
				return NO_ERROR;
			});

			FWPM_FILTER filter1;
			GUID guid;
			UuidCreate(&guid);
			filter1.filterKey = guid;
			filter1.displayData.name = MakeName(L"test filter1");
			filter1.layerKey = FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
			filter1.action.type = FWP_ACTION_BLOCK;
			filter1.weight.type = FWP_UINT8;
			filter1.weight.uint8 = 1;

			FWPM_FILTER filter2;
			UuidCreate(&guid);
			filter2.filterKey = guid;
			filter2.displayData.name = MakeName(L"test filter2");
			filter2.layerKey = FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
			filter2.action.type = FWP_ACTION_PERMIT;
			filter2.weight.type = FWP_UINT8;
			filter2.weight.uint8 = 1;

			WfpEngine::FilterVector filters;
			filters.push_back(&filter1);
			filters.push_back(&filter2);

			engine.ApplyFilters(filters);

			Assert::IsTrue(filter1.filterId == 1);
			Assert::IsTrue(filter2.filterId == 2);

			Assert::IsTrue(&filter1 == filterVector.at(0));
			Assert::IsTrue(&filter2 == filterVector.at(1));

			Assert::AreEqual(callCount, 2);

			SetNotFoundErrorProviderForAddingProviderAndSubLayer();
			engine.ApplyFilters(filters);
			Assert::IsTrue(filter1.filterId == 3);
			Assert::IsTrue(filter2.filterId == 4);

			Assert::IsTrue(&filter1 == filterVector.at(2));
			Assert::IsTrue(&filter2 == filterVector.at(3));

			Assert::AreEqual(callCount, 4);

			SetAddFilterCallback([&callCount](HANDLE handle,
				const FWPM_FILTER* filter,
				PSECURITY_DESCRIPTOR sd,
				UINT64* id)
			{
				callCount++;
				return FWP_E_ALREADY_EXISTS;
			});

			Assert::ExpectException<WfpEngineException, function<void()>>
				([&engine, &filters]
			{
				engine.ApplyFilters(filters);
			},
			L"Error code returned should cause exception");

			Assert::AreEqual(callCount, 5);
		}

		TEST_METHOD(TestRemoveFilter)
		{
			int callCount = 0;
			SetDeleteFilterByKeyCallback([&callCount](HANDLE handle, const GUID* filterKey)
			{
				callCount++;
				return NO_ERROR;
			});

			WfpEngine engine;
			FWPM_FILTER filter;
			GUID guid;
			UuidCreate(&guid);
			filter.filterKey = guid;
			filter.displayData.name = L"Test filter";
			engine.RemoveFilter(filter);

			Assert::AreEqual(callCount, 1);

			SetDeleteFilterByKeyCallback([&callCount](HANDLE handle, const GUID* filterKey)
			{
				callCount++;
				return FWP_E_FILTER_NOT_FOUND;
			});

			engine.RemoveFilter(filter);
			Assert::AreEqual(callCount, 2,
				L"When FWP_E_FILTER_NOT_FOUND, should not throw exception but return successfully");

			SetDeleteFilterByKeyCallback([&callCount](HANDLE handle, const GUID* filterKey)
			{
				callCount++;
				return FWP_E_NOT_FOUND;
			});

			Assert::ExpectException<WfpEngineException, function<void()>>
				([&engine, &filter]
				{
					engine.RemoveFilter(filter);
				},
				L"Error code other than FWP_E_FILTER_NOT_FOUND should cause exception");
			Assert::AreEqual(callCount, 3);
		}

		TEST_METHOD(TestRemoveFilters)
		{
			int callCount = 0;
			SetDeleteFilterByKeyCallback([&callCount](HANDLE handle, const GUID* filterKey)
			{
				callCount++;
				return NO_ERROR;
			});

			WfpEngine engine;
			FWPM_FILTER filter1, filter2;
			GUID guid;
			UuidCreate(&guid);
			filter1.filterKey = guid;
			filter1.displayData.name = L"Test filter1";

			UuidCreate(&guid);
			filter2.filterKey = guid;
			filter2.displayData.name = L"Test filter2";

			engine.RemoveFilters({&filter1, &filter2});

			Assert::AreEqual(callCount, 2);

			SetDeleteFilterByKeyCallback([&callCount](HANDLE handle, const GUID* filterKey)
			{
				callCount++;
				return FWP_E_FILTER_NOT_FOUND;
			});

			engine.RemoveFilters({ &filter1, &filter2 });
			Assert::AreEqual(callCount, 4,
				L"When FWP_E_FILTER_NOT_FOUND, should not throw exception but return successfully");

			SetDeleteFilterByKeyCallback([&callCount](HANDLE handle, const GUID* filterKey)
			{
				callCount++;
				return FWP_E_NOT_FOUND;
			});

			Assert::ExpectException<WfpEngineException, function<void()>>
				([&engine, &filter1]
			{
				engine.RemoveFilter(filter1);
			},
			L"Error code other than FWP_E_FILTER_NOT_FOUND should cause exception");
			Assert::AreEqual(callCount, 5);
		}

		TEST_METHOD(TestWfpEngine_RemoveAllFilters)
		{
			WfpEngine engine;
			FWPM_FILTER filter1, filter2, filter3;

			GUID* providerKey = engine.GetProviderKey();

			GUID guid;
			UuidCreate(&guid);
			filter1.filterKey = guid;
			filter1.displayData.name = MakeName(L"Test filter1");
			filter1.providerKey = providerKey;

			UuidCreate(&guid);
			filter2.filterKey = guid;
			filter2.displayData.name = MakeName(L"Test filter2");
			filter2.providerKey = providerKey;

			UuidCreate(&guid);
			filter3.filterKey = guid;
			filter3.providerKey = nullptr;
			filter3.displayData.name = MakeName(L"Test filter2");

			FWPM_FILTER* filters[3] = { &filter1, &filter2, &filter3 };
			SetMockFilters(filters, 3);

			vector<FWPM_FILTER*> filterVector = { &filter1, &filter2, &filter3 };

			SetDeleteFilterByKeyCallback([&filterVector](HANDLE handle, const GUID* filterKey)
			{
				for (size_t i = 0; i < filterVector.size(); i++)
				{
					auto filter = filterVector.at(i);
					if (filterKey != nullptr && filter->filterKey == *filterKey)
					{
						filterVector.erase(filterVector.begin() + i);
						return NO_ERROR;
					}
				}

				return NO_ERROR;
			});

			engine.RemoveAllFilters(true);

			// We expect that filter1, filter2 will be removed,
			// but not filter3 since filter3's providerKey is null
			Assert::IsTrue(filterVector.size() == 1);
			Assert::IsTrue(filterVector[0] == &filter3);
		}
	};
}