#include "stdafx.h"
#include "DraftPaper.h"

namespace UnnamedEditor {
namespace Workspace {


DraftPaper::DraftPaper(const std::vector<SP<Font::Glyph>> &glyphs, double angle)
: _glyphs(glyphs)
, _angle(angle) {
	double _lineInterval = glyphs[0]->getFontSize()*1.2;
	Vec2 toNewLine = glyphs[0]->isVertical() ? Vec2(-_lineInterval, 0) : Vec2(0, _lineInterval);
	toNewLine.rotate(angle);

	Vec2 min(0, 0), max(1e100, 1e100);
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
		
		if (i % 15 == 0) {
			pen = (lineHead += toNewLine);
		}
		else {
			pen += glyphs[i]->getAdvance(angle);
		}
	}
	_desirableMargin = Vec2((max.x - min.x)/2.0, (max.y - min.y)/2.0);
}

DevicePos DraftPaper::desirableMargin() {
	return _desirableMargin;
}


}
}