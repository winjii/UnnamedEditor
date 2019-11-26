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
	_textWindow.insertText(_textWindow.cursor(), text);
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

	if (!_textWindow.isEditing() && (addend.size() > 0 || editing.size() > 0)) {
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

	_area.draw(Palette::White);
	auto itr = _textWindow.drawStart();
	if (!isFloating || !_floatingArrangement->lowerTextArea(_floatingArrangement->drawStart())) {
		while (itr.first != _textWindow.endConstraints() && _textWindow.onTextArea(itr)) {
			if (isFloating && itr.first == _floatingArrangement->beginConstraints()) break;
			itr.first->glyph->draw(*(itr.second));
			drawCursor(itr);
			itr = _textWindow.nextExtended(itr);
		}
	}
	if (isFloating) {
		auto fitr = _floatingArrangement->drawStart();
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

GlyphArrangement::Iterator GlyphArrangement::prevForce(Iterator itr) {
	if (itr.second == _pos.begin()) _pos.push_front(Vec2());
	return { _text->prev(itr.first), --itr.second };
}

GlyphArrangement::Iterator GlyphArrangement::nextForce(Iterator itr) {
	if (itr.second == --_pos.end()) _pos.push_back(Vec2());
	return { _text->next(itr.first), ++itr.second };
}

GlyphArrangement::GlyphArrangement(SP<const Text> text, const RectF& area, double lineInterval, Vec2 originPos)
: _text(text)
, _area(area)
, _lineInterval(lineInterval)
, _pos()
, _begin()
, _end()
, _drawStart()
, _beginConstraints(text->begin())
, _endConstraints(text->end()) {
	auto [nlhead, cnt] = _text->nextLineHead(_beginConstraints);
	_pos = decltype(_pos)(cnt, Vec2());
	_begin = _drawStart = { _beginConstraints, _pos.begin() };
	_end = { nlhead, _pos.end() };
	arrange(_begin, _end, originPos);
}

GlyphArrangement::GlyphArrangement(GlyphArrangement& ga, Iterator beginConstraints)
: _text(ga._text)
, _area(ga._area)
, _lineInterval(ga._lineInterval)
, _pos()
, _begin()
, _end()
, _drawStart()
, _beginConstraints(beginConstraints.first)
, _endConstraints(ga._endConstraints) {
	auto [lhead, cnt] = ga.lineHead(beginConstraints);
	Iterator nlhead = ga.nextLineHead(beginConstraints).first;
	_pos = { lhead.second, nlhead.second };
	_begin = _drawStart = { _beginConstraints, std::next(_pos.begin(), cnt) };
	_end = { nlhead.first, _pos.end() };
}

GlyphArrangement::Iterator GlyphArrangement::next(Iterator itr) {
	if (itr.first == _endConstraints) throw "out of range";
	return nextForce(itr);
}

GlyphArrangement::Iterator GlyphArrangement::prev(Iterator itr) {
	if (itr.first == _beginConstraints) throw "out of range";
	return prevForce(itr);
}

GlyphArrangement::Iterator GlyphArrangement::advanced(Iterator itr, int d) {
	if (0 <= d) {
		for (int i = 0; i < d; i++) itr = next(itr);
	}
	else {
		for (int i = 0; i < -d; i++) itr = prev(itr);
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

std::pair<TextWindow::Iterator, int> GlyphArrangement::lineHead(Iterator itr) {
	int ret = 0;
	while (true) {
		if (itr.first == _text->begin()) break;
		Iterator itr_ = prevForce(itr);
		if (_text->isNewline(itr_.first)) break;
		ret++;
		itr = itr_;
	}
	return { itr, ret };
}

std::pair<TextWindow::Iterator, int> GlyphArrangement::nextLineHead(Iterator itr) {
	int ret = 0;
	while (true) {
		if (itr.first == _text->end()) break;
		bool flg = _text->isNewline(itr.first);
		itr = nextForce(itr);
		ret++;
		if (flg) break;
	}
	return { itr, ret };
}

GlyphArrangement::Iterator GlyphArrangement::drawStart() {
	while (_drawStart.first != _beginConstraints && !lowerTextArea(_drawStart))
		_drawStart = prevExtended(_drawStart);
	while (_drawStart.first != _endConstraints && lowerTextArea(_drawStart))
		_drawStart = nextExtended(_drawStart);
	return _drawStart;
}

GlyphArrangement::Iterator GlyphArrangement::prevExtended(Iterator itr) {
	if (itr.first == _beginConstraints) throw "out of range";
	if (itr != _begin) return prev(itr); //グリフ位置キャッシュ済み

	Iterator prv = prev(itr);
	Iterator lhead = lineHead(prv).first;
	arrange(lhead, itr, Vec2(0, 0));
	double d = (itr.second->x + _lineInterval) - prv.second->x;
	std::transform(_pos.begin(), itr.second, _pos.begin(), [d](Vec2 v) { return v + Vec2(d, 0); });
	_begin = lhead;
	return prv;
}

GlyphArrangement::Iterator GlyphArrangement::nextExtended(Iterator itr) {
	if (itr.first == _endConstraints) throw "out of range";
	if (itr.first == _text->prev(_endConstraints) || itr != prev(_end))
		return next(itr); //テキスト終端 or グリフ位置キャッシュ済み

	Iterator nxt = next(itr);
	Iterator nlhead = nextLineHead(nxt).first;
	Vec2 origin = Vec2(itr.second->x - _lineInterval, _area.y);
	arrange(nxt, nlhead, origin);
	_end = nlhead;
	return nxt;
}

GlyphArrangement::Iterator GlyphArrangement::prevExtended(Iterator itr, int cnt) {
	for (int i = 0; i < cnt; i++) itr = prevExtended(itr);
	return itr;
}

GlyphArrangement::Iterator GlyphArrangement::nextExtended(Iterator itr, int cnt) {
	for (int i = 0; i < cnt; i++) itr = nextExtended(itr);
	return itr;
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
TextWindow::Iterator TextWindow::editText(Iterator destroyed, std::function<Iterator()> edit) {
	Vec2 origin = *(lineHead(destroyed).first).second; //先に座標とっておかないとdestroyed==lineHeadだった場合にどっかいっちゃうことがある
	auto edited = edit();
	auto lhead = lineHead(edited).first;
	auto nlhead = nextLineHead(edited).first;
	arrange(lhead, nlhead, origin);

	if (destroyed == _begin) _begin = edited;
	if (destroyed.first == _beginConstraints)
		_beginConstraints = edited.first;
	if (destroyed == _cursor) _cursor = edited;
	_drawStart = _begin = lhead;
	_end = nlhead;
	return edited;
}

TextWindow::Iterator TextWindow::insertText(Iterator itr, const String &s) {
	auto edit = [&]() {
		return Iterator(_text->insert(itr.first, s), _pos.insert(itr.second, s.size(), Vec2()));
	};
	return editText(itr, edit);
}

TextWindow::Iterator TextWindow::eraseText(Iterator first, Iterator last) {
	auto edit = [&]() {
		return Iterator(_text->erase(first.first, last.first), _pos.erase(first.second, last.second));
	};
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
	setCursor(eraseText(unsettledBegin(), next(_cursor))); // ゼロ幅スペースも消す
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
	if (!_isEditing) return;
	Iterator uBegin = editedBegin();
	auto edit = [&]() {
		auto itr = _text->erase(editedBegin().first, _cursor.first);
		auto pitr = _pos.erase(editedBegin().second, _cursor.second);
		_unsettledCount += addend.size();
		_editedCount = editing.size();
		return Iterator(_text->insert(itr, addend + editing), _pos.insert(pitr, addend.size() + editing.size(), Vec2()));
	};
	_cursor = nextExtended(editText(uBegin, edit), _unsettledCount + _editedCount);
}

}
}