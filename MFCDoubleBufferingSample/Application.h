// Application.h : サンプル アプリケーション

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'pch.h' をインクルードしてください"
#endif

// SDI (単一ドキュメント) のアプリケーション
class Application : public CWinApp
{
public:
	Application() noexcept;
	BOOL InitInstance() override;

	DECLARE_MESSAGE_MAP()

private:
	static Application application; // 唯一のアプリケーション オブジェクト (MFC の theApp に当たる)
};
