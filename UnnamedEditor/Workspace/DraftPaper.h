#pragma once
#include "Font/Glyph.h"

namespace UnnamedEditor {
namespace Workspace {


using DevicePos = Vec2;


class DraftPaper {
private:

	RectF _paper;

	std::vector<SP<const Font::Glyph>> _glyphs;

	std::vector<DevicePos> _glyphPositions;

	//glyphsのboundingBoxの中心の位置が、このオブジェクトの位置
	DevicePos _pos;

	//画面下を0とするrad
	double _angle;

	Vec2 _desirableMargin;

public:

	DraftPaper(const std::vector<SP<const Font::Glyph>> &glyphs, double angle);

	void setPos(const DevicePos &pos);

	DevicePos getPos() const;

	void draw() const;
	
	//文字が収まるために必要なマージン
	DevicePos desirableMargin();

	
};


}
}