// MFCDoubleBuffer.h : MFC のビュー向けダブルバッファリング ライブラリ
//
// ■ 仕組み
//   ビューの描画内容を 2 つの層に分けて扱う。
//
//   - Layer 1 (OnDrawLayer1) : あまり変化しない内容 (確定した図形など)
//       クライアント領域と同じ大きさのビットマップ (キャッシュ) に描いて保持する。
//       Update() でキャッシュを無効にするまで、再描画されない。
//   - Layer 2 (OnDrawLayer2) : 頻繁に変化する内容 (ラバーバンド、カーソル位置の表示など)
//       WM_PAINT のたびに、キャッシュの上に重ねて描く。
//
//   WM_PAINT では、次の手順で画面に表示する。
//     1. キャッシュが無効なら、背景色で塗りつぶしてから Layer 1 を描き、キャッシュを作り直す
//     2. 作業用ビットマップにキャッシュをコピーし、その上に Layer 2 を描く
//     3. 作業用ビットマップを画面に一度だけ転送する (途中経過が見えないので、ちらつかない)
//
// ■ 使い方
//   ビューの基底クラスを DoubleBufferView (CView 版) または DoubleBufferScrollView (CScrollView 版) にし、
//   OnDraw の代わりに OnDrawLayer1 / OnDrawLayer2 をオーバーライドする。
//
// ■ 制限事項
//   印刷・印刷プレビューには対応していない。

#pragma once

#include <concepts>
#include <optional>

namespace Shos::MFCDoubleBuffer {

// GDI オブジェクト (ペン、ブラシ、ビットマップなど) を DC に選択し、
// スコープを抜けるときに元のオブジェクトへ自動的に戻す RAII クラス
//
//   例:
//     CPen pen(PS_SOLID, 0, RGB(0, 0, 0));
//     GdiObjectSelector selector(dc, pen);  // ここで pen を選択
//     dc.Ellipse(&area);
//                                          // スコープを抜けると元のペンに戻る
class AFX_EXT_CLASS GdiObjectSelector final
{
    CDC&        dc;
    CGdiObject* oldGdiObject; // 選択前に DC に選択されていたオブジェクト (戻す先)

public:
    GdiObjectSelector(CDC& dc, CGdiObject& gdiObject) : dc(dc), oldGdiObject(dc.SelectObject(&gdiObject))
    {}

    ~GdiObjectSelector()
    {
        // 選択に失敗していた (nullptr が返された) 場合は、戻すものがない
        if (oldGdiObject != nullptr)
            dc.SelectObject(oldGdiObject);
    }

    // コピーすると、元のオブジェクトへ 2 回戻すことになるため禁止
    GdiObjectSelector(const GdiObjectSelector&)            = delete;
    GdiObjectSelector& operator=(const GdiObjectSelector&) = delete;
};

// ダブルバッファリングの本体
// ビュー クラスと多重継承して使う (直接使わず、DoubleBufferView / DoubleBufferScrollView を使う)
class AFX_EXT_CLASS DoubleBuffer
{
    bool     isCacheValid    = false;                       // キャッシュの内容が最新かどうか
    CBitmap  cache;                                         // Layer 1 を描いたビットマップ (クライアント領域の大きさ)
    CPoint   cacheOrigin;                                   // キャッシュを作ったときのスクロール位置 (デバイス座標)
    COLORREF backgroundColor = ::GetSysColor(COLOR_WINDOW); // Layer 1 を描く前に塗りつぶす色
    CView&   view;                                          // 描画対象のビュー (このオブジェクト自身の派生先)

public:
    explicit DoubleBuffer(CView& view) : view(view)
    {}

    // 描画対象のビューを返す (ウィンドウが作成済みであることを、デバッグ時に確認する)
    [[nodiscard]] CView& Window()
    {
        ASSERT_VALID(&view);
        ASSERT(view.GetSafeHwnd() != nullptr && ::IsWindow(view.GetSafeHwnd()));
        return view;
    }

protected:
    // キャッシュ (Layer 1) に直接描き足すための DC
    //
    // Layer 1 の内容が少しだけ増えたとき (図形を 1 つ追加したときなど) に、
    // Update() でキャッシュ全体を作り直す代わりに、差分だけを描き足すために使う。
    // 描き足した後は、該当する範囲を InvalidateRect(..., FALSE) で再描画させること。
    //
    // OnPrepareDC 適用済みなので、論理座標 (スクロール位置を考慮した座標) で描画できる。
    // キャッシュが無効なとき (Update() の後など) に描いた内容は、次にキャッシュを作り直すときに捨てられる。
    //
    //   例:
    //     DrawFigure(CacheDevice(*this), point);  // 一時オブジェクトとして使い、文の終わりで後始末される
    class CacheDevice final
    {
        CView&                           view;
        CDC                              memoryDC;    // キャッシュ用ビットマップを選択したメモリ DC
        std::optional<GdiObjectSelector> selector;    // memoryDC を作ってから選択するため、後から構築する
        int                              savedDC = 0; // OnPrepareDC 適用前の DC の状態

