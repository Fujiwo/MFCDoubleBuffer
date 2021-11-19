#include "pch.h"
#include "framework.h"
#include "Document.h"
#include "View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(View, DoubleBufferScrollView)

BEGIN_MESSAGE_MAP(View, DoubleBufferScrollView)
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()
