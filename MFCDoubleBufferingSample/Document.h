// Document.h : サンプルのドキュメント
// クリックで配置した図形 (円の中心座標) の一覧と、マウスに追従するラバーバンドの位置を保持する

#pragma once
#include <span>
#include <vector>

class Document : public CDocument
{
	DECLARE_DYNCREATE(Document)

	std::vector<CPoint> figures;    // 配置済みの図形 (論理座標)。ビューの Layer 1 で描く
	CPoint              rubberBand; // マウス カーソルの位置 (論理座標)。ビューの Layer 2 で描く

public:
	// ドキュメント全体の大きさ (スクロール範囲)
	[[nodiscard]] CSize GetSize() const
	{ return { 1000, 1000 }; }

	[[nodiscard]] CPoint GetRubberBand() const
	{ return rubberBand; }

	// ラバーバンドは一時的な表示なので、変更してもドキュメントの変更扱いにはしない
	void SetRubberBand(CPoint position)
	{ rubberBand = position; }

	[[nodiscard]] std::span<const CPoint> GetFigures() const
	{ return figures; }

	void AddFigure(CPoint figure)
	{
		figures.push_back(figure);
		SetModifiedFlag(); // 終了時などに保存を確認させる
	}

	// SDI ではドキュメント オブジェクトが再利用されるので、
	// 「新規作成」や「開く」の前に呼ばれるこの関数で内容を空にする
	void DeleteContents() override
	{
		figures.clear();
		rubberBand = {};
		CDocument::DeleteContents();
	}

	// ファイル形式: 図形の個数 (DWORD) に続けて、各図形の座標 (CPoint) を並べる
	void Serialize(CArchive& ar) override
	{
		if (ar.IsStoring()) {
			ar << static_cast<DWORD>(figures.size());
			for (const auto& figure : figures)
				ar << figure;
		} else {
			DWORD count = 0;
			ar >> count;
			figures.clear();
			// 壊れたファイルで count が巨大な値になっていても大量のメモリを確保しないよう、
			// 先に resize せず 1 件ずつ読む (データが足りなければ CArchiveException になる)
			for (DWORD index = 0; index < count; ++index) {
				CPoint figure;
				ar >> figure;
				figures.push_back(figure);
			}
		}
	}

	DECLARE_MESSAGE_MAP()
};
