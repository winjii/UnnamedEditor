#pragma once
#include <ft2build.h>
#include <map>
#include FT_FREETYPE_H
#include FT_OPENTYPE_VALIDATE_H
#include <string>
#include <FontDataPicker\GsubReader.h>
#include "Glyph.h"
#include "FontBase.h"

namespace UnnamedEditor {
namespace Font {


class FixedFont : public FontBase {
private:
	
	int _fontSize;

	std::unordered_map<FontDataPicker::GlyphIndex, SP<Glyph>> _glyphMemo;

public:

	FixedFont(FTLibraryWrapper lib, FTFaceWrapper face, int pixelSize, bool isVertical = false);

	FixedFont(FTLibraryWrapper lib, std::string fontPath, int pixelSize, bool isVertical = false);

	int getFontSize();

	bool isVertical();

	int ascender() const;

	int descender() const;

	Line getCursor(Vec2 pen);

	SP<const Glyph> renderChar(char16_t charCode);

	std::vector<SP<const Glyph>> renderString(std::u16string charCodes);
};


}
}