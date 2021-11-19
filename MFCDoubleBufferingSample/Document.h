#pragma once
#include <vector>

class Document : public CDocument
{
	DECLARE_DYNCREATE(Document)

	std::vector<CPoint> figures;
	CPoint              rubberBand;

public:
	using const_iterator = std::vector<CPoint>::const_iterator;

	CSize GetSize() const
	{ return CSize(1000, 1000); }

	CPoint GetRubberBand()
	{ return rubberBand; }

	void SetRubberBand(CPoint position)
	{ rubberBand = position; }

	const_iterator begin() const
	{ return figures.begin(); }

	const_iterator end() const
	{ return figures.end(); }

	void push_back(CPoint figure)
	{ figures.push_back(figure); }

	DECLARE_MESSAGE_MAP()
};
