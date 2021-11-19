#include "pch.h"
#include "framework.h"
#include <afxwin.h>
#include <afxdllx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE MFCDoubleBufferDLL = { false, nullptr };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH) {
		TRACE0("MFCDoubleBuffer.DLL を初期化しています。\n");

		if (!AfxInitExtensionModule(MFCDoubleBufferDLL, hInstance))
			return 0;
		new CDynLinkLibrary(MFCDoubleBufferDLL);

	} else if (dwReason == DLL_PROCESS_DETACH) {
		TRACE0("MFCDoubleBuffer.DLL を終了しています。\n");
		AfxTermExtensionModule(MFCDoubleBufferDLL);
	}
	return 1;
}
