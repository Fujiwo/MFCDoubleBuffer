# [C++/MFC] MFCDoubleBuffer
Double Buffering Library for MFC Applications

for Visual Studio 2026 (Platform Toolset v145, C++20)

## 概要

`CView` / `CScrollView` に 2 層構成のダブルバッファリングを追加する MFC 拡張 DLL です。

| 層 | 描画メソッド | 内容 |
|---|---|---|
| Layer 1 | `OnDrawLayer1(CDC&)` | ビットマップにキャッシュされる。`Update()` を呼ぶまで再描画されない |
| Layer 2 | `OnDrawLayer2(CDC&)` | 毎回描画される (ラバーバンドなど)。画面外で合成してから転送する |

## 使い方

1. ビューの基底クラスを `Shos::MFCDoubleBuffer::DoubleBufferView` または `DoubleBufferScrollView` にする
2. `OnDrawLayer1` / `OnDrawLayer2` をオーバーライドする (`OnDraw` は使わない)
3. Layer 1 の内容が変わったら
   - 全体を描き直す: `Update()` → `Invalidate(FALSE)` (`OnUpdate` / `OnSize` / スクロール時は自動)
   - 差分だけを描き足す: `CacheDevice` に描画 → `InvalidateRect(..., FALSE)`
4. 背景色は `SetBackgroundColor()` で変更する

```cpp
class View : public Shos::MFCDoubleBuffer::DoubleBufferScrollView
{
protected:
    void OnDrawLayer1(CDC& dc) override { /* 固定的な内容 */ }
    void OnDrawLayer2(CDC& dc) override { /* 頻繁に変わる内容 */ }

    afx_msg void OnLButtonUp(UINT nFlags, CPoint point)
    {
        // ... ドキュメントに追加 ...
        DrawFigure(CacheDevice(*this), logicalPoint); // キャッシュに差分を描画 (論理座標)
        InvalidateRect(&deviceArea, FALSE);
    }
};
```

詳しくは `MFCDoubleBufferingSample` を参照してください。

## ビルド

`MFCDoubleBuffer.sln` をビルドします。ポストビルドで、ヘッダーが `include/` に、インポート ライブラリが `lib/` にコピーされます。
`lib/` は Debug と Release で共用です (最後にビルドした構成・プラットフォームのものが入ります)。

## 制限事項

- 印刷・印刷プレビューには対応していません。
