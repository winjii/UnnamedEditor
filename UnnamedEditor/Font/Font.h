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

	FT_Library _lib; //���L���������Ȃ�
	
	FT_Face _face;

	bool _isVertical;
	
	SP<GsubReader> _gsubReader;

public:

	Font(FT_Library lib, std::string fontPath, int pixelWidth, int pixelHeight, bool isVertical = false);

	~Font();

	void ChangeSize(int pixelWidth, int pixelHeight);

	Glyph renderChar(char16_t charCode);

	std::vector<Glyph> renderString(std::u16string charCodes);
};


}
}