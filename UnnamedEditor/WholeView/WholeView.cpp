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
, _ju() {
	
}

void WholeView::setText(const String &text) {
	_text = text;
	auto res = _font->renderString(text.toUTF16());
	_glyphs.assign(res.begin(), res.end());
}

void WholeView::update() {
	String updated, unsettled;
	TextInput::UpdateText(updated);
	unsettled = TextInput::GetMarkedText();
	_ju.update(unsettled.length());

	int cursorDelta = 0;
	if (KeyControl.pressed() && KeyLeft.down()) _pageCount++;
	else if (KeyControl.pressed() && KeyRight.down()) _pageCount--;
	else if (KeyDown.down()) cursorDelta++;
	else if (KeyUp.down()) cursorDelta--;

	bool deleted = _ju.isSettled() && KeyBackspace.down();

	if ((unsettled.length() > 0 || deleted) && _insertMode == nullptr) {
		_insertMode.reset(new InsertMode(RectF(_pos, _size), _lineInterval, _cursorIndex, getCursorPos(_cursorIndex), _font));
	}
	if (_insertMode != nullptr) {
		if (!_insertMode->isActive() && !_insertMode->isAnimating()) _insertMode.reset();
		else _insertMode->update(updated, unsettled, cursorDelta, deleted);
	}
	if (_insertMode == nullptr) {
		_cursorIndex += cursorDelta;
	}

	RectF(_borderPos, _borderSize).draw(Palette::White);

	Vec2 pen = _pos + Vec2(_size.x + _pageCount*_size.x/2.0 - _lineInterval, 0);
	for (int i = 0; i < (int)_glyphs.size(); i++) {
		if (_insertMode != nullptr && i == _insertMode->getInsertIndex()) {
			pen = _insertMode->getNextPen();
		}
		auto g = _glyphs[i];
		if ((pen + g->getAdvance()).y > _pos.y + _size.y) pen = Vec2(pen.x - _lineInterval, _pos.y);
		if (_insertMode == nullptr && i == _cursorIndex) {
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
	if (_insertMode != nullptr) _insertMode->draw();
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