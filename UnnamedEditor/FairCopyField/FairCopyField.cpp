#include "stdafx.h"
#include "FairCopyField.h"

namespace UnnamedEditor {
namespace FairCopyField {

FairCopyField::FairCopyField(double x, double y, double w, double h, FT_Library lib)
: _pos(Vec2(x, y))
, _size(Vec2(w, h))
, _font(lib, "C:/Windows/Fonts/msmincho.ttc", 20, 20, true) {
	
}

FairCopyField::~FairCopyField() {}

void FairCopyField::SetText(const String & text) {
	_text = text;
	std::u16string u16Text = _text.toUTF16();
	_glyphs.clear();
	for each (char16_t c in u16Text) {
		_glyphs.push_back(_font.renderChar(c));
	}
}

void FairCopyField::Update() {
	DevicePos origin(_size.x - 20, 0);
	CharPos charPos(0, 0);
	double lineInterval = 30;
	double lineHeight = _size.y;

	RectF(_pos, _size).draw(Palette::White);
	for each (Font::Glyph glyph in _glyphs) {
		DevicePos pos = origin + charPos.ToDeviceDelta(lineInterval, lineHeight);
		glyph.draw(pos);
		charPos = charPos.Add(glyph.GetAdvance(), lineHeight);
	}
}

}
}