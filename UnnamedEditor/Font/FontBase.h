#pragma once
#include "FontDataPicker\GsubReader.h"
#include "Glyph.h"


namespace UnnamedEditor {
namespace Font {


class FontBase {
protected:

	FT_Library _lib; //èäóLå†Ç»Çµ

	FT_Face _face; //èäóLå†Ç†ÇË

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