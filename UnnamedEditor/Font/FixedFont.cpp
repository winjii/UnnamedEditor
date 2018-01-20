#include "FixedFont.h"
#include "ft2build.h"
#include FT_FREETYPE_H


using namespace FontDataPicker;


namespace UnnamedEditor {
namespace Font {


FixedFont::FixedFont(FT_Library lib, std::string fontPath, int pixelSize, bool isVertical)
: FontBase(lib, fontPath, isVertical)
, _fontSize(pixelSize) {
	
}

int FixedFont::getFontSize() {
	return _fontSize;
}

Line FixedFont::getCursor(Vec2 pen) {
	return FontBase::getCursor(pen, _fontSize);
}

SP<const Glyph> FixedFont::renderChar(char16_t charCode) {
	GlyphIndex gid = getGID(charCode);
	auto itr = _glyphMemo.find(gid);
	if (itr != _glyphMemo.end()) return itr->second;
	return _glyphMemo[gid] = FontBase::renderChar(gid, _fontSize);
}

std::vector<SP<const Glyph>> FixedFont::renderString(std::u16string charCodes) {
	std::vector<SP<const Glyph>> ret;
	for each (char16_t var in charCodes) {
		ret.push_back(renderChar(var));
	}
	return ret;
}


}
}