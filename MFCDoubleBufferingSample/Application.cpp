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

	auto pDocTemplate = new CSingleDocTemplate(IDR_MAINFRAME, RUNTIME_CLASS(Document), RUNTIME_CLASS(CFrameWnd), RUNTIME_CLASS(View));
	if (pDocTemplate == nullptr)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

Application Application::application;
