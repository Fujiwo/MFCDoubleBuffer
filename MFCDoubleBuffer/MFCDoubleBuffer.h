#pragma once

namespace ShosMFCDoubleBuffer {

#ifdef EXPORTING
#define CLASS_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC __declspec(dllimport)
#endif

class CLASS_DECLSPEC GdiObjectSelector
{
    CDC& dc;
    CGdiObject* oldGdiObject;

public:
    GdiObjectSelector(CDC& dc, CGdiObject& gdiObject) : dc(dc), oldGdiObject(dc.SelectObject(&gdiObject))
    {}

    virtual ~GdiObjectSelector()
    { dc.SelectObject(oldGdiObject); }
};

class CLASS_DECLSPEC DoubleBuffer
{
    bool     isCashValid;
    CBitmap  cash;
    COLORREF backgroundColor;
    CView&   view;

public:
    DoubleBuffer(CView& view) : isCashValid(false), backgroundColor(::GetSysColor(COLOR_WINDOW)), view(view)
    {}

    CView& Window()
    {
        ASSERT_VALID(&view);
        ASSERT(view.GetSafeHwnd() != nullptr && ::IsWindow(view.GetSafeHwnd()));
        return view;
    }

protected:
    class CashDevice
    {
        CDC                      memoryDC;
        const GdiObjectSelector* selector;

    public:
        CashDevice(DoubleBuffer& doubleBuffer)
        {
            CClientDC clientDC(&doubleBuffer.Window());
            memoryDC.CreateCompatibleDC(&clientDC);
            selector = new GdiObjectSelector(memoryDC, doubleBuffer.cash);
        }

        virtual ~CashDevice()
        { delete selector; }

        operator CDC& ()
        { return memoryDC; }
    };

    void SetBackgroundColor(COLORREF backgroundColor)
    { this->backgroundColor = backgroundColor; }

    void Update()
    { isCashValid = false; }

    void Draw(CDC& dc)
    {
        DrawCash(dc);
        view.OnPrepareDC(&dc);
        OnDrawLayer2(dc);
    }

    virtual void OnDrawLayer1(CDC& dc) {}
    virtual void OnDrawLayer2(CDC& dc) {}

private:
    void DrawCash(CDC& dc)
    {
        CRect clientRect;
        view.GetClientRect(&clientRect);

        CDC   memoryDC;
        memoryDC.CreateCompatibleDC(&dc);

        if (!isCashValid) {
            cash.DeleteObject();
            cash.CreateCompatibleBitmap(&dc, clientRect.Width(), clientRect.Height());
        }

        GdiObjectSelector selector(memoryDC, cash);

        if (!isCashValid) {
            memoryDC.FillSolidRect(&clientRect, backgroundColor);
            auto savedDC = memoryDC.SaveDC();
            view.OnPrepareDC(&memoryDC);
            OnDrawLayer1(memoryDC);
            memoryDC.RestoreDC(savedDC);
            isCashValid = true;
        }

        CRect clipBox;
        dc.GetClipBox(&clipBox);
        dc.BitBlt(clipBox.left, clipBox.top, clipBox.Width(), clipBox.Height(), &memoryDC, clipBox.left, clipBox.top, SRCCOPY);
    }
};

template <class TView>
class CLASS_DECLSPEC DoubleBufferViewBase : public TView, protected DoubleBuffer
{
public:
    DoubleBufferViewBase() : DoubleBuffer(*(TView*)this)
    {}

protected:
    virtual void OnPaint(CDC& dc)
    { Draw(dc); }
    
    virtual void OnDraw(CDC* pDC) override
    { ASSERT(false); }

    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override
    {
        Update();
        TView::OnUpdate(pSender, lHint, pHint);
    }

    afx_msg void OnPaint()
    {
        CPaintDC dc(this);
        OnPaint(dc);
    }

    afx_msg BOOL OnEraseBkgnd(CDC* pDC)
    {
        //return TView::OnEraseBkgnd(pDC);
        return true;
    }

    afx_msg void OnSize(UINT nType, int cx, int cy)
    {
        TView::OnSize(nType, cx, cy);
        Update();
    }
};

class CLASS_DECLSPEC DoubleBufferView : public DoubleBufferViewBase<CView>
{
    DECLARE_DYNCREATE(DoubleBufferView)
    DECLARE_MESSAGE_MAP()
};

class CLASS_DECLSPEC  DoubleBufferScrollView : public DoubleBufferViewBase<CScrollView>
{
protected:
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
    {
        DoubleBufferViewBase<CScrollView>::OnHScroll(nSBCode, nPos, pScrollBar);
        Update();
        Invalidate();
    }

    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
    {
        DoubleBufferViewBase<CScrollView>::OnVScroll(nSBCode, nPos, pScrollBar);
        Update();
        Invalidate();
    }

    afx_msg BOOL OnMouseWheel(UINT nFlags, short delta, CPoint point)
    {
        auto result = DoubleBufferViewBase<CScrollView>::OnMouseWheel(nFlags, delta, point);
        Update();
        Invalidate();
        return result;
    }

    DECLARE_DYNCREATE(DoubleBufferScrollView)
    DECLARE_MESSAGE_MAP()
};

} // namespace ShosMFCDoubleBuffer
