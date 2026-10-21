// View.cpp : サンプルのビューの実行時クラス情報とメッセージ マップの定義
// 実装は View.h にある

#include "pch.h"
#include "framework.h"
#include "Document.h"
#include "View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using Shos::MFCDoubleBuffer::DoubleBufferScrollView;

IMPLEMENT_DYNCREATE(View, DoubleBufferScrollView)

// WM_PAINT などは基底クラス (DoubleBufferScrollView) で処理するので、ここではマウスの処理だけを登録する
BEGIN_MESSAGE_MAP(View, DoubleBufferScrollView)
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()
