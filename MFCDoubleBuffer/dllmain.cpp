// dllmain.cpp : MFC 拡張 DLL の初期化と終了処理

#include "pch.h"
#include "framework.h"
#include <afxwin.h>
#include <afxdllx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace {
	// この DLL のモジュール情報 (実行時クラス情報やリソースを、利用側のアプリケーションと共有するために使う)
	AFX_EXTENSION_MODULE MFCDoubleBufferDLL{};
}

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, [[maybe_unused]] LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		TRACE0("MFCDoubleBuffer.DLL を初期化しています。\n");

		if (!AfxInitExtensionModule(MFCDoubleBufferDLL, hInstance))
			return FALSE;
		// この DLL を MFC のリソース検索チェーンに登録する (オブジェクトは MFC が終了時に削除する)
		new CDynLinkLibrary(MFCDoubleBufferDLL);
		break;

	case DLL_PROCESS_DETACH:
		TRACE0("MFCDoubleBuffer.DLL を終了しています。\n");
		AfxTermExtensionModule(MFCDoubleBufferDLL);
		break;
	}
	return TRUE;
}
