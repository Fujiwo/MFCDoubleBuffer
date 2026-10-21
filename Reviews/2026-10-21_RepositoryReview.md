# MFCDoubleBuffer リポジトリ全体レビュー

- レビュー日: 2026-10-21
- 対象: `main` ブランチ (HEAD = `81ec6e8`) と作業ツリー
- 対象ファイル: ライブラリ (`MFCDoubleBuffer/`, `include/`, `lib/`)、サンプル (`MFCDoubleBufferingSample/`)、ソリューション/プロジェクト設定、ドキュメント類

> 注: 本レビューは静的な読解によるものです。Windows/MSVC でのビルドや実行による確認は行っていません。

---

## 1. 概要

MFC の `CView` / `CScrollView` に 2 層構成のダブルバッファリングを追加する、ヘッダー中心の小さな MFC 拡張 DLL です。

- **Layer 1**: `OnDrawLayer1()` で描画し、クライアント領域サイズのビットマップにキャッシュする
- **Layer 2**: `OnDrawLayer2()` で毎回、画面 DC へ直接描画する (ラバーバンドなど)

設計の考え方は分かりやすく、サンプルも短くて読みやすいです。ただし次の問題があります。

- 印刷が動かない
- スクロール時にキャッシュが古いまま残る経路がある
- ビルド構成 (lib の置き場所) が壊れやすい
- サンプルに不具合がある

---

## 2. 重要度別サマリー

| # | 重要度 | 区分 | 概要 |
|---|---|---|---|
| 1 | 🔴 高 | 作業ツリー | `LICENSE` など 4 ファイルが NUL バイトで上書きされ、破損している |
| 2 | 🔴 高 | ライブラリ | `OnDraw()` が `ASSERT(false)` なので、印刷と印刷プレビューが動かない |
| 3 | 🔴 高 | ビルド | `lib/` が構成・プラットフォーム共通で 1 つしかない。Win32 では更新されず、Debug/Release も混ざる |
| 4 | 🟠 中 | ライブラリ | 一部のスクロール経路でキャッシュが無効化されない (キーボード、`ScrollToPosition` など) |
| 5 | 🟠 中 | ライブラリ | `CashDevice` が `OnPrepareDC` を適用しないため、スクロール中は座標がずれる |
| 6 | 🟠 中 | ライブラリ | `GdiObjectSelector` / `CashDevice` がコピー可能で、二重解放・二重復元の恐れがある |
| 7 | 🟠 中 | ライブラリ | Layer 2 を画面 DC に直接描くため、オーバーレイ部分がちらつく可能性がある |
| 8 | 🟠 中 | サンプル | `OnLButtonUp` の差分描画が直後の `Update()` で無駄になり、`Invalidate()` もない |
| 9 | 🟠 中 | サンプル | `DeleteContents()` を実装していないため、SDI で「新規作成」しても図形が残る |
| 10 | 🟡 低 | ライブラリ | 汎用すぎるマクロ名 (`EXPORTING`, `CLASS_DECLSPEC`)。`AFX_EXT_CLASS` を使うべき |
| 11 | 🟡 低 | ライブラリ | 綴りの誤り: `Cash` → `Cache` |
| 12 | 🟡 低 | ライブラリ | サイズ 0 (最小化時) のビットマップ作成失敗を考慮していない |
| 13 | 🟡 低 | 全般 | README が少なく、内容も古い (VS2019 と書かれているが、実際のツールセットは v145) |
| 14 | 🟡 低 | 全般 | 不要なファイル (`MFCDoubleBuffer.old/`, 中身がすべてコメントの `MainFrame.cpp`) と、二重管理のヘッダー |

---

## 3. 詳細

### 3.1 🔴 作業ツリーのファイル破損

次の 4 ファイルは `git status` で変更ありと表示されます。中身を確認すると、**すべて NUL (`\0`) バイトだけ**になっています。

| ファイル | HEAD のサイズ | 作業ツリー |
|---|---|---|
| `LICENSE` | 1069 B (MIT License) | 1090 B、すべて NUL |
| `MFCDoubleBuffer.old/MFCDoubleBuffer.vcxproj` | 11545 B | 11779 B、すべて NUL |
| `MFCDoubleBuffer.old/MFCDoubleBuffer.vcxproj.filters` | 2143 B | 2200 B、すべて NUL |
| `MFCDoubleBufferingSample/MainFrame.cpp` | 1360 B | 1427 B、すべて NUL |