    public:
        explicit CacheDevice(DoubleBuffer& doubleBuffer) : view(doubleBuffer.Window())
        {
            CClientDC clientDC(&view);
            memoryDC.CreateCompatibleDC(&clientDC);
            selector.emplace(memoryDC, doubleBuffer.cache);

            // スクロール位置やマッピング モードを、画面と同じように設定する
            savedDC = memoryDC.SaveDC();
            view.OnPrepareDC(&memoryDC);
        }

        ~CacheDevice()
        { memoryDC.RestoreDC(savedDC); }

        // DC やビットマップの選択を二重に解放しないよう、コピーは禁止
        CacheDevice(const CacheDevice&)            = delete;
        CacheDevice& operator=(const CacheDevice&) = delete;

        // CDC& を受け取る描画関数に、そのまま渡せるようにする
        operator CDC& ()
        { return memoryDC; }
    };

    // 旧名 (綴りの誤り)。互換性のために残している
    using CashDevice [[deprecated("CacheDevice を使用してください")]] = CacheDevice;

    // 背景色を変更する (キャッシュは次の描画時に作り直される)
    void SetBackgroundColor(COLORREF backgroundColor)
    {
        this->backgroundColor = backgroundColor;
        Update();
    }

    // キャッシュを無効にする
    // Layer 1 の内容が変わったときに呼ぶ。実際に作り直すのは次の WM_PAINT のとき。
    // 画面にすぐ反映するには、続けて Invalidate(FALSE) を呼ぶこと。
    void Update()
    { isCacheValid = false; }

    // WM_PAINT の DC (CPaintDC) に、Layer 1 と Layer 2 を重ねて描く
    void Draw(CDC& dc)
    {
        CRect clientRect;
        view.GetClientRect(&clientRect);
        if (clientRect.IsRectEmpty()) // 最小化時など、描く場所がない
            return;

        // 再描画が必要な範囲。OnPrepareDC 適用前なので、デバイス座標で得られる
        CRect clipBox;
        dc.GetClipBox(&clipBox);
        clipBox &= clientRect;
        if (clipBox.IsRectEmpty())
            return;

        // キャッシュはスクロール位置に依存している (表示中の部分だけを保持している) ため、
        // スクロール位置が変わっていたら作り直す。
        // 通常のスクロールは OnScrollBy で処理済みなので、ここに来るのは ScrollToPosition などを直接呼んだ場合。
        if (GetCacheOrigin() != cacheOrigin) {
            Update();
            // ScrollWindow で移動しただけの部分には古い Layer 2 が残っているため、全体を描き直させる
            if (clipBox != clientRect)
                view.Invalidate(FALSE);
        }
        if (!UpdateCache(dc, clientRect))
            return;

        CDC cacheDC;
        cacheDC.CreateCompatibleDC(&dc);
        GdiObjectSelector cacheSelector(cacheDC, cache);

        // 作業用ビットマップ (再描画が必要な範囲の大きさ) を用意する
        const auto [width, height] = clipBox.Size();
        CDC     workDC;
        CBitmap workBitmap;
        workDC.CreateCompatibleDC(&dc);
        if (!workBitmap.CreateCompatibleBitmap(&dc, width, height))
            return;
        GdiObjectSelector workSelector(workDC, workBitmap);

        // 1. キャッシュ (Layer 1) のうち、再描画が必要な範囲を作業用ビットマップにコピー
        workDC.BitBlt(0, 0, width, height, &cacheDC, clipBox.left, clipBox.top, SRCCOPY);

        // 2. その上に Layer 2 を描く
        //    作業用ビットマップの左上は clipBox の左上に当たるので、その分だけ原点をずらす
        const auto savedDC = workDC.SaveDC();
        view.OnPrepareDC(&workDC);
        workDC.OffsetViewportOrg(-clipBox.left, -clipBox.top);
        OnDrawLayer2(workDC);
        workDC.RestoreDC(savedDC);

        // 3. 合成した結果を画面へ一度に転送する
        dc.BitBlt(clipBox.left, clipBox.top, width, height, &workDC, 0, 0, SRCCOPY);
    }

    // キャッシュの内容が依存するスクロール位置 (デバイス座標) を返す
    // スクロールしないビューでは常に (0, 0)。DoubleBufferScrollView でオーバーライドしている。
    [[nodiscard]] virtual CPoint GetCacheOrigin() const
    { return {}; }

