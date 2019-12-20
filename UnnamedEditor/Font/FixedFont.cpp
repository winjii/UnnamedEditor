#include "FixedFont.h"
#include "ft2build.h"
#include FT_FREETYPE_H


using namespace FontDataPicker;


namespace UnnamedEditor {
namespace Font {


FixedFont::FixedFont(FTLibraryWrapper lib, FTFaceWrapper face, int pixelSize, bool isVertical)
: FontBase(lib, face, isVertical)
, _fontSize(pixelSize) {
}

FixedFont::FixedFont(FTLibraryWrapper lib, std::string fontPath, int pixelSize, bool isVertical)
: FixedFont(lib, FTFaceWrapper(lib, fontPath), pixelSize, isVertical) { }

int FixedFont::ascender() const {
	return (int)(FontBase::ascender(_fontSize) + 0.5);
}

int FixedFont::descender() const {
	return (int)(FontBase::descender(_fontSize) + 0.5);
}

int FixedFont::getFontSize() {
	return _fontSize;
}

bool FixedFont::isVertical() {
	return _isVertical;
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
	for (char16_t var : charCodes) {
		ret.push_back(renderChar(var));
	}
	return ret;
}


}
}