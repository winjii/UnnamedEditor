#include "stdafx.h"
#include "WholeView.h"


namespace UnnamedEditor {
namespace WholeView {

/*
//TODO: sizeをちゃんと取得
WholeView::TranslationIntoWorkspace::TranslationIntoWorkspace(const WholeView &wv, double endSize, const Vec2 &endHead)
: _startRect(wv._pos, wv._size)
, _startSize(wv._font->getFontSize())
, _endSize(endSize)
, _startHead(wv.getCharPos(wv._cursorIndex))
, _endHead(endHead)
, _lineInterval(wv._lineInterval) {
	
}*/

WholeView::WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font)
: _borderPos(pos)
, _borderSize(size)
, _pos(pos + Vec2(0, font->getFontSize()*2))
, _size(size - Vec2(0, font->getFontSize()*2*2))
, _font(font)
, _pageCount(0)
, _lineInterval(font->getFontSize()*1.2)
, _cursorIndex(0) {
	
}

void WholeView::setText(const String &text) {
	_text = text;
	auto res = _font->renderString(text.toUTF16());
	_glyphs.assign(res.begin(), res.end());
}

void WholeView::update() {
	if (KeyControl.pressed() && KeyLeft.down()) _pageCount++;
	else if (KeyControl.pressed() && KeyRight.down()) _pageCount--;
	else if (KeyDown.down()) _cursorIndex++;
	else if (KeyUp.down()) _cursorIndex--;

	RectF(_borderPos, _borderSize).draw(Palette::White);

	Vec2 pen = _pos + Vec2(_size.x + _pageCount*_size.x/2.0 - _lineInterval, 0);
	auto itr = _glyphs.begin();
	for (int i = 0; itr != _glyphs.end(); i++, itr++) {
		auto g = *itr;
		if ((pen + g->getAdvance()).y > _pos.y + _size.y) pen = Vec2(pen.x - _lineInterval, _pos.y);
		if (i == _cursorIndex) {
			_font->getCursor(pen).draw(2, Palette::Orange);
		}
		//画面内に到達していなければ描画しない
		if (_pos.x + _size.x + _font->getFontSize() < pen.x) {
			pen += g->getAdvance();
			continue;
		}
		//画面外に出てしまったらfor文を終了
		if (pen.x < _pos.x) break;

		pen = g->draw(pen);
	}
}

Vec2 WholeView::getCharPos(int charIndex) const {
	Vec2 pen = _pos + Vec2(_size.x + _pageCount*_size.x/2.0 - _lineInterval, 0);
	auto itr = _glyphs.begin();
	for (int i = 0; itr != _glyphs.end(); i++, itr++) {
		auto g = *itr;
		if ((pen + g->getAdvance()).y > _pos.y + _size.y) pen = Vec2(pen.x - _lineInterval, _pos.y);
		if (i == charIndex) return pen;
		pen += g->getAdvance();
	}
}

}
}