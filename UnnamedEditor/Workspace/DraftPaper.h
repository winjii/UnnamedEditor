#pragma once
#include "Font/Glyph.h"

namespace UnnamedEditor {
namespace Workspace {


using DevicePos = Vec2;


class DraftPaper {
private:

	RectF _paper;

	std::vector<SP<Font::Glyph>> _glyphs;

	std::vector<DevicePos> _glyphPositions;

	DevicePos _pos;

	//‰æ–Ê‰º‚ð0‚Æ‚·‚érad
	double _angle;

	Vec2 _desirableMargin;

public:

	DraftPaper(const std::vector<SP<Font::Glyph>> &glyphs, double angle);

	void setPos(const DevicePos &pos);

	DevicePos desirableMargin();

	
};


}
}