#include "Font.h"

namespace UnnamedEditor {
namespace Font {

Font::Font(FT_Library lib, std::string filePath, int pixelSize, bool isVertical)
: _isVertical(isVertical)
, _fontSize(pixelSize) {
	//TODO:コレクションの場合でも適当に0番目のfaceを選択している
	//これをちゃんとするには、そもそもフォントファイルをregisterしてから名前で呼び出す感じの実装が必要？
	FT_New_Face(lib, filePath.c_str(), 0, &_face);
	FT_Select_Charmap(_face, FT_Encoding_::FT_ENCODING_UNICODE);
	FT_Set_Pixel_Sizes(_face, _fontSize, _fontSize);
	_gsubReader = SP<GsubReader>(new GsubReader(_face));
}

Font::~Font() {
	FT_Done_Face(_face);
}

SP<const Glyph> Font::renderChar(char16_t charCode) {
	GlyphIndex gid = FT_Get_Char_Index(_face, charCode);
	if (_isVertical) gid = _gsubReader->vertSubstitute(gid);
	auto itr = _glyphData.find(gid);
	if (itr != _glyphData.end()) return itr->second;

	//TODO:bitmapタイプのフォントの時に死にそう
	//FT_LOAD_NO_BITMAPを指定してRenderするのは外形指定タイプのフォントの読み込み処理
	FT_Load_Glyph(_face, gid, FT_LOAD_NO_BITMAP);
	FT_Render_Glyph(_face->glyph, FT_Render_Mode::FT_RENDER_MODE_NORMAL);
	FT_GlyphSlot slot = _face->glyph;
	FT_Bitmap bitmap = _face->glyph->bitmap;
	bool valid = (bool)(bitmap.rows*bitmap.width);
	Image image = valid ? Image(bitmap.width, bitmap.rows) : Image();
	for (int r = 0; r < bitmap.rows; r++) {
		for (int c = 0; c < bitmap.width; c++) {
			HSV gray(Color(bitmap.buffer[r*bitmap.width + c]));
			image[r][c] = Color(Palette::White, 255*gray.v);
		}
	}
	SP<Glyph> ret;
	if (_isVertical) {
		ret.reset(new Glyph(_isVertical,
							_fontSize,
							slot->metrics.vertBearingX/64.0,
							slot->metrics.vertBearingY/64.0,
							slot->metrics.vertAdvance/64.0,
							image));
		/*ret.reset(new Glyph(_isVertical,
							_fontSize,
							slot->metrics.vertBearingX >> 6,
							slot->metrics.vertBearingY >> 6,
							slot->metrics.vertAdvance >> 6,
							image));*/
	}
	else {
		ret.reset(new Glyph(_isVertical,
							_fontSize,
							slot->metrics.horiBearingX/64.0,
							-slot->metrics.horiBearingY/64.0,
							slot->metrics.horiAdvance/64.0,
							image));
	}
	_glyphData[gid] = ret;
	return ret;
}

std::vector<SP<const Glyph>> Font::renderString(std::u16string charCodes) {
	std::vector<SP<const Glyph>> ret;
	for each (char16_t var in charCodes) {
		ret.push_back(renderChar(var));
	}
	return ret;
}


}
}