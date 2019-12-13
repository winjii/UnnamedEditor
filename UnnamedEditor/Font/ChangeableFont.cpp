#include "ChangeableFont.h"


using namespace FontDataPicker;


namespace UnnamedEditor {
namespace Font {


ChangeableFont::ChangeableFont(FTLibraryWrapper lib, std::string fontPath, bool isVertical)
: FontBase(lib, fontPath, isVertical) {}

ChangeableFont::~ChangeableFont() {}

SP<const Glyph> ChangeableFont::renderChar(char16_t charCode, double size, double &o_scale) {
	int baseSize = 1;
	while (baseSize + 1e-3 < size) baseSize *= 2;
	o_scale = size/baseSize;
	GlyphIndex gid = getGID(charCode);
	std::pair<int, GlyphIndex> key(baseSize, gid);
	auto itr = _glyphMemo.find(key);
	if (itr != _glyphMemo.end()) return itr->second;
	return _glyphMemo[key] = FontBase::renderChar(gid, baseSize);
}


}
}