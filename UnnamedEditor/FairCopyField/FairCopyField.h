#pragma once
#include <list>
#include "Font\Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace UnnamedEditor {
namespace FairCopyField {

using DevicePos = OpaqueAlias<Vec2>;

class CharPos {
private:

	LLInt _lineIndex;
	
	double _posInLine;

public:

	CharPos(LLInt lineIndex, double posInLine) : _lineIndex(lineIndex), _posInLine(posInLine) {}

	CharPos Add(double addend, double lineHeight) const {
		CharPos ret(*this);
		ret._posInLine += addend;
		while (ret._posInLine >= lineHeight) {
			ret._posInLine -= lineHeight;
			ret._lineIndex++;
		}
		return ret;
	}
	
	LLInt GetLineIndex() { return _lineIndex; }

	double GetPosInLine() { return _posInLine; }

	DevicePos ToDeviceDelta(double lineInterval, double lineHeight) {
		return DevicePos(-lineInterval*_lineIndex, _posInLine);
	}
};

class FairCopyField {
private:

	DevicePos _pos, _size;

	Font::Font _font;

	std::list<Font::Glyph> _glyphs;

	String _text;

public:

	FairCopyField(double x, double y, double w, double h, FT_Library lib);

	~FairCopyField();

	void SetText(const String &text);

	void Update();
};

}
}