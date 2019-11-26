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

//TODO: 最初からRectで受け取る
WholeView::WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font)
: _area(pos, size)
, _lineInterval(font->getFontSize())
, _font(font)
, _textWindow(SP<Text>(new Text(font)), _area, _lineInterval, _area.tr())
, _floatingArrangement()
, _ju()
, _floatingStep(FloatingStep::Inactive)
, _floatingProgress()
, _text(_textWindow.text()) {
}

void WholeView::setText(const String &text) {
	_textWindow.insertText(_textWindow.beginExtended(), text);
}

void WholeView::draw() {
	String addend, editing;
	TextInput::UpdateText(addend);
	editing = TextInput::GetEditingText();
	_ju.update(editing.length());

	_floatingProgress.update();
	if (_floatingProgress.getStep() == AnimationProgress::Step::Stable) {
		if (_floatingStep == FloatingStep::AnimatingIn)
			_floatingStep = FloatingStep::Stable;
		else if (_floatingStep == FloatingStep::AnimatingOut) {
			_floatingStep == FloatingStep::Inactive;
			_textWindow.undetatch(std::move(_floatingArrangement));
			_floatingArrangement = nullptr;
		}
	}
	bool isFloating = _floatingStep != FloatingStep::Inactive;

	if (_textWindow.isEditing() && (KeyDown.down() || KeyUp.down())) {
		// Floating終了を開始
		_textWindow.stopEditing();
		_floatingStep = FloatingStep::AnimatingOut;
		_floatingProgress.start(1);
	}
	

	if (KeyDown.down()) _textWindow.setCursor(_textWindow.nextExtended(_textWindow.cursor()));
	if (KeyUp.down()) _textWindow.setCursor(_textWindow.prevExtended(_textWindow.cursor()));

	if (_textWindow.isEditing() && (addend.size() > 0 || editing.size() > 0)) {
		_textWindow.startEditing();
		_floatingArrangement = _textWindow.detachBack(_textWindow.cursor());
		_floatingStep = FloatingStep::AnimatingIn;
		_floatingProgress.start(1.5);
	}

	auto drawCursor = [&](const GlyphArrangement::Iterator& itr) {
		if (itr.first != _textWindow.cursor().first) return;
		Vec2 c = *(itr.second);
		Vec2 a(_lineInterval / 2, 0);
		double t = Scene::Time() * 2 * Math::Pi / 2.5;
		Line(c - a, c + a).draw(1, Color(Palette::Black, (Sin(t) + 1) / 2 * 255));
	};

	_textWindow.inputText(addend, editing);
	_textWindow.fitBegin();

	_area.draw(Palette::White);
	auto itr = _textWindow.beginExtended();
	if (!isFloating || !_floatingArrangement->lowerTextArea(_floatingArrangement->beginExtended())) {
		while (itr.first != _textWindow.endConstraints() && _textWindow.onTextArea(itr)) {
			if (isFloating && itr.first == _floatingArrangement->beginExtended().first) break;
			itr.first->glyph->draw(*(itr.second));
			drawCursor(itr);
			itr = _textWindow.nextExtended(itr);
		}
	}
	if (isFloating) {
		auto fitr = _floatingArrangement->beginExtended();
		int cnt = 0;
		while (fitr.first != _floatingArrangement->endConstraints() && !_floatingArrangement->upperTextArea(fitr)) {
			if (_floatingArrangement->onTextArea(fitr)) {
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
			fitr = _floatingArrangement->nextExtended(fitr);
			itr = _textWindow.nextExtended(itr);
			cnt++;
		}
	}
}

Text::Text(SP<Font::FixedFont> font)
: _data()
, _font(font) {
	CharData cd;
	cd.code = ZERO_WIDTH_SPACE;
	cd.glyph = font->renderChar(cd.code);
	_data.push_back(cd);
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
	//TODO: 色々な改行に対応する
	//統一的な内部表現に変換してしまった方が楽？
	return itr->code == NEWLINE;
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
		bool flg = isNewline(itr);
		itr = next(itr);
		ret++;
		if (flg) break;
	}
	return { itr, ret };
}

GlyphArrangement::GlyphArrangement(SP<const Text> text, const RectF& area, double lineInterval, Vec2 originPos)
: _text(text)
, _area(area)
, _lineInterval(lineInterval)
, _beginConstraints(text->begin())
, _endConstraints(text->end()) {
	bool flg = text->begin() == text->end();
	auto [nlhead, cnt] = _text->nextLineHead(_beginConstraints);
	_pos = decltype(_pos)(cnt, Vec2());
	_begin = { _beginConstraints, _pos.begin() };
	arrange(_begin, { nlhead, std::next( _begin.second, cnt) }, originPos);
}

GlyphArrangement::GlyphArrangement(const GlyphArrangement& ga, Iterator beginConstraints)
: _text(ga._text)
, _area(ga._area)
, _beginConstraints(beginConstraints.first)
, _endConstraints(ga._endConstraints)
, _lineInterval(ga._lineInterval) {
	auto [lhead, d0] = ga._text->lineHead(beginConstraints.first);
	auto [nlhead, d1] = ga._text->nextLineHead(beginConstraints.first);
	_pos = { std::prev(beginConstraints.second, d0), std::prev(beginConstraints.second, d1) };
	_begin = { lhead, _pos.begin() };

	//begin側がブロックの途中で切れるがbeginConstraintsによってbegin側の拡張が制限されるので問題ない
}

GlyphArrangement::Iterator GlyphArrangement::nextUnsafe(Iterator itr) {
	return { _text->next(itr.first), ++itr.second};
}

GlyphArrangement::Iterator GlyphArrangement::prevUnsafe(Iterator itr) {
	return { _text->prev(itr.first), --itr.second };
}

GlyphArrangement::Iterator GlyphArrangement::advancedUnsafe(Iterator itr, int d) {
	if (0 <= d) {
		for (int i = 0; i < d; i++) itr = nextUnsafe(itr);
	}
	else {
		for (int i = 0; i < -d; i++) itr = prevUnsafe(itr);
	}
	return itr;
}

void GlyphArrangement::arrange(Iterator first, Iterator last, Vec2 origin) {
	Vec2 pen = origin;
	Iterator itr = first;
	while (itr != last) {
		*itr.second = pen;
		pen += itr.first->glyph->getAdvance();
		if (_text->isNewline(itr.first) || _area.y + _area.h < pen.y) {
			pen = Vec2(pen.x - _lineInterval, _area.y);
		}
		itr.first = _text->next(itr.first);
		itr.second++;
	}
}

Text::Iterator GlyphArrangement::beginConstraints() const {
	return _beginConstraints;
}

Text::Iterator GlyphArrangement::endConstraints() const {
	return _endConstraints;
}

RectF GlyphArrangement::area() const {
	return _area;
}

double GlyphArrangement::lineInterval() const {
	return _lineInterval;
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
	_endConstraints = _beginConstraints;
}

GlyphArrangement::Iterator GlyphArrangement::prevExtended(Iterator itr) {
	if (itr.first == _beginConstraints) throw "out of range";
	if (itr.second != _pos.begin()) return { _text->prev(itr.first), --itr.second }; //グリフ位置キャッシュ済み

	auto [lhead, cnt] = _text->lineHead(_text->prev(itr.first));
	_pos.insert(_pos.begin(), cnt, Vec2());
	Iterator first(lhead, _pos.begin());
	arrange(first, itr, Vec2(0, 0));
	Iterator prev(_text->prev(itr.first), std::prev(itr.second));
	Vec2 d = (*itr.second + Vec2(_lineInterval, 0)) - *prev.second;
	std::transform(_pos.begin(), prev.second, _pos.begin(), [d](Vec2 v) { return v + d; });
	return prev;
}

GlyphArrangement::Iterator GlyphArrangement::nextExtended(Iterator itr) {
	if (itr.first == _endConstraints) throw "out of range";
	if (itr.second != --_pos.end()) return { _text->next(itr.first), ++itr.second }; //グリフ位置キャッシュ済み

	auto [nlhead, cnt] = _text->nextLineHead(_text->next(itr.first));
	Vec2 origin = Vec2(itr.second->x - _lineInterval, _area.y);
	_pos.insert(_pos.end(), cnt, origin);
	Iterator next(_text->next(itr.first), std::prev(_pos.end(), cnt));
	Iterator last(nlhead, _pos.end());
	arrange(next, last, origin);
	return next;
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
	return _begin;
}

void GlyphArrangement::fitBegin() {
	while (_begin.first != _beginConstraints && !lowerTextArea(_begin)) {
		_begin = prevExtended(_begin);
	}
	while (_begin.first != _endConstraints && lowerTextArea(_begin)) {
		_begin = nextExtended(_begin);
	}
}

TextWindow::TextWindow(SP<Text> text, const RectF& area, double lineInterval, Vec2 originPos)
: GlyphArrangement(text, area, lineInterval, originPos)
, _text(text)
, _cursor(_begin)
, _editedCount(0)
, _unsettledCount(0)
, _isEditing(false) {
}

//TODO: 無駄なイテレータ計算が多い
TextWindow::Iterator TextWindow::editText(Iterator destroyed, std::function<Text::Iterator()> edit) {
	bool changedB = destroyed.first == _begin.first;
	bool changedBC = destroyed.first == _beginConstraints;
	bool changedCursor = destroyed.first == _cursor.first;
	auto edited = edit();
	auto [lhead, d0] = _text->lineHead(edited);
	auto lheadPos = std::prev(destroyed.second, d0);
	Vec2 origin = *lheadPos;
	_pos.erase(lheadPos, _pos.end());
	auto [nlhead, d1] = _text->nextLineHead(edited);
	_pos.insert(_pos.end(), d0 + d1, Vec2());

	Iterator newItr(edited, std::prev(_pos.end(), d1));
	if (changedB) _begin = newItr; //begin.secondはeditで壊れるため
	if (changedBC) _beginConstraints = edited;
	if (changedCursor) _cursor = newItr;

	arrange({ lhead, std::prev(_pos.end(), d0 + d1) }, { nlhead, _pos.end() }, origin);
	return { edited, std::prev(_pos.end(), d1) };
}

TextWindow::Iterator TextWindow::insertText(Iterator itr, const String &s) {
	auto edit = [&]() { return _text->insert(itr.first, s); };
	return editText(itr, edit);
}

TextWindow::Iterator TextWindow::eraseText(Iterator first, Iterator last) {
	auto edit = [&]() { return _text->erase(first.first, last.first); };
	return editText(first, edit);
}

SP<const Text> TextWindow::text() {
	return _text;
}

UP<GlyphArrangement> TextWindow::detachBack(Iterator partition) {
	_endConstraints = partition.first;
	return UP<GlyphArrangement>(new GlyphArrangement(*this, partition));
}

void TextWindow::undetatch(UP<GlyphArrangement> ga) {
	if (ga->beginConstraints() == _endConstraints) {
		_endConstraints = ga->endConstraints();
	}
	else if (ga->endConstraints() == _beginConstraints) {
		_beginConstraints = ga->beginConstraints();
	}
	else throw "not undetachable";
}

bool TextWindow::setCursor(Iterator cursor) {
	if (_isEditing) return false;
	_cursor = cursor;
	return true;
}

void TextWindow::startEditing() {
	_isEditing = true;
	_cursor = insertText(_cursor, { ZERO_WIDTH_SPACE });
}

void TextWindow::stopEditing() {
	_isEditing = false;
	setCursor(eraseText(unsettledBegin(), nextUnsafe(_cursor))); // ゼロ幅スペースも消す
	_editedCount = _unsettledCount = 0;
}

bool TextWindow::isEditing() {
	return _isEditing;
}

TextWindow::Iterator TextWindow::cursor() {
	return _cursor;
}

TextWindow::Iterator TextWindow::editedBegin() {
	return prevExtended(_cursor, _editedCount);
}

TextWindow::Iterator TextWindow::unsettledBegin() {
	return prevExtended(_cursor, _unsettledCount + _editedCount);
}

void TextWindow::inputText(const String& addend, const String& editing) {
	Iterator uBegin = unsettledBegin();
	auto edit = [&]() {
		auto itr = _text->erase(editedBegin().first, _cursor.first);
		_unsettledCount += addend.size();
		_editedCount = editing.size();
		return _text->insert(itr, addend + editing);
	};
	_cursor = nextExtended(editText(uBegin, edit), _unsettledCount + _editedCount);
}

}
}