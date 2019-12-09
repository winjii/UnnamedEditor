#include "FontBase.h"


using namespace FontDataPicker;


namespace UnnamedEditor {
namespace Font {


FontDataPicker::GlyphIndex FontBase::getGID(char16_t charCode) {
	GlyphIndex gid = FT_Get_Char_Index(_face, charCode);
	if (_isVertical) gid = _gsubReader->vertSubstitute(gid);
	return gid;
}

SP<Glyph> FontBase::renderChar(GlyphIndex gid, int fontSize) {
	SP<Glyph> ret;

	FT_Set_Pixel_Sizes(_face, fontSize, 0);
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
			image[r][c] = ColorF(Palette::White, gray.v);
		}
	}
	if (_isVertical) {
		ret.reset(new Glyph(_isVertical,
							fontSize,
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
							fontSize,
							slot->metrics.horiBearingX/64.0,
							-slot->metrics.horiBearingY/64.0,
							slot->metrics.horiAdvance/64.0,
							image));
	}
	return ret;
}

FontBase::FontBase(FT_Library lib, std::string fontPath, bool isVertical)
: _lib(lib)
, _isVertical(isVertical) {
	//TODO:コレクションの場合でも適当に0番目のfaceを選択している
	//これをちゃんとするには、そもそもフォントファイルをregisterしてから名前で呼び出す感じの実装が必要？
	FT_New_Face(lib, fontPath.c_str(), 0, &_face);
	FT_Select_Charmap(_face, FT_Encoding_::FT_ENCODING_UNICODE);
	_gsubReader = SP<GsubReader>(new GsubReader(_face));
	_ascender = _face->ascender/64.0; //TODO: scalable(outline)フォントの場合は26.6 pixcel formatじゃない
	_descender = _face->descender/64.0;
}

FontBase::~FontBase() {
	// FT_Done_Face(_face);
}

double FontBase::ascender() {
	return _ascender;
}

double FontBase::descender() {
	return _descender;
}

Line FontBase::getCursor(Vec2 pen, double fontSize) {
	if (!_isVertical) return Line(pen.x, pen.y + _ascender, pen.x, pen.y + _descender);
	return Line(Vec2(pen.x - fontSize/2, pen.y), Vec2(pen.x + fontSize/2, pen.y));
}

}
}