Dropbox の同期と WSL/Windows の併用など、外部の要因でファイルが壊れたと考えられます。**誤ってコミットしないよう注意してください。**

- 復旧方法: `git restore LICENSE MFCDoubleBuffer.old MFCDoubleBufferingSample/MainFrame.cpp`
- 中長期の対策: Git リポジトリを Dropbox の同期対象から外すことを推奨します。

### 3.2 🔴 印刷と印刷プレビューが動かない (`MFCDoubleBuffer.h:122`)

```cpp
virtual void OnDraw(CDC* pDC) override
{ ASSERT(false); }
```

`CView::OnPrint()` は `OnDraw()` を呼びます。そのため、このライブラリを使うビューで印刷やプレビューを行うと、次のようになります。

- Debug ビルド: アサーションで止まる
- Release ビルド: 白紙が出力される

サンプルには印刷メニューがないので表面化しませんが、汎用ライブラリとしては致命的です。

**提案**: `OnDraw()` ではキャッシュを使わずに、2 つの層を直接描きます。

```cpp
virtual void OnDraw(CDC* pDC) override
{
    ASSERT_VALID(pDC);
    OnDrawLayer1(*pDC);
    OnDrawLayer2(*pDC);
}
```

印刷時は CView が `OnPrepareDC` を適用した DC を渡すので、そのまま描画できます。

### 3.3 🔴 `lib/` の出力先と参照が構成に依存しない

- ポストビルドイベント (`xcopy ... "$(SolutionDir)\lib"`) は **Debug|x64 と Release|x64 にしか設定されていません** (`MFCDoubleBuffer.vcxproj:111-115`, `:193-197`)。
  - Win32 でビルドしても `lib/` は更新されません。
  - サンプルは古い x64 用 lib をリンクしようとし、`LNK1112` (マシンの種類の不一致) になります。
- サンプルは構成に関係なく `..\lib\MFCDoubleBuffer.lib` を参照しています (`MFCDoubleBufferingSample.vcxproj:221`)。
  - Debug と Release が同じファイルを上書きし合います。
  - Release の EXE が Debug の拡張 DLL の import lib をリンクすることがあります。MFC 拡張 DLL は Debug/Release の MFC ランタイムを混在できないため、実行時に問題が起きます。
- ビルド成果物 `lib/MFCDoubleBuffer.lib` がリポジトリにコミットされています。

**提案**
- ソリューション内では、`<ProjectReference>` でライブラリプロジェクトを参照する形に変えます。これで構成ごとに正しい lib が自動でリンクされます。
- 配布用の `lib/` が必要なら、`lib\$(Platform)\$(Configuration)\` のように分けます。ポストビルドは全構成に設定し、成果物は `.gitignore` に追加します。

### 3.4 🟠 スクロール時にキャッシュが無効化されない経路 (`MFCDoubleBuffer.h:156-183`)

キャッシュはクライアント領域サイズで、スクロール位置を反映した状態で描かれています。そのため、スクロールするたびに `Update()` を呼ぶ必要があります。現在は `OnHScroll` / `OnVScroll` / `OnMouseWheel` だけで呼んでいるため、次の経路ではキャッシュが古いまま残ります。

- `CScrollView::ScrollToPosition()` を直接呼んだとき (プログラムからのスクロール)
- `SetScrollSizes()` によってスクロール位置が補正されたとき
- `OnScrollBy()` を直接使う派生クラス (キーボードによるスクロールの実装など)

また、`OnMouseWheel` は実際にスクロールしなかった場合 (`result == FALSE`) でも `Update()` と `Invalidate()` を呼んでいます。

**提案**
- 仮想関数 `OnScrollBy(CSize, BOOL)` をオーバーライドし、スクロールが成功したときに `Update()` と `Invalidate(FALSE)` を呼びます。
  - こうすると、スクロールバー・ホイール・キーボードの経路を 1 か所でまとめて扱えます。
- 加えて、`OnPrepareDC` の時点でスクロール位置 (`GetDeviceScrollPosition()`) が前回と違えば無効化する、という方法もあります。この方法はより確実です。
- 根本的な対策として、キャッシュを「ドキュメント全体のサイズ」で持ち、BitBlt の転送元をスクロール位置に合わせてずらす方式も考えられます。メモリは多く使いますが、スクロールのたびに再描画しなくて済みます。

### 3.5 🟠 `CashDevice` にマッピング (`OnPrepareDC`) が適用されない (`MFCDoubleBuffer.h:44-62`)

`DrawCash()` は Layer 1 を描くときに `view.OnPrepareDC(&memoryDC)` を適用しています。一方、`CashDevice` が返す DC には何も適用されていません。

サンプルの `OnLButtonUp` では、`DPtoLP` で論理座標に変換した点を `CashDevice` に描いています。このため、**スクロールしている状態では、キャッシュ上の描画位置がずれます**。今のところ、直後の `Update()` でキャッシュが作り直されるので表に出ていないだけです (3.8 を参照)。

**提案**: `CashDevice` のコンストラクターで `SaveDC()` と `view.OnPrepareDC(&memoryDC)` を行い、デストラクターで `RestoreDC()` します。さらに、`isCashValid == false` のとき (ビットマップがまだない、または作り直す予定のとき) は、描画しても無駄になることをドキュメントに書くか、アサーションで知らせます。

### 3.6 🟠 RAII クラスのコピーが禁止されていない

- `GdiObjectSelector` (`:12-23`): コピーすると、デストラクターで元のオブジェクトの選択を 2 回戻すことになります。
- `CashDevice` (`:44-62`): 生ポインターを `new` / `delete` しているので、コピーすると**二重 delete** になります。

**提案**

```cpp
GdiObjectSelector(const GdiObjectSelector&) = delete;
GdiObjectSelector& operator=(const GdiObjectSelector&) = delete;
```

`CashDevice` は `new` をやめ、メンバーの宣言順を利用して直接持つ形にします (`CDC memoryDC; GdiObjectSelector selector;`)。`std::optional<GdiObjectSelector>` を使う方法もあります。どちらのクラスもポリモーフィックに使わないので、`virtual` デストラクターは不要です。`final` を付けることも検討してください。

また、`SelectObject` は失敗すると `nullptr` を返します。その場合、デストラクターで `SelectObject(nullptr)` が呼ばれないように、ガードを入れるのが安全です。

### 3.7 🟠 Layer 2 の描画でちらつく可能性 (`MFCDoubleBuffer.h:70-75`)

```cpp
DrawCash(dc);            // キャッシュを画面へ BitBlt
view.OnPrepareDC(&dc);
OnDrawLayer2(dc);        // 画面 DC に直接描画
```

背景は BitBlt で一度に転送されますが、その直後に Layer 2 を画面へ直接描いています。そのため、Layer 2 の部分は「背景だけの状態 → オーバーレイが描かれた状態」と 2 段階で表示され、ちらつく可能性があります。描画量が少なければ目立ちませんが、ライブラリの目的 (ちらつき防止) からすると不完全です。

**提案**: 作業用のビットマップをもう 1 枚用意します。キャッシュを作業用ビットマップへコピーし、その上に Layer 2 を描いてから、画面へ一度に BitBlt します。

### 3.8 🟠 サンプル: `OnLButtonUp` の処理が噛み合っていない (`View.h:30-39`)

```cpp
Draw(CashDevice(*this), point);  // キャッシュに差分を描く
Update();                        // ↑ の直後にキャッシュを無効化 → 差分描画が無駄になる
// Invalidate() がない
```

- 差分描画のすぐ後に `Update()` でキャッシュを無効化しているので、`CashDevice` を使う意味がありません。
- `Invalidate()` を呼んでいないので、クリックしただけでは画面が更新されません。今はマウスの移動で再描画されるので、たまたま動いているだけです。
- この処理はサンプルの中心となる部分なので、ライブラリの正しい使い方を示すように直すべきです。

**提案** (3.5 の修正が前提です)

```cpp
Draw(CashDevice(*this), point);   // キャッシュへ差分を描画
Invalidate(FALSE);                // 再描画を要求 (Update() は呼ばない)
```

あるいは、ドキュメントの変更として `GetDocument()->UpdateAllViews(nullptr)` を呼ぶ形にします (`OnUpdate` → `Update()` が実行されます)。

### 3.9 🟠 サンプル: SDI で「新規作成」しても図形が残る (`Document.h`)

`CSingleDocTemplate` はドキュメントオブジェクトを再利用し、「新規作成」や「開く」のときに `DeleteContents()` を呼びます。`Document` はこれをオーバーライドしていないため、`figures` と `rubberBand` が前の状態のまま残ります。

また、メニューには「上書き保存」「名前を付けて保存」「開く」がありますが、`Serialize()` を実装していないので、保存しても中身は空です。

**提案**
- `DeleteContents()` で `figures.clear()` を行います。
- `Serialize()` を実装するか、使わないメニュー項目をリソースから削除します。

### 3.10 🟡 エクスポート用マクロ (`MFCDoubleBuffer.h:6-10`, `pch.h:5`)

- `EXPORTING` と `CLASS_DECLSPEC` は名前が汎用的すぎます。利用者側のコードや他のライブラリと衝突する恐れがあります。マクロは名前空間の影響を受けないので、`namespace` の中で `#define` しても意味がありません。
- このライブラリは MFC 拡張 DLL (`_AFXEXT` が定義済み) です。標準の `AFX_EXT_CLASS` を使えば、独自のマクロは不要になります。
- クラステンプレート `DoubleBufferViewBase` に `__declspec(dllexport/dllimport)` を付けています。すべてインラインで書かれているので実害は小さいですが、意図が分かりにくく、C4275 / C4251 系の警告の原因にもなります。
- このヘッダーは `#include` を持たず、利用側の pch で `<afxwin.h>` などが読み込まれていることを前提にしています。

### 3.11 🟡 名前

- `isCashValid`, `cash`, `CashDevice`, `DrawCash` は「キャッシュ」の意味なので、`Cache` と綴るのが正しいです (`Cash` は現金の意味)。`CashDevice` は公開 API なので、改名すると互換性が壊れます。互換用の別名を残すことを検討してください。
- `OnPaint(CDC&)` (仮想関数) と `afx_msg OnPaint()` (メッセージハンドラー) が同じ名前でオーバーロードされています。派生クラスで片方を宣言すると、もう片方が隠れてしまいます。前者は `OnPaintBuffered(CDC&)` などに改名するのが安全です。
- サンプルの `View::Draw(CDC&, CPoint)` も、基底クラスの `DoubleBuffer::Draw(CDC&)` を隠しています。
- `Window()` は `CView&` を返すので、`View()` か `GetView()` のほうが実態に合っています。

### 3.12 🟡 細かい堅牢性

- **最小化時**: `clientRect` の幅や高さが 0 になると、`CreateCompatibleBitmap` が失敗します。それでも `isCashValid = true` になるので、空のビットマップを選択して BitBlt することになります。サイズが 0 のときは早めに return するか、作成の成否を確認してください。
- **`SetBackgroundColor()`** はキャッシュを無効化しません。描画後に色を変えても、次に `Update()` が呼ばれるまで反映されません。中で `Update()` を呼ぶのが自然です。
- **`WM_SYSCOLORCHANGE`**: 既定の背景色は、構築時の `GetSysColor(COLOR_WINDOW)` に固定されます。
- **`DrawCash()` の `GetClipBox()`**: 呼び出し時点で `OnPrepareDC` が適用されていないことを前提に、論理座標をデバイス座標として使っています。派生クラスで `OnPaint(CDC&)` をオーバーライドし、先に `OnPrepareDC` を呼ぶと、この前提が崩れます。コメントで明記するか、`GetClipBox` の後に `LPtoDP` で変換してください。
- **`OnEraseBkgnd`** が `bool` の `true` を返しています。`BOOL` を返す関数なので `TRUE` を使ってください。コメントアウトされた行は削除してください。
- **`DoubleBufferViewBase` のコンストラクター**: C スタイルのキャスト `*(TView*)this` は不要です。`DoubleBuffer(static_cast<CView&>(*this))`、あるいは単に `*this` で十分です。
- **`IMPLEMENT_DYNCREATE(DoubleBufferView, DoubleBufferViewBase<CView>)`**: テンプレートには `DECLARE_DYNAMIC` がないため、`CRuntimeClass` 上の基底クラスは `CView` になります。動作上の問題はありませんが、コメントで書いておくと親切です。
- **メッセージマップ**: `ON_WM_PAINT` / `ON_WM_ERASEBKGND` / `ON_WM_SIZE` の 3 つが 2 つのクラスに重複して書かれています。

### 3.13 🟡 サンプルのその他

- `View.h:4` でヘッダーに `using namespace Shos::MFCDoubleBuffer;` と書いています。ヘッダーでの using ディレクティブは避けるべきです。
- `GetDocument()` の Release 版で `reinterpret_cast` を使っています。正しいのは `static_cast` です (`STATIC_DOWNCAST` でも構いません)。
- `OnInitialUpdate()` に `override` が付いていません。
- `Document::GetRubberBand()` に `const` を付けるべきです。起動直後はラバーバンドが (0,0) に描かれます。マウスがウィンドウの外にある間は描かない、などの扱いを検討してください。
- `OnMouseMove` が毎回クライアント全体を `Invalidate()` しています。キャッシュがあるので負荷は小さいですが、前後の円を囲む矩形だけを `InvalidateRect(..., FALSE)` すれば、ライブラリの利点がより分かりやすく示せます。
- `Application.cpp:28` の `new` の結果を `nullptr` と比べるチェックは意味がありません (`new` は失敗すると例外を投げます)。
- `.rc` のバージョン情報に `TODO: <会社名>` が残っています。`IDD_ABOUTBOX` も使われていません。
- インデントがライブラリ (スペース) とサンプル (タブ) で統一されていません。`.editorconfig` を追加することを推奨します。

### 3.14 🟡 ビルド設定

- DLL 側の `framework.h` は、OLE / ODBC / DAO / コモンコントロールのヘッダーを読み込んでいますが、どれも使っていません。ビルドが遅くなり、依存関係も増えるので、`<afxwin.h>` と `<afxext.h>` だけで足ります。
- `WarningLevel` が `Level3` です。ライブラリでは `/W4` と `/permissive-` を有効にすることを推奨します。
- `.sln` のヘッダーは `# Visual Studio Version 16` (VS2019) のままですが、ツールセットは `v145` です。README の「for Visual Studio 2019」とも合っていません。

### 3.15 🟡 リポジトリ構成とドキュメント

- **README** は 3 行だけです。少なくとも次の内容を書くことを推奨します。
  - 2 層描画の考え方
  - 使い方 (継承するクラス、`OnDrawLayer1/2`、`Update()`、`CashDevice` の使い分け)
  - ビルド手順と対応する VS のバージョン
  - 制限事項 (印刷など)
- **ヘッダーの二重管理**: `include/MFCDoubleBuffer.h` は `MFCDoubleBuffer/MFCDoubleBuffer.h` をポストビルドでコピーしたもので、両方がコミットされています。現在は同じ内容ですが、Win32 でビルドするとコピーされないので、内容がずれる恐れがあります。サンプルからは `AdditionalIncludeDirectories` で元のヘッダーを直接参照し、`include/` はリリース用の成果物にするのが安全です。
- **不要なファイル**
  - `MFCDoubleBuffer.old/`: 古いプロジェクトファイル (v143) です。git の履歴に残るので削除して構いません。
  - `MFCDoubleBufferingSample/MainFrame.cpp`: HEAD では `#include "pch.h"` 以外がすべてコメントアウトされており、プロジェクトにも含まれていません。削除を推奨します。
- テストと CI がありません。GitHub Actions の `windows-latest` で `msbuild` によるビルドを確認するだけでも、3.3 のような構成の問題を早く見つけられます。

---

## 4. 良い点

- 「キャッシュする層」と「毎回描く層」を分けた API が単純で分かりやすく、`OnDrawLayer1/2` をオーバーライドするだけで使えます。
- `CView` 版と `CScrollView` 版を、テンプレートの基底クラス 1 つから派生させており、重複が少なくなっています。
- `OnUpdate` / `OnSize` で自動的にキャッシュを無効化しているので、普通の使い方では利用者が意識する必要がありません。
- `GdiObjectSelector` による RAII で、GDI オブジェクトの選択を戻し忘れることを防いでいます。
- BitBlt を `GetClipBox()` の範囲に限っていて、無駄な転送をしていません。
- `Shos::MFCDoubleBuffer` の名前空間で囲まれています。

---

## 5. 推奨する対応順

1. 作業ツリーの破損ファイルを `git restore` で戻す (3.1)
2. `OnDraw()` を実装し、印刷に対応する (3.2)
3. `ProjectReference` を使い、lib を構成ごとに分ける (3.3)
4. スクロール時の無効化を `OnScrollBy` に集約し、`CashDevice` に `OnPrepareDC` を適用する (3.4, 3.5)
5. サンプルの `OnLButtonUp` と `DeleteContents` を直す (3.8, 3.9)
6. RAII クラスのコピーを禁止し、マクロを `AFX_EXT_CLASS` に置き換える (3.6, 3.10)
7. README を充実させ、不要なファイルを削除し、CI を追加する (3.15)

---

## 6. 対応状況 (2026-10-21 追記)

方針 (オーナーより):
- `MFCDoubleBuffer.old/` は削除済み
- 印刷機能は不要
- `lib/` は Debug と Release で共用する

| # | 対応 | 内容 |
|---|---|---|
| 1 | ✅ | `LICENSE` を `git restore` で復旧。中身がコメントだけの `MainFrame.cpp` は削除 |
| 2 | ➖ | 印刷は非サポートとする。印刷に関するコード (メニュー、リソース、`OnPrint*`、`OnPreparePrinting` など) は残っていないことを確認した。純粋仮想関数として実装が必須の `OnDraw()` だけが残り、「非サポート」とコメントを付けた |
| 3 | ✅ (一部) | `lib/` の共用は維持。Win32 構成にもポストビルドを追加し、どのプラットフォームでも `include/` と `lib/` を更新するようにした |
| 4 | ✅ | `OnHScroll` / `OnVScroll` / `OnMouseWheel` をやめ、`OnScrollBy` のオーバーライドに集約。さらに、スクロール位置 (`GetCacheOrigin()`) が変わったことを描画時に検知し、`ScrollToPosition` などでもキャッシュを無効化するようにした |
| 5 | ✅ | `CacheDevice` が `SaveDC` / `OnPrepareDC` / `RestoreDC` を行うようにした |
| 6 | ✅ | `GdiObjectSelector` と `CacheDevice` のコピーを禁止。`new` / `delete` をやめて `std::optional` を使い、`nullptr` のガードを追加 |
| 7 | ✅ | Layer 2 を作業用ビットマップで合成してから、画面へ一度に転送するようにした |
| 8 | ✅ | サンプルは `CacheDevice` で差分を描いた後、該当する矩形だけを `InvalidateRect` する形に変更。`OnMouseMove` も、前後の円を囲む矩形だけを無効化するようにした |
| 9 | ✅ | `DeleteContents()` と `Serialize()` を実装し、`SetModifiedFlag()` を呼ぶようにした |
| 10 | ✅ | マクロを `AFX_EXT_CLASS` に置き換え、`EXPORTING` を削除。テンプレートからは `__declspec` を外した |
| 11 | ✅ | `Cash` → `Cache` に改名。互換用に `[[deprecated]] CashDevice` の別名を残した。`OnPaint(CDC&)` は `OnPaintBuffered` に、サンプルの `Draw` は `DrawFigure` に改名 |
| 12 | ✅ | クライアント領域が空のときやビットマップの作成に失敗したときに早めに return するようにした。`SetBackgroundColor` で `Update()` を呼ぶ。`OnEraseBkgnd` は `TRUE` を返す。C スタイルのキャストを削除 |
| 13 | ✅ | README を全面的に更新 |
| 14 | ✅ | 不要なファイルを削除。`include/` へは手動で同期した |
| 他 | ✅ | `using namespace` をヘッダーから削除、`static_cast` に変更、`override` を追加、意味のない `nullptr` チェックを削除、DLL の `framework.h` から不要な include を削除 |
| 未対応 | — | `.rc` の TODO 表記、`IDD_ABOUTBOX`、`/W4`、`.sln` の VS バージョン表記、CI、`WM_SYSCOLORCHANGE`、マウスがウィンドウ外にあるときのラバーバンド表示 |
