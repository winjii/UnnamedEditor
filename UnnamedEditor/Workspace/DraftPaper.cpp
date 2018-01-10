#include "stdafx.h"
#include "DraftPaper.h"

namespace UnnamedEditor {
namespace Workspace {


DraftPaper::DraftPaper(const std::vector<SP<const Font::Glyph>> &glyphs, double angle)
: _glyphs(glyphs)
, _glyphPositions(glyphs.size())
, _angle(angle) {
	double _lineInterval = glyphs[0]->getFontSize()*1.2;
	Vec2 lineDelta = glyphs[0]->isVertical() ? Vec2(-_lineInterval, 0) : Vec2(0, _lineInterval);
	Vec2 inclinedLineDelta = lineDelta.rotated(angle);

	Vec2 center;
	{
		Vec2 min(1e100, 1e100), max(0, 0);
		auto takeMinMax = [&](const Vec2 &pos) {
			min.x = std::min(min.x, pos.x);
			min.y = std::min(min.y, pos.y);
			max.x = std::max(max.x, pos.x);
			max.y = std::max(max.y, pos.y);
		};

		Vec2 lineHead(0, 0);
		Vec2 pen = lineHead;
		for (int i = 0; i < (int)glyphs.size(); i++) {
			RectF bBox = glyphs[i]->boundingBox(pen, angle);
			takeMinMax(bBox.bl());
			takeMinMax(bBox.br());
			takeMinMax(bBox.tl());
			takeMinMax(bBox.tr());

			_glyphPositions[i] = pen;

			if ((i + 1) % 15 == 0) {
				pen = (lineHead += inclinedLineDelta);
			}
			else {
				pen += glyphs[i]->getAdvance(angle);
			}
		}
		center = (max + min)/2.0;
		_desirableMargin = (max - min)/2.0;
		for (int i = 0; i < (int)_glyphPositions.size(); i++) {
			_glyphPositions[i] -= center;
		}
	}
	{
		Vec2 min(1e100, 1e100), max(0, 0);
		auto takeMinMax = [&](const Vec2 &pos) {
			min.x = std::min(min.x, pos.x);
			min.y = std::min(min.y, pos.y);
			max.x = std::max(max.x, pos.x);
			max.y = std::max(max.y, pos.y);
		};

		Vec2 lineHead(0, 0);
		Vec2 pen = lineHead;
		for (int i = 0; i < (int)glyphs.size(); i++) {
			RectF bBox = glyphs[i]->boundingBox(pen);
			takeMinMax(bBox.bl());
			takeMinMax(bBox.br());
			takeMinMax(bBox.tl());
			takeMinMax(bBox.tr());

			if ((i + 1) % 15 == 0) {
				pen = (lineHead += lineDelta);
			}
			else {
				pen += glyphs[i]->getAdvance();
			}
		}
		_paper = RectF(min - center.rotated(-angle) - Vec2(_lineInterval, _lineInterval),
					   max - min + Vec2(_lineInterval, _lineInterval)*2);
	}
}

void DraftPaper::setPos(const DevicePos &pos) {
	_pos = pos;
}

DevicePos DraftPaper::getPos() const {
	return _pos;
}

void DraftPaper::draw() const {
	Color paperColor = Color(Palette::White, 220);
	_paper.rotatedAt(Vec2(0, 0), _angle).moveBy(_pos).draw(paperColor).drawFrame(1, Palette::Black);
	for (int i = 0; i < (int)_glyphPositions.size(); i++) {
		_glyphs[i]->draw(_pos + _glyphPositions[i], Palette::Black, _angle);
	}
}

Vec2 DraftPaper::desirableMargin() {
	return _desirableMargin;
}

RectF DraftPaper::boundingBox(const Vec2 & pos) {
	Vec2 min(1e100, 1e100), max(0, 0);
	auto takeMinMax = [&](const Vec2 &pos) {
		min.x = std::min(min.x, pos.x);
		min.y = std::min(min.y, pos.y);
		max.x = std::max(max.x, pos.x);
		max.y = std::max(max.y, pos.y);
	};
	Quad quad = _paper.rotatedAt(Vec2(0, 0), _angle).moveBy(_pos);
	takeMinMax(quad.p0);
	takeMinMax(quad.p1);
	takeMinMax(quad.p2);
	takeMinMax(quad.p3);
	return RectF(min, max - min);
}


}
}