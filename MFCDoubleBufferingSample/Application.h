#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'pch.h' をインクルードしてください"
#endif

class Application : public CWinApp
{
public:
	Application() noexcept;
	virtual BOOL InitInstance() override;

	DECLARE_MESSAGE_MAP()

private:
	static Application application;
};
