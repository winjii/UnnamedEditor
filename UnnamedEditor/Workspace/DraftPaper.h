#pragma once
#include "Font/Glyph.h"

namespace UnnamedEditor {
namespace Workspace {


using DevicePos = Vec2;


class DraftPaper {
private:

	//(0, 0)とオブジェクトの位置が一致するように
	RectF _paper;

	std::vector<SP<const Font::Glyph>> _glyphs;

	std::vector<DevicePos> _glyphPositions;

	//glyphsのboundingBoxの中心の位置が、このオブジェクトの位置
	DevicePos _pos;

	//文字のデフォルトの角度を0とするrad
	double _angle;

	Vec2 _desirableMargin;

public:

	DraftPaper(const std::vector<SP<const Font::Glyph>> &glyphs, double angle);

	void setPos(const DevicePos &pos);

	DevicePos getPos() const;

	void draw() const;
	
	//文字が収まるために上下と左右に必要なマージン
	Vec2 desirableMargin();

	RectF boundingBox(const Vec2 &pos = Vec2(0, 0));
	
};


}
}