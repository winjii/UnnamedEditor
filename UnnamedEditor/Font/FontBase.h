#pragma once
#include "FontDataPicker\GsubReader.h"
#include "Glyph.h"


namespace UnnamedEditor {
namespace Font {


class FontBase {
protected:

	FT_Library _lib; //所有権なし

	FT_Face _face; //所有権あり

	bool _isVertical;

	double _ascender;

	double _descender;

	SP<FontDataPicker::GsubReader> _gsubReader;



	FontDataPicker::GlyphIndex getGID(char16_t charCode);

	SP<Glyph> renderChar(FontDataPicker::GlyphIndex gid, int size);

public:

	FontBase(FT_Library lib, std::string fontPath, bool isVertical);
	
	~FontBase();

	double ascender();

	double descender();

	Line getCursor(Vec2 pen, double fontSize);

};


}
}