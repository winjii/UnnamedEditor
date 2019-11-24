#include "stdafx.h"
#include "WholeView.h"


namespace UnnamedEditor {
namespace WholeView {

/*
//TODO: size�������Ǝ擾
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

	double d1 = source.y - _area.y;
	double d2 = _area.h;
	double d3 = _area.y + _area.h - target.y;
	double delta = EaseOut(Easing::Quint, t) * (d1 + d2 + d3);
	if (delta <= d1) return Vec2(source.x, source.y - delta);
	delta -= d1;
	if (delta <= d2) return Vec2((source.x + target.x) / 2.0, _area.y + _area.h - delta);
	delta -= d2;
	return Vec2(target.x, _area.y + _area.h - delta);
}

//TODO: �ŏ�����Rect�Ŏ󂯎��
WholeView::WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font)
: _area()
, _font()
, _textWindow()
, _floatingArrangement()
, _cursor()
, _lineInterval()
, _ju()
, _floatingStep()
, _floatingProgress()
, _text() {
	throw "�S�����������ׂ�";
}

void WholeView::setText(const String &text) {

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

	_floatingProgress.update();
	if (_floatingProgress.getStep() == AnimationProgress::Step::Stable) {
		if (_floatingStep == FloatingStep::AnimatingIn)
			_floatingStep = FloatingStep::Stable;
		else if (_floatingStep == FloatingStep::AnimatingOut)
			_floatingStep == FloatingStep::Inactive;
			_floatingArrangement.disable();
	}
	bool isFloating = _floatingStep != FloatingStep::Inactive;

	if (_floatingStep != FloatingStep::AnimatingOut &&
		_floatingStep != FloatingStep::Inactive &&
		_text.next(_cursor.first) == _floatingArrangement.beginExtended().first &&
		(KeyDown.down() || KeyUp.down())) {
		// Floating�I��
		_floatingStep = FloatingStep::AnimatingOut;
		_floatingProgress.start(1);
	}

	if (KeyDown.down()) _cursor = _textWindow.nextExtended(_cursor);
	if (KeyUp.down()) _cursor = _textWindow.prevExtended(_cursor);

	if (_floatingStep != FloatingStep::AnimatingIn &&
		_floatingStep != FloatingStep::Stable &&
		(updated.size() > 0 || unsettled.size() > 0)) {
		Text::Iterator origin = _cursor.first;
		_cursor = _textWindow.insertText(_cursor, { ZERO_WIDTH_SPACE });
		//TODO: ���͂𒆓r���[�Ȉʒu�Őؒf����Ƌ֑������Ȃǂɂ����Floating�O�ƌ�Ŕz�u���ς��\���H��GlyphArrangement���ŋ֑������ɓ�����Ƃ��ɓK���ɓs���̂����Ƃ���܂Ńe�L�X�g�ǂ߂�
		//TODO: _floatingArrangement������
		_floatingStep = FloatingStep::AnimatingIn;
		_floatingProgress.start(1.5);
	}

	auto drawCursor = [&](const GlyphArrangement::Iterator& itr) {
		if (itr.first != _cursor.first) return;
		Vec2 c = *(itr.second);
		Vec2 a(_lineInterval / 2, 0);
		double t = Scene::Time() * 2 * Math::Pi / 2.5;
		Line(c - a, c + a).draw(1, Color(Palette::Black, (Sin(t) + 1) / 2 * 255));
	};

	_textWindow.fitBegin();

	_area.draw(Palette::White);
	auto itr = _textWindow.beginExtended();
	if (!isFloating || !_floatingArrangement.lowerTextArea(_floatingArrangement.beginExtended())) {
		while (itr.first != _floatingArrangement.beginExtended().first && _textWindow.onTextArea(itr)) {
			itr.first->glyph->draw(*(itr.second));
			drawCursor(itr);
			itr = _textWindow.nextExtended(itr);
		}
	}
	if (isFloating) {
		auto fitr = _floatingArrangement.beginExtended();
		int cnt = 0;
		while (fitr != _floatingArrangement.end() && !_floatingArrangement.upperTextArea(fitr)) {
			if (_floatingArrangement.onTextArea(fitr)) {
				Vec2 p; 
				double t = _floatingProgress.getProgress();
				if (_floatingStep == FloatingStep::AnimatingOut) {
					p = floatingTextIn(*(fitr.second), *(fitr.second) - Vec2(_lineInterval*2, 0), t, cnt);
				}
				else {
					p = floatingTextOut(*(itr.second), *(fitr.second), t, cnt);
				}
				fitr.first->glyph->draw(p);
			}
			drawCursor(fitr);
			fitr = _floatingArrangement.nextExtended(fitr);
			itr = _textWindow.nextExtended(itr);
			cnt++;
		}
	}
}

Text::Text(SP<Font::FixedFont> font)
: _data()
, _font(font) {
}

Text::Iterator Text::begin() const {
	return _data.begin();
}

Text::Iterator Text::end() const {
	return _data.end();
}

Text::Iterator Text::next(Iterator itr) const {
	return ++itr;
}

Text::Iterator Text::prev(Iterator itr) const {
	return --itr;
}

Text::Iterator Text::insert(Iterator itr, String s) {
	for (auto c : s.reversed()) {
		CharData cd;
		cd.code = c;
		cd.glyph = _font->renderChar(c);
		itr = _data.insert(itr, cd);
	}
	return itr;
}

Text::Iterator Text::erase(Iterator first, Iterator last) {
	return _data.erase(first, last);
}

bool Text::isNewline(Iterator itr) const
{
	//TODO: �F�X�ȉ��s�ɑΉ�����
	//����I�ȓ����\���ɕϊ����Ă��܂��������y�H
	return itr->code == U'\n';
}

std::pair<Text::Iterator, int> Text::lineHead(Iterator itr) const
{
	int ret = 0;
	while (true) {
		if (itr == begin()) break;
		Iterator itr_ = prev(itr);
		if (isNewline(itr_)) break;
		ret++;
		itr = itr_;
	}
	return { itr, ret };
}

std::pair<Text::Iterator, int> Text::nextLineHead(Iterator itr) const
{
	int ret = 0;
	while (true) {
		if (itr == end()) break;
		Iterator itr_ = next(itr);
		if (isNewline(itr_)) break;
		ret++;
		itr = itr_;
	}
	return { itr, ret };
}

void GlyphArrangement::arrange(Iterator first, Iterator last, Vec2 origin) {
	Vec2 pen = origin;
	Iterator itr = first;
	while (itr != last) {
		*itr.second = pen;
		pen += itr.first->glyph->getAdvance();
		if (_text.isNewline(itr.first) || _area.y + _area.h < pen.y) {
			pen = Vec2(pen.x - _lineInterval, _area.y);
		}
		itr.first = _text.next(itr.first);
		itr.second++;
	}
}

GlyphArrangement::Iterator GlyphArrangement::end() const {
	return _end;
}

bool GlyphArrangement::onTextArea(Iterator itr) const {
	return !upperTextArea(itr) && !lowerTextArea(itr);
}

bool GlyphArrangement::upperTextArea(Iterator itr) const {
	Vec2 p = *itr.second;
	return p.x < _area.x - _lineInterval;
}

bool GlyphArrangement::lowerTextArea(Iterator itr) const {
	Vec2 p = *itr.second;
	return _area.x + _area.w + _lineInterval < p.x;
}

void GlyphArrangement::scroll(double delta) {
	for (Vec2& p : _pos) {
		p += Vec2(-delta, 0);
	}
}

void GlyphArrangement::disable() {
	_pos.clear();
	_begin = _end = std::make_pair(_text.end(), _pos.end());
}

GlyphArrangement::Iterator GlyphArrangement::prevExtended(Iterator itr) {
	if (itr.first == _text.begin()) throw "out of range";
	if (itr.second != _pos.begin()) return { _text.prev(itr.first), --itr.second }; //�O���t�ʒu�L���b�V���ς�

	auto [lhead, cnt] = _text.lineHead(_text.prev(itr.first));
	_pos.insert(_pos.begin(), cnt, Vec2());
	Iterator first(lhead, _pos.begin());
	arrange(first, itr, Vec2(0, 0));
	Iterator prev(_text.prev(itr.first), std::prev(itr.second));
	Vec2 d = (*itr.second + Vec2(_lineInterval, 0)) - *prev.second;
	for (int i = 0; i < cnt + 1; i++) {
		_pos[i] += d;
	}
	return prev;
}

GlyphArrangement::Iterator GlyphArrangement::nextExtended(Iterator itr) {
	if (itr.first == _text.end()) throw "out of range";
	if (itr.second != _pos.end()) return { _text.next(itr.first), ++itr.second }; //�O���t�ʒu�L���b�V���ς�

	auto [nlhead, cnt] = _text.nextLineHead(_text.next(itr.first));
	Vec2 origin = Vec2(itr.second->x - _lineInterval, _area.y);
	Iterator first(_text.next(itr.first), _pos.end());
	_pos.insert(_pos.end(), cnt, origin);
	Iterator last(nlhead, _pos.end());
	arrange(first, last, origin);
}

GlyphArrangement::Iterator GlyphArrangement::prevExtended(Iterator itr, int cnt) {
	for (int i = 0; i < cnt; i++) itr = prevExtended(itr);
	return itr;
}

GlyphArrangement::Iterator GlyphArrangement::nextExtended(Iterator itr, int cnt) {
	for (int i = 0; i < cnt; i++) itr = nextExtended(itr);
	return itr;
}

GlyphArrangement::Iterator GlyphArrangement::beginExtended() {
	if (_begin == _end && _begin.first != _text.end()) {
		_end.first = _text.next(_end.first);
		_end.second++;
	}
	return _begin;
}

void GlyphArrangement::fitBegin() {
	while (_begin.first != _text.begin() && !lowerTextArea(_begin)) {
		_begin = prevExtended(_begin);
	}
	while (_begin.first != _text.end() && lowerTextArea(_begin)) {
		if (_begin == _end) _begin = _end = nextExtended(_begin);
		_begin = nextExtended(_begin);
	}
}

TextWindow::Iterator TextWindow::insertText(Iterator itr, String s) {
	auto [lhead, d0] = _text.lineHead(itr.first);
	Vec2 origin = *(itr.second - d0);
	_pos.erase(itr.second - d0, _pos.end());
	auto inserted = _text.insert(itr.first, s);
	auto [nlhead, d1] = _text.nextLineHead(inserted);
	_pos.insert(_pos.end(), d0 + d1, Vec2());
	arrange({ lhead, _pos.end() - d0 - d1 }, { nlhead, _pos.end() }, origin);
}

}
}