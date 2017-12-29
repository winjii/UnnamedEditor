#pragma once
#include <ft2build.h>
#include <map>
#include FT_FREETYPE_H
#include FT_OPENTYPE_VALIDATE_H
#include <string>
#include <FontDataPicker\GsubReader.h>
#include "Glyph.h"

using namespace FontDataPicker;

namespace UnnamedEditor {
namespace Font {


class Font {
private:

	FT_Library _lib; //所有権を持たない
	
	FT_Face _face;

	bool _isVertical;
	
	SP<GsubReader> _gsubReader;

public:

	Font(FT_Library lib, std::string fontPath, bool isVertical);

	Glyph renderChar(wchar_t charCode);
};


}
}