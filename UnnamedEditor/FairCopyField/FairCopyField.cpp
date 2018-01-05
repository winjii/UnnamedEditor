#include "stdafx.h"
#include "FairCopyField.h"

namespace UnnamedEditor {
namespace FairCopyField {

FairCopyField::FairCopyField(double x, double y, double w, double h, FT_Library lib, int fontSize)
: _pos(Vec2(x, y))
, _size(Vec2(w, h))
, _font(lib, "C:/Windows/Fonts/msmincho.ttc", fontSize, true)
, _fontSize(fontSize)
, _lineInterval(fontSize*1.3)
, _lineHeight(h)
, _cursorLength(fontSize)
, _cursor(x + w - (_lineInterval - _cursorLength)/2.0, y + fontSize/2.0) {
	
}

FairCopyField::~FairCopyField() {}

void FairCopyField::setText(const String & text) {
	_text = text;
	std::u16string u16Text = _text.toUTF16();
	_chars.clear();

	CharPos charPos(0, 0);
	for each (char16_t c in u16Text) {
		SP<const Font::Glyph> glyph = _font.renderChar(c);
		_chars.push_back(Char(*glyph, charPos));
		charPos = charPos.Add(glyph->getAdvance().y, _lineHeight);
	}
}

void FairCopyField::update() {
	if (KeyUp.pressed()) {
		double t = Min(KeyUp.pressedDuration().count()/(long double)700, 1.0l);
		_cursor += DevicePos(0, -5*EaseIn(Easing::Quad, t));
	}
	if (KeyDown.pressed()) {
		double t = Min(KeyDown.pressedDuration().count()/(long double)700, 1.0l);
		_cursor += DevicePos(0, 5*EaseIn(Easing::Quad, t));
	}
	if (KeyLeft.pressed()) {
		double t = Min(KeyLeft.pressedDuration().count()/(long double)700, 1.0l);
		_cursor += DevicePos(-5*EaseIn(Easing::Quad, t), 0);
	}
	if (KeyRight.pressed()) {
		double t = Min(KeyRight.pressedDuration().count()/(long double)700, 1.0l);
		_cursor += DevicePos(5*EaseIn(Easing::Quad, t), 0);
	}

	DevicePos origin(_pos.x + _size.x - _lineInterval*0.5, _pos.y);

	RectF((Vec2)_pos, _size).draw(Palette::White);
	for each (Char c in _chars) {
		DevicePos pos = DevicePos(origin + c._pos.toDeviceDelta(_lineInterval, _lineHeight));
		c._glyph.draw(pos);
	}
	Line l(_cursor, _cursor + DevicePos(-_fontSize, 0));
	l.draw(4.0, Palette::Red);
}

}
}