#include "pch.h"
#include "framework.h"
#include "MFCDoubleBuffer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(DoubleBufferView, DoubleBufferViewBase<CView>)

BEGIN_MESSAGE_MAP(DoubleBufferView, DoubleBufferViewBase<CView>)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(DoubleBufferScrollView, DoubleBufferViewBase<CScrollView>)

BEGIN_MESSAGE_MAP(DoubleBufferScrollView, DoubleBufferViewBase<CScrollView>)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()
