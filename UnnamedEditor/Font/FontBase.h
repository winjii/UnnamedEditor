#pragma once
#include "FontDataPicker\GsubReader.h"
#include "Glyph.h"


namespace UnnamedEditor {
namespace Font {


class FTLibraryWrapper : public Util::PointerWrapperBase<FT_Library> {
	//FT_Face = FT_FaceRec_*
	using Base = Util::PointerWrapperBase<FT_Library>;
private:
	static FT_Library CreateLibrary() {
		FT_Library lib;
		FT_Init_FreeType(&lib);
		return lib;
	}
	virtual void destroy(FT_Library lib) const { FT_Done_FreeType(lib); }
public:
	FTLibraryWrapper() : Base(CreateLibrary()) {}
	FTLibraryWrapper(FT_Library lib) : Base(lib) {}
};

class FTFaceWrapper : public Util::PointerWrapperBase<FT_Face> {
	//FT_Face = FT_FaceRec_*
	using Base = Util::PointerWrapperBase<FT_Face>;
private:
	static FT_Face CreateFace(FTLibraryWrapper lib, const std::string &fontPath) {
		//TODO:�R���N�V�����̏ꍇ�ł��K����0�Ԗڂ�face��I�����Ă���
		//����������Ƃ���ɂ́A���������t�H���g�t�@�C����register���Ă��疼�O�ŌĂяo�������̎������K�v�H 
		FT_Face face;
		int code = FT_New_Face(lib.raw(), fontPath.c_str(), 0, &face);
		return face;
	}
	virtual void destroy(FT_Face face) const { FT_Done_Face(face); }
public:
	FTFaceWrapper(FTLibraryWrapper lib, const std::string &fontPath) : Base(CreateFace(lib, fontPath)) {}
	FTFaceWrapper(FT_Face face) : Base(face) {}
};


class FontBase {
protected:

	FTLibraryWrapper _lib;

	FTFaceWrapper _face;

	bool _isVertical;

	double _ascender;

	double _descender;

	SP<FontDataPicker::GsubReader> _gsubReader;

	std::function<void(Image&)> _retouchImage;



	FontDataPicker::GlyphIndex getGID(char16_t charCode);

	SP<Glyph> renderChar(FontDataPicker::GlyphIndex gid, int size);

public:

	FontBase(FTLibraryWrapper lib, FTFaceWrapper face, bool isVertical);

	FontBase(FTLibraryWrapper lib, std::string fontPath, bool isVertical);

	FTLibraryWrapper ftLibrary();

	FTFaceWrapper ftFace();

	double ascender(double fontSize) const;

	double descender(double fontSize) const;

	Line getCursor(Vec2 pen, double fontSize);

	void setRetouchImage(std::function<void(Image&)> retouchImage);

};


}
}