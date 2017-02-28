// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

#pragma comment(lib,"rpcrt4.lib")
#pragma comment(lib,"fwpuclnt.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"IPHlpApi.lib")

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ulReasonForCall,
                       LPVOID lpReserved
					 )
{
	switch (ulReasonForCall)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