    // Layer 1 (キャッシュされる層) を描く。派生クラスでオーバーライドする
    // dc は OnPrepareDC 適用済みなので、論理座標で描画できる
    virtual void OnDrawLayer1(CDC&) {}

    // Layer 2 (毎回描かれる層) を描く。派生クラスでオーバーライドする
    // dc は OnPrepareDC 適用済みなので、論理座標で描画できる
    virtual void OnDrawLayer2(CDC&) {}

private:
    // キャッシュが無効なら作り直す
    // 戻り値: キャッシュが使える状態なら true (ビットマップを作れなかったときは false)
    [[nodiscard]] bool UpdateCache(CDC& dc, const CRect& clientRect)
    {
        if (isCacheValid)
            return true;

        // クライアント領域の大きさは変わっている可能性があるので、ビットマップごと作り直す
        cache.DeleteObject();
        if (!cache.CreateCompatibleBitmap(&dc, clientRect.Width(), clientRect.Height()))
            return false;

        CDC memoryDC;
        memoryDC.CreateCompatibleDC(&dc);
        GdiObjectSelector selector(memoryDC, cache);

        // 背景を塗りつぶす (デバイス座標で行うため、OnPrepareDC の前に行う)
        memoryDC.FillSolidRect(&clientRect, backgroundColor);

        // Layer 1 を論理座標で描く
        const auto savedDC = memoryDC.SaveDC();
        view.OnPrepareDC(&memoryDC);
        OnDrawLayer1(memoryDC);
        memoryDC.RestoreDC(savedDC);

        cacheOrigin  = GetCacheOrigin();
        isCacheValid = true;
        return true;
    }
};

// ビュー クラス (CView またはその派生クラス) に、ダブルバッファリングを組み込むための共通の基底クラス
// MFC のメッセージ マップはテンプレートでは定義できないため、
// メッセージ ハンドラーの実装だけをここに置き、マップは派生クラス (MFCDoubleBuffer.cpp) で定義している。
template <std::derived_from<CView> TView>
class DoubleBufferViewBase : public TView, protected DoubleBuffer
{
public:
    // 基底クラスは宣言順 (TView → DoubleBuffer) に構築されるので、
    // DoubleBuffer の構築時には TView 部分の構築が済んでいる
    DoubleBufferViewBase() : DoubleBuffer(static_cast<TView&>(*this))
    {}

protected:
    // WM_PAINT 時の描画処理
    // 描画の前後に処理を追加したい場合は、これをオーバーライドする
    virtual void OnPaintBuffered(CDC& dc)
    { Draw(dc); }

    // 描画は OnPaint → Draw で行うので、OnDraw は使わない
    // (CView の純粋仮想関数なので定義だけ必要。印刷は非サポート)
    void OnDraw(CDC*) override
    { ASSERT(false); }

    // ドキュメントが更新されたら (UpdateAllViews など)、Layer 1 を作り直す
    void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override
    {
        Update();
        TView::OnUpdate(pSender, lHint, pHint);
    }

    afx_msg void OnPaint()
    {
        CPaintDC dc(this);
        OnPaintBuffered(dc);
    }

    // 背景は Draw の中で描くので、ここでは消去しない (消去するとちらつきの原因になる)
    afx_msg BOOL OnEraseBkgnd(CDC*)
    { return TRUE; }

    // クライアント領域の大きさが変わったら、キャッシュのビットマップを作り直す
    afx_msg void OnSize(UINT nType, int cx, int cy)
    {
        TView::OnSize(nType, cx, cy);
        Update();
    }
};

// ダブルバッファリング付きの CView
class AFX_EXT_CLASS DoubleBufferView : public DoubleBufferViewBase<CView>
{
    DECLARE_DYNCREATE(DoubleBufferView)
    DECLARE_MESSAGE_MAP()
};

// ダブルバッファリング付きの CScrollView
// キャッシュは表示中の部分だけを保持するので、スクロールするたびに作り直す。
class AFX_EXT_CLASS DoubleBufferScrollView : public DoubleBufferViewBase<CScrollView>
{
protected:
    // スクロール バーやマウス ホイールによるスクロールは、すべてここを通る
    BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE) override
    {
        const auto result = DoubleBufferViewBase::OnScrollBy(sizeScroll, bDoScroll);
        // 実際にスクロールしたときだけ、キャッシュを作り直して全体を描き直す
        if (result && bDoScroll) {
            Update();
            Invalidate(FALSE);
        }
        return result;
    }

    // キャッシュは現在のスクロール位置を基準に作られる
    [[nodiscard]] CPoint GetCacheOrigin() const override
    { return GetDeviceScrollPosition(); }

    DECLARE_DYNCREATE(DoubleBufferScrollView)
    DECLARE_MESSAGE_MAP()
};

} // namespace Shos::MFCDoubleBuffer
