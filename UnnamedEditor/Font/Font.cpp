#include "Font.h"

namespace UnnamedEditor {
namespace Font {

Font::Font(FT_Library lib, std::string filePath, bool isVertical)
: _isVertical(isVertical) {
	FT_New_Face(lib, filePath.c_str(), 0, &_face);
	FT_Select_Charmap(_face, FT_Encoding_::FT_ENCODING_UNICODE);
	int error = FT_Select_Size(_face, 3);
	_gsubReader = SP<GsubReader>(new GsubReader(_face));
}

Glyph Font::renderChar(wchar_t charCode) {
	FT_UInt gid = FT_Get_Char_Index(_face, charCode);
	if (_isVertical) gid = _gsubReader->vertSubstitute(gid);
	FT_Load_Glyph(_face, gid, FT_LOAD_NO_BITMAP);
	FT_Render_Glyph(_face->glyph, FT_Render_Mode::FT_RENDER_MODE_NORMAL);
	FT_GlyphSlot slot = _face->glyph;
	FT_Bitmap bitmap = _face->glyph->bitmap;
	Image image(bitmap.width, bitmap.rows);
	for (int r = 0; r < bitmap.rows; r++) {
		for (int c = 0; c < bitmap.width; c++) {
			HSV gray(Color(bitmap.buffer[r*bitmap.width + c]));
			image[r][c] = Color(Palette::Black, 255*gray.v);
		}
	}
	if (_isVertical) {
		return Glyph(_isVertical,
					 slot->metrics.vertBearingX/64.0,
					 slot->metrics.vertBearingY/64.0,
					 slot->metrics.vertAdvance/64.0,
					 image);
	}
	else {
		return Glyph(_isVertical,
					 slot->metrics.horiBearingX/64.0,
					 -slot->metrics.horiBearingY/64.0,
					 slot->metrics.horiAdvance/64.0,
					 image);
	}
}


}
}