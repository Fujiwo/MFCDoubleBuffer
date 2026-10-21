// View.h : サンプルのビュー (ライブラリの使い方の例)
//
//   - クリックした位置に円を配置する       → Layer 1 (キャッシュ) に描き足す
//   - マウス カーソルの位置に円を表示する   → Layer 2 (毎回描く層) に描く

#pragma once
#include "..\include\MFCDoubleBuffer.h"

class View : public Shos::MFCDoubleBuffer::DoubleBufferScrollView
{
	DECLARE_DYNCREATE(View)

	using GdiObjectSelector = Shos::MFCDoubleBuffer::GdiObjectSelector;

	static constexpr int      radius     = 30;                     // 円の半径 (論理単位)
	static constexpr COLORREF penColor   = RGB(0x20, 0x10, 0x00); // 円の輪郭の色
	static constexpr COLORREF brushColor = RGB(0x00, 0x20, 0xff); // 円の塗りつぶしの色

public:
	View()
	{ SetBackgroundColor(RGB(0xff, 0xf8, 0xf0)); }

protected:
	// Layer 1: 配置済みの図形をすべて描く (キャッシュを作り直すときだけ呼ばれる)
	void OnDrawLayer1(CDC& dc) override
	{
		for (const auto figure : GetDocument()->GetFigures())
			DrawFigure(dc, figure);
	}

	// Layer 2: マウス カーソルの位置に円を描く (再描画のたびに呼ばれる)
	void OnDrawLayer2(CDC& dc) override
	{ DrawFigure(dc, GetDocument()->GetRubberBand()); }

	void OnInitialUpdate() override
	{
		DoubleBufferScrollView::OnInitialUpdate();
		SetScrollSizes(MM_TEXT, GetDocument()->GetSize());
	}

	// クリックした位置に図形を追加する
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point)
	{
		DoubleBufferScrollView::OnLButtonUp(nFlags, point);

		DPtoLP(point);
		GetDocument()->AddFigure(point);

		// キャッシュ全体を作り直す (Update()) 代わりに、追加した図形だけを描き足し、
		// その範囲だけを画面に反映する
		DrawFigure(CacheDevice(*this), point);
		InvalidateFigure(point);
	}

	// ラバーバンドをマウス カーソルに追従させる
	afx_msg void OnMouseMove(UINT nFlags, CPoint point)
	{
		DoubleBufferScrollView::OnMouseMove(nFlags, point);

		DPtoLP(point);
		// 移動前と移動後の円の範囲だけを再描画する (Layer 1 はキャッシュから転送されるので、描き直しは不要)
		InvalidateFigure(GetDocument()->GetRubberBand());
		GetDocument()->SetRubberBand(point);
		InvalidateFigure(point);
	}

private:
	[[nodiscard]] Document* GetDocument() const
	{
		ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(Document)));
		return static_cast<Document*>(m_pDocument);
	}

	// 図形 (中心 figure、半径 radius の円) を囲む矩形 (論理座標)
	[[nodiscard]] static CRect GetFigureArea(CPoint figure)
	{
		CRect area(figure, figure);
		area.InflateRect(radius, radius);
		return area;
	}

	static void DrawFigure(CDC& dc, CPoint figure)
	{
		CPen              pen(PS_SOLID, 0, penColor);
		GdiObjectSelector penSelector(dc, pen);

		CBrush            brush(brushColor);
		GdiObjectSelector brushSelector(dc, brush);

		const auto area = GetFigureArea(figure);
		dc.Ellipse(&area);
	}

	// 図形の範囲を再描画させる
	void InvalidateFigure(CPoint figure)
	{
		auto area = GetFigureArea(figure);
		LPtoDP(area);                 // InvalidateRect はデバイス座標で指定する
		area.InflateRect(1, 1);       // 輪郭の線が欠けないよう、少し広げる
		InvalidateRect(&area, FALSE); // 背景の消去は不要 (OnEraseBkgnd では何もしない)
	}

	// デバイス座標 (マウスの位置など) を論理座標 (スクロール位置を考慮した座標) に変換する
	void DPtoLP(CPoint& point)
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.DPtoLP(&point);
	}

	// 論理座標をデバイス座標に変換する
	void LPtoDP(CRect& rect)
	{
		CClientDC dc(this);
		OnPrepareDC(&dc);
		dc.LPtoDP(&rect);
	}

	DECLARE_MESSAGE_MAP()
};
