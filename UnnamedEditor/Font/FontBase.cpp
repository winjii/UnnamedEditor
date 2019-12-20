#include "FontBase.h"


using namespace FontDataPicker;


namespace UnnamedEditor {
namespace Font {


FontDataPicker::GlyphIndex FontBase::getGID(char16_t charCode) {
	GlyphIndex gid = FT_Get_Char_Index(_face.raw(), charCode);
	if (_isVertical) gid = _gsubReader->vertSubstitute(gid);
	return gid;
}

SP<Glyph> FontBase::renderChar(GlyphIndex gid, int fontSize) {
	SP<Glyph> ret;

	FT_Set_Pixel_Sizes(_face.raw(), fontSize, 0);
	//TODO:bitmapタイプのフォントの時に死にそう
	//FT_LOAD_NO_BITMAPを指定してRenderするのは外形指定タイプのフォントの読み込み処理
	FT_Load_Glyph(_face.raw(), gid, FT_LOAD_NO_BITMAP);
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
	if (_retouchImage) _retouchImage(image);
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

FontBase::FontBase(FTLibraryWrapper lib, FTFaceWrapper face, bool isVertical)
: _lib(lib)
, _face(face)
, _isVertical(isVertical) {
	FT_Select_Charmap(_face.raw(), FT_Encoding_::FT_ENCODING_UNICODE);
	_gsubReader = SP<GsubReader>(new GsubReader(_face.raw()));
}

FontBase::FontBase(FTLibraryWrapper lib, std::string fontPath, bool isVertical)
: FontBase(lib, FTFaceWrapper(lib, fontPath), isVertical) {
}

FTLibraryWrapper FontBase::ftLibrary() {
	return _lib;
}

FTFaceWrapper FontBase::ftFace() {
	return _face;
}

double FontBase::ascender(double fontSize) const {
	return fontSize * (_face->ascender / (double)_face->units_per_EM);
}

double FontBase::descender(double fontSize) const {
	return fontSize * (_face->descender / (double)_face->units_per_EM);
}

Line FontBase::getCursor(Vec2 pen, double fontSize) {
	if (!_isVertical)
		return Line(pen.x, pen.y - ascender(fontSize), pen.x, pen.y - descender(fontSize));
	return Line(Vec2(pen.x - fontSize/2, pen.y), Vec2(pen.x + fontSize/2, pen.y));
}

void FontBase::setRetouchImage(std::function<void(Image&)> retouchImage) {
	_retouchImage = retouchImage;
}

}
}
