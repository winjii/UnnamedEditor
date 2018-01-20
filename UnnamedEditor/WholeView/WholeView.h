#pragma once
#include "Font\FixedFont.h"
#include <list>


namespace UnnamedEditor {
namespace WholeView {


using DevicePos = Vec2;


class WholeView {
private:
	/*
	class TranslationIntoWorkspace {
	private:

		RectF _startRect;

		double _startSize, _endSize;

		Vec2 _startHead, _endHead;

		double _lineInterval;

		Stopwatch _sw;

		const int _animationMS = 2000;

	public:

		TranslationIntoWorkspace(const WholeView &wv, double endSize, const Vec2 &endHead);
	};*/

private:

	DevicePos _borderPos, _borderSize;

	Vec2 _pos, _size;

	SP<Font::FixedFont> _font;

	String _text;

	std::list<SP<const Font::Glyph>> _glyphs;

	int _pageCount;

	double _lineInterval;

	int _cursorIndex;

public:

	//font: verticalÇ≈Ç»ÇØÇÍÇŒÇ»ÇÁÇ»Ç¢(Ç±ÇÍê›åvâòÇ≠Ç»Ç¢ÅH)
	WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font);

	void setText(const String &text);

	void update();

	Vec2 getCharPos(int charIndex) const;

};


}
}