#include "stdafx.h"
#include "WholeView.h"


namespace UnnamedEditor {
namespace WholeView {


WholeView::WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::Font> font)
: _borderPos(pos)
, _borderSize(size)
, _pos(pos + Vec2(0, font->getFontSize()*2))
, _size(size - Vec2(0, font->getFontSize()*2*2))
, _font(font)
, _pageCount(0)
, _lineInterval(font->getFontSize()*1.2) {
	
}

void WholeView::setText(const String &text) {
	_text = text;
	auto res = _font->renderString(text.toUTF16());
	_glyphs.assign(res.begin(), res.end());
}

void WholeView::update() {
	if (KeyLeft.down()) _pageCount++;
	else if (KeyRight.down()) _pageCount--;

	RectF(_borderPos, _borderSize).draw(Palette::White);

	Vec2 pen = _pos + Vec2(_size.x + _pageCount*_size.x/2.0 - _lineInterval, 0);
	for each (auto g in _glyphs) {
		if ((pen + g->getAdvance()).y > _pos.y + _size.y) pen = Vec2(pen.x - _lineInterval, _pos.y);
		if (_pos.x + _size.x + _font->getFontSize() < pen.x) {
			pen += g->getAdvance();
			continue;
		}
		if (pen.x < _pos.x) break;
		pen = g->draw(pen);
	}
}


}
}