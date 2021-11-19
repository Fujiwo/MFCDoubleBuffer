#pragma once
#include "..\include\MFCDoubleBuffer.h"

class View : public DoubleBufferScrollView
{
	DECLARE_DYNCREATE(View)

public:
	View()
	{ SetBackgroundColor(RGB(0xff, 0xf8, 0xf0)); }

protected:
	virtual void OnDrawLayer1(CDC& dc) override
	{
		for (auto figure : *GetDocument())
			Draw(dc, figure);
	}

	virtual void OnDrawLayer2(CDC& dc) override
	{ Draw(dc, GetDocument()->GetRubberBand()); }

	virtual void OnInitialUpdate()
	{
		DoubleBufferScrollView::OnInitialUpdate();
		SetScrollSizes(MM_TEXT, GetDocument()->GetSize());
	}

	afx_msg void OnLButtonUp(UINT nFlags, CPoint point)
	{
		DoubleBufferScrollView::OnLButtonUp(nFlags, point);

		DPtoLP(point);
		GetDocument()->push_back(point);

		Draw(CashDevice(*this), point);
		Update();
	}

	afx_msg void OnMouseMove(UINT nFlags, CPoint point)
	{
		DoubleBufferScrollView::OnMouseMove(nFlags, point);

		DPtoLP(point);
		GetDocument()->SetRubberBand(point);
		Invalidate();
	}

private:
	Document* GetDocument() const
	{
#ifdef _DEBUG
		ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(Document)));
		return (Document*)m_pDocument;
#else
		return reinterpret_cast<Document*>(m_pDocument);
#endif
	}

	static void Draw(CDC& dc, CPoint figure)
	{
		const auto radius = 30;
		CRect area(figure.x - radius, figure.y - radius, figure.x + radius, figure.y + radius);

		const auto penColor = RGB(0x20, 0x10, 0x00);
		CPen       pen(PS_SOLID, 0, penColor);
		GdiObjectSelector penSelector(dc, pen);

		const auto brushColor = RGB(0x00, 0x20, 0xff);
		CBrush     brush;
		brush.CreateSolidBrush(brushColor);
		GdiObjectSelector brushSelector(dc, brush);

		dc.Ellipse(&area);
	}

	void DPtoLP(CPoint& point)
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.DPtoLP(&point);
	}

	DECLARE_MESSAGE_MAP()
};