// MFCDoubleBuffer.cpp : 実行時クラス情報とメッセージ マップの定義
// クラスの実装はすべて MFCDoubleBuffer.h にある

#include "pch.h"
#include "framework.h"
#include "MFCDoubleBuffer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace Shos::MFCDoubleBuffer {

// DoubleBufferViewBase はテンプレートで、メッセージ マップを持てないため、
// 共通のハンドラー (OnPaint / OnEraseBkgnd / OnSize) を、それぞれの派生クラスのマップに登録している

IMPLEMENT_DYNCREATE(DoubleBufferView, DoubleBufferViewBase<CView>)

BEGIN_MESSAGE_MAP(DoubleBufferView, DoubleBufferViewBase<CView>)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(DoubleBufferScrollView, DoubleBufferViewBase<CScrollView>)

// スクロールの処理は OnScrollBy (仮想関数) で行うので、WM_HSCROLL などの登録は不要
BEGIN_MESSAGE_MAP(DoubleBufferScrollView, DoubleBufferViewBase<CScrollView>)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
END_MESSAGE_MAP()

} // namespace Shos::MFCDoubleBuffer
