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

Vec2 WholeView::floatingTextIn(Vec2 source, Vec2 target, double t, int i)
{
	return EaseOut(Easing::Back, source, target, t);
}

Vec2 WholeView::floatingTextOut(Vec2 source, Vec2 target, double t, int i)
{
	double rate = EaseOut(Easing::Expo, 3.0, 1.0, std::min(1.0, i / 200.0));
	t = std::min(1.0, t*rate);

	double d1 = source.y - _pos.y;
	double d2 = _size.y;
	double d3 = _pos.y + _size.y - target.y;
	double delta = EaseOut(Easing::Quint, t) * (d1 + d2 + d3);
	if (delta <= d1) return Vec2(source.x, source.y - delta);
	delta -= d1;
	if (delta <= d2) return Vec2((source.x + target.x) / 2.0, _pos.y + _size.y - delta);
	delta -= d2;
	return Vec2(target.x, _pos.y + _size.y - delta);
}

WholeView::WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font)
: _borderPos(pos)
, _borderSize(size)
, _pos(pos + Vec2(0, font->getFontSize()*2))
, _size(size - Vec2(0, font->getFontSize()*2*2))
, _startPos(_pos + Vec2(_size.x, 0))
, _font(font)
, _lineInterval(font->getFontSize()*1.2)
, _ju()
, _floatingAnimation() {
	throw "全部初期化すべし";
}

void WholeView::setText(const String &text) {
	_text.clear();
	for (char16_t c : text) {
		_text.push_back(c);
	}
	auto res = _font->renderString(text.toUTF16());
	_glyphs.assign(res.begin(), res.end());
}

void WholeView::update() {
	String updated, unsettled;
	TextInput::UpdateText(updated);
	unsettled = TextInput::GetEditingText();
	_ju.update(unsettled.length());

	int cursorDelta = 0;
	if (KeyDown.down()) cursorDelta++;
	if (KeyUp.down()) cursorDelta--;

	bool deleted = _ju.isSettled() && KeyBackspace.down();

	if (!_floatingAnimation &&
		(updated.size() > 0 || unsettled.size() > 0)) {
		_normalGlyphPos.resize(0);
		_floatingGlyphPos.resize(0);
		Vec2 pen = _startPos;
		for (auto itr = _startItr; itr != _text.end(); itr++) {
			//画面外に出てしまったらfor文を終了
			if (pen.x < _pos.x) break;

			if (i >= _cursorIndex) {
				_normalGlyphPos.push_back(pen);
				_floatingGlyphPos.push_back(pen - Vec2(_lineInterval*2, 0));
			}
			pen += g->getAdvance();
			if (pen.y > _pos.y + _size.y) pen = Vec2(pen.x - _lineInterval, _pos.y);
		}
		auto f = [&](Vec2 vs, Vec2 vt, double t, int i) { return floatingTextIn(vs, vt, t, i); };
		_glyphAnimation = SP<Animation>(new Animation(_normalGlyphPos, _floatingGlyphPos, 1.5, f));
		_glyphAnimation->start();
	}
	bool isFloating = _floatingAnimation && _floatingAnimation->getState() != Animation::State::Inactive;
	if (isFloating) _floatingAnimation->update();

	RectF(_borderPos, _borderSize).draw(Palette::White);
	if (_normalArrangement.originIndex() < _floatingArrangement.originIndex()) {
		auto posItr = _normalArrangement.begin();
		auto textItr = _normalArrangement.origin();
		while (_pos.x + _size.x <= posItr->x) {
			posItr = _normalArrangement.ensureNext(posItr);
			textItr = _text.next(textItr);
		}
		_normalArrangement.rejectFront(posItr);
		while (posItr != _floatingArrangement.begin() && posItr != _normalArrangement.end()) {
			if (posItr->x < _pos.x) break;
			textItr->glyph->draw(*posItr);
			posItr = _normalArrangement.ensureNext(posItr);
			textItr = _text.next(textItr);
		}
	}
	if (isFloating) {
		auto posItr = _floatingArrangement.begin();
		auto textItr = _floatingArrangement.origin();
		while (_pos.x < posItr->x) {
			if (posItr->x < _pos.x + _size.x)
				textItr->glyph->draw(*posItr);
			posItr = _floatingArrangement.ensureNext(posItr);
			textItr = _text.next(textItr);
		}
	}
	/*{
		Vec2 c = getCharPos(_cursorIndex);
		Vec2 a(_lineInterval/2, 0);
		double t = Scene::Time()*2*Math::Pi/2.5;
		Line(c - a, c + a).draw(1, Color(Palette::Black, (Sin(t) + 1)/2*255));
	}*/
}

}
}