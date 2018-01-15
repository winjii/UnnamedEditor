#pragma once
#include "Font\Font.h"
#include <list>


namespace UnnamedEditor {
namespace WholeView {


using DevicePos = Vec2;


class WholeView {
private:

	DevicePos _borderPos, _borderSize;

	Vec2 _pos, _size;

	SP<Font::Font> _font;

	String _text;

	std::list<SP<const Font::Glyph>> _glyphs;

	int _pageCount;

	double _lineInterval;

public:

	//font: verticalÇ≈Ç»ÇØÇÍÇŒÇ»ÇÁÇ»Ç¢(Ç±ÇÍê›åvâòÇ≠Ç»Ç¢ÅH)
	WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::Font> font);

	void setText(const String &text);

	void update();

};


}
}