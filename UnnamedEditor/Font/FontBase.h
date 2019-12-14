#pragma once
#include "FontDataPicker\GsubReader.h"
#include "Glyph.h"


namespace UnnamedEditor {
namespace Font {


class FTLibraryWrapper {
	//FT_Face = FT_FaceRec_*
private:
	static FT_Library CreateLibrary() {
		FT_Library lib;
		FT_Init_FreeType(&lib);
		return lib;
	}
	
	struct Inner {
		FT_Library _lib;
		Inner(FT_Library lib) : _lib(lib) {}
		~Inner() { FT_Done_FreeType(_lib); }
	};
	SP<Inner> _inner;
public:
	FTLibraryWrapper() : FTLibraryWrapper(CreateLibrary()) {}
	FTLibraryWrapper(FT_Library lib) : _inner(new Inner(lib)) {}
	FT_Library operator->() { return _inner->_lib; }
	FT_Library raw() { return _inner->_lib; }
};

class FTFaceWrapper {
	//FT_Face = FT_FaceRec_*
private:
	static FT_Face CreateFace(FTLibraryWrapper lib, const std::string &fontPath) {
		//TODO:�R���N�V�����̏ꍇ�ł��K����0�Ԗڂ�face��I�����Ă���
		//����������Ƃ���ɂ́A���������t�H���g�t�@�C����register���Ă��疼�O�ŌĂяo�������̎������K�v�H 
		FT_Face face;
		FT_New_Face(lib.raw(), fontPath.c_str(), 0, &face);
		return face;
	}

	struct Inner {
		FT_Face _face;
		Inner(FT_Face face) : _face(face) {}
		~Inner() { FT_Done_Face(_face); }
	};
	SP<Inner> _inner;
public:
	FTFaceWrapper(FTLibraryWrapper lib, const std::string &fontPath)
		: _inner(new Inner(CreateFace(lib, fontPath))) {}
	FTFaceWrapper(FT_Face face) : _inner(new Inner(face)) {}
	FT_Face operator->() { return _inner->_face; }
	FT_Face raw() { return _inner->_face; }
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

	double ascender();

	double descender();

	Line getCursor(Vec2 pen, double fontSize);

	void setRetouchImage(std::function<void(Image&)> retouchImage);

};


}
}