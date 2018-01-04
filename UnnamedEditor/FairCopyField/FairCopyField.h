#pragma once
#include <list>
#include "Font\Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace UnnamedEditor {
namespace FairCopyField {

using DevicePos = Vec2;

class CharPos {
private:

	LLInt _lineIndex;
	
	double _posInLine;

public:

	CharPos(LLInt lineIndex, double posInLine) : _lineIndex(lineIndex), _posInLine(posInLine) {}

	//addend: •‰‚àOK
	CharPos Add(double addend, double lineHeight) const {
		CharPos ret(*this);
		ret._posInLine += addend;
		while (ret._posInLine >= lineHeight) {
			ret._posInLine -= lineHeight;
			ret._lineIndex++;
		}
		while (ret._posInLine < 0) {
			ret._posInLine += lineHeight;
			ret._lineIndex--;
		}
		return ret;
	}
	
	LLInt getLineIndex() { return _lineIndex; }

	double getPosInLine() { return _posInLine; }

	//‘‚«o‚µˆÊ’u‚©‚ç‚Ì‘Š‘ÎˆÊ’u
	DevicePos toDeviceDelta(double lineInterval, double lineHeight) {
		return DevicePos(-lineInterval*_lineIndex, _posInLine);
	}
};

struct Char {
	Font::Glyph _glyph;
	CharPos _pos;

	Char(const Font::Glyph &glyph, const CharPos &pos) : _glyph(glyph), _pos(pos) {}
};

class FairCopyField {
private:

	DevicePos _pos, _size;

	Font::Font _font;

	int _fontSize;

	double _lineInterval, _lineHeight;

	std::list<Char> _chars;

	String _text;

	double _cursorLength;

	DevicePos _cursor;

public:

	FairCopyField(double x, double y, double w, double h, FT_Library lib, int fontSize = 20);

	~FairCopyField();

	void setText(const String &text);

	void update();
};

}
}