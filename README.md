# WFPTool
A `C++` wrapper library around `Windows WFP` (WFP stands for `Windows Filtering Platform`) API which greatly simplifies the usage of the tedious `WFP` API

For `C++` to call into the library, please refer to `src/WfpTool/FilterApiProvider.h`, `src/WfpTool/FilterApiProvider.cpp` and related tests in `src/WfpTool.Tests/FilterApiProviderTests.cpp`.

For other languages to call into the library, please refer to `src/WfpTool/FilterApiExport.h`, `src/WfpTool/FilterApiExport.cpp` and `src/WfpTool.Tests/FilterApiExportTests.cpp` for a `C` style API.

The `src/WfpTool.Api` project will build a dll for other languages to call into. 

BTW, for `WfpTool` project and `WfpTool.Api.Integrate` to run successfully, `Visual Studio` has to run under admin user
