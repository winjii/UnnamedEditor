#include "stdafx.h"
#include "WholeView.h"


namespace UnnamedEditor {
namespace WholeView {


WholeView::WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::Font> font)
: _pos(pos)
, _size(size)
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

	RectF(_pos, _size).draw(Palette::White);

	Vec2 pen(_size.x + _pageCount*_size.x/2.0, 0);
	for each (auto g in _glyphs) {
		if (pen.x < 0) break;
		if ((pen + g->getAdvance()).y > _size.y) pen = Vec2(pen.x - _lineInterval, 0);
		if (_size.x + _font->getFontSize()*2 < pen.x) {
			pen += g->getAdvance();
			continue;
		}
		pen = g->draw(pen);
	}
}


}
}