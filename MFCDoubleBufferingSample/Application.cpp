// Application.cpp : サンプル アプリケーションの初期化

#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "Application.h"
#include "Document.h"
#include "View.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 「新規作成」「開く」は CWinApp の標準の処理を使う
// (「上書き保存」などはドキュメント側の標準の処理が使われる)
BEGIN_MESSAGE_MAP(Application, CWinApp)
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
END_MESSAGE_MAP()

Application::Application() noexcept
{ SetAppID(_T("MFCDoubleBufferingSample.AppID.NoVersion")); }

BOOL Application::InitInstance()
{
	CWinApp::InitInstance();

	EnableTaskbarInteraction(FALSE);

	// ドキュメント・フレーム・ビューの組み合わせを登録する
	// フレームは標準の CFrameWnd をそのまま使う
	AddDocTemplate(new CSingleDocTemplate(IDR_MAINFRAME, RUNTIME_CLASS(Document), RUNTIME_CLASS(CFrameWnd), RUNTIME_CLASS(View)));

	// コマンド ラインの指定 (ファイル名など) に従って、新規作成またはファイルを開く
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

Application Application::application;
