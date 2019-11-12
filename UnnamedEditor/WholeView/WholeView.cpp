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

int WholeView::deleteChar(int cursorIndex) {
	if (cursorIndex <= 0 || _text.size() < cursorIndex) return cursorIndex;
	_text.erase(_text.begin() + cursorIndex - 1);
	_glyphs.erase(_glyphs.begin() + cursorIndex - 1);
	return cursorIndex - 1;
}

WholeView::WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font)
: _borderPos(pos)
, _borderSize(size)
, _pos(pos + Vec2(0, font->getFontSize()*2))
, _size(size - Vec2(0, font->getFontSize()*2*2))
, _font(font)
, _pageCount(0)
, _lineInterval(font->getFontSize()*1.2)
, _cursorIndex(0)
, _ju()
, _ft() {
	
}

void WholeView::setText(const String &text) {
	_text = text;
	auto res = _font->renderString(text.toUTF16());
	_glyphs.assign(res.begin(), res.end());
}

void WholeView::update() {
	String updated, unsettled;
	TextInput::UpdateText(updated);
	unsettled = TextInput::GetEditingText();
	_ju.update(unsettled.length());

	int cursorDelta = 0;
	if (KeyControl.pressed() && KeyLeft.down()) _pageCount++;
	if (KeyControl.pressed() && KeyRight.down()) _pageCount--;
	if (KeyDown.down()) cursorDelta++;
	if (KeyUp.down()) cursorDelta--;

	bool deleted = _ju.isSettled() && KeyBackspace.down();

	RectF(_borderPos, _borderSize).draw(Palette::White);

	auto drawer = [&](int idx, Vec2 pen) {
		if (_pos.x + _size.x + _font->getFontSize() < pen.x) return true;
		if (pen.x < _pos.x) return false;
		_glyphs[idx]->draw(pen);
		return true;
	};

	Vec2 pen = _pos + Vec2(_size.x + _pageCount*_size.x/2.0 - _lineInterval, 0);
	for (int i = 0; i < (int)_glyphs.size(); i++) {
		auto g = _glyphs[i];
		if ((pen + g->getAdvance()).y > _pos.y + _size.y) pen = Vec2(pen.x - _lineInterval, _pos.y);
		//画面内に到達していなければ描画しない
		if (_pos.x + _size.x + _font->getFontSize() < pen.x) {
			pen += g->getAdvance();
			continue;
		}
		//画面外に出てしまったらfor文を終了
		if (pen.x < _pos.x) break;

		pen = g->draw(pen);
	}
	{
		Vec2 c = getCharPos(_cursorIndex);
		Vec2 a(_lineInterval/2, 0);
		double t = Scene::Time()*2*Math::Pi/2.5;
		Line(c - a, c + a).draw(1, Color(Palette::Black, (Sin(t) + 1)/2*255));
	}
}

Vec2 WholeView::getCharPos(int charIndex) const {
	Vec2 pen = _pos + Vec2(_size.x + _pageCount*_size.x/2.0 - _lineInterval, 0);
	for (int i = 0; i < (int)_glyphs.size(); i++) {
		auto g = _glyphs[i];
		if ((pen + g->getAdvance()).y > _pos.y + _size.y) pen = Vec2(pen.x - _lineInterval, _pos.y);
		if (i == charIndex) return pen;
		pen += g->getAdvance();
	}
	return pen;
}

Vec2 WholeView::getCursorPos(int cursorIndex) const {
	return getCharPos(cursorIndex);
}

}
}