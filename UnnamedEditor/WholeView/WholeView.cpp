#include "stdafx.h"
#include "WholeView.h"
#include <iterator>
#include <optional>


namespace UnnamedEditor {
namespace WholeView {


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
, _textWindow([&]() {
	RectF r(_area.pos + _font->getFontSize() * Vec2(1, 1), _area.size - 2 * _font->getFontSize() * Vec2(1, 1));
	return std::move(TextWindow(SP<Text::Text>(new Text::Text(font)), r, _lineInterval, r.tr()));
}())
, _floatingArrangement()
, _ju()
, _floatingStep(FloatingStep::Inactive)
, _floatingProgress()
, _text(_textWindow.text()) {
}

void WholeView::setText(const String &text) {
	_textWindow.insertText(_textWindow.cursor(), text, true);
}

void WholeView::draw() {
	String addend, editing;
	TextInput::UpdateText(addend);
	editing = TextInput::GetEditingText();
	_ju.update(editing.length());

	if (_floatingStep == FloatingStep::Stable || _floatingStep == FloatingStep::AnimatingIn && (KeyDown.down() || KeyUp.down())) {
		// Floating終了を開始
		_floatingStep = FloatingStep::AnimatingOut;
		_floatingProgress.start(1);
	}
	

	if (KeyDown.down()) _textWindow.cursorNext();
	if (KeyUp.down()) _textWindow.cursorPrev();

	if (!_textWindow.isEditing() && (addend.size() > 0 || editing.size() > 0)) {
		_floatingArrangement = _textWindow.startEditing();
		_floatingStep = FloatingStep::AnimatingIn;
		_floatingProgress.start(1.5);
	}

	auto drawCursor = [&](const GlyphArrangement::Iterator& itr) {
		if (itr.first != _textWindow.cursor().first) return;
		Vec2 c = *(itr.second);
		int cnt = _textWindow.text()->idx(itr.first);
		Vec2 a(_lineInterval / 2, 0);
		double t = Scene::Time() * 2 * Math::Pi / 2.5;
		Line(c - a, c + a).draw(1, Color(Palette::Black, (Sin(t) + 1) / 2 * 255));
	};

	if (_floatingStep == FloatingStep::Inactive || _floatingStep == FloatingStep::AnimatingOut) {
		if (KeyLeft.down()) _scrollDelta.startScroll(1);
		if (KeyRight.down()) _scrollDelta.startScroll(-1);
	}
	if (_scrollDelta.step() == ScrollDelta::Step::Scrolling) {
		if (_scrollDelta.direction() == 1 && KeyLeft.up()) {
			_scrollDelta.stopScroll();
		}
		if (_scrollDelta.direction() == -1 && KeyRight.up()) {
			_scrollDelta.stopScroll();
		}
	}

	_floatingProgress.update();
	if (_floatingProgress.getStep() == AnimationProgress::Step::Stable) {
		if (_floatingStep == FloatingStep::AnimatingIn)
			_floatingStep = FloatingStep::Stable;
		else if (_floatingStep == FloatingStep::AnimatingOut) {
			_textWindow.stopEditing();
			_floatingStep = FloatingStep::Inactive;
			_floatingArrangement = nullptr;
		}
	}
	bool isFloating = _floatingStep != FloatingStep::Inactive;


	if (_floatingStep == FloatingStep::AnimatingIn || _floatingStep == FloatingStep::Stable)
		_textWindow.inputText(addend, editing);
	if (_scrollDelta.step() == ScrollDelta::Step::Scrolling)
		_textWindow.scroll(_scrollDelta.useDelta());

	_area.draw(Palette::White);
	auto itr = _textWindow.calcDrawBegin();
	auto debug = itr;
	auto twEnd = _textWindow.calcDrawEnd();
	std::optional<GlyphArrangement::Iterator> fBegin;
	if (isFloating)
		fBegin = _floatingArrangement->calcDrawBegin();
	while (true) {
		drawCursor(itr);
		if (itr == twEnd) break;
		if (isFloating && itr.first == fBegin.value().first) break;
		itr.first->glyph->draw(*(itr.second));
		itr = _textWindow.nextExtended(itr);
	}
	if (isFloating) {
		auto fitr = fBegin.value();
		auto fEnd = _floatingArrangement->calcDrawEnd();
		int cnt = 0;
		while (true) {
			drawCursor(fitr);
			if (fitr == fEnd) break;
			Vec2 p;
			double t = _floatingProgress.getProgress();
			if (_floatingStep == FloatingStep::AnimatingOut) {
				p = floatingTextOut(*(fitr.second), *(itr.second), t, cnt);
			}
			else {
				p = floatingTextIn(*(fitr.second), *(fitr.second) - Vec2(_lineInterval * 2, 0), t, cnt);
			}
			fitr.first->glyph->draw(p);
			fitr = _floatingArrangement->nextExtended(fitr);
			itr = _textWindow.nextExtended(itr);
			cnt++;
		}
	}
}

GlyphArrangement::GlyphArrangement(SP<Text::Text> text, const RectF& area, double lineInterval, Vec2 originPos)
: _text(text)
, _area(area)
, _lineInterval(lineInterval)
, _pos()
, _cacheBegin()
, _cacheEnd()
, _begin()
, _end() {
	_begin = _text->beginSentinel();
	_end = _text->endSentinel();
	auto [nlhead, cnt] = _text->nextLineHead(_text->next(_begin));
	_pos = decltype(_pos)(cnt + 2, Vec2());
	_cacheBegin = { _begin, _pos.begin() };
	_cacheEnd = { nlhead, std::prev(_pos.end()) };
	*next(_cacheBegin).second = originPos;
	arrange(next(_cacheBegin), _cacheEnd);
}

GlyphArrangement::GlyphArrangement(GlyphArrangement& ga, Iterator begin)
: _text()
, _area(ga._area)
, _lineInterval(ga._lineInterval)
, _pos()
, _cacheBegin()
, _cacheEnd()
, _begin()
, _end() {
	auto lheadOld = ga.lineHead(begin).first;
	auto nlheadOld = ga.nextLineHead(begin).first;
	_text.reset(new Text::TextWithHead(*ga._text, begin.first, lheadOld.first));
	_begin = _text->prev(begin.first);
	_end = _text->endSentinel();
	_pos = std::list<Vec2>(lheadOld.second, std::next(nlheadOld.second));
	_pos.push_front(Vec2());
	auto lhead = _text->lineHead(begin.first).first;
	auto nlhead = _text->nextLineHead(begin.first).first;
	_cacheBegin = { _text->prev(lhead), _pos.begin() };
	_cacheEnd = { nlhead, std::prev(_pos.end()) };
}

//TODO: 実装（NULL文字が増殖するだけで致命的なバグが起こるわけではないので後回し）
GlyphArrangement::~GlyphArrangement() {
	_pos.clear();
}

GlyphArrangement::Iterator GlyphArrangement::next(Iterator itr) {
	assert(itr.first != _text->endSentinel());
	if (itr.second == --_pos.end()) _pos.push_back(Vec2());
	return { _text->next(itr.first), ++itr.second };
}

GlyphArrangement::Iterator GlyphArrangement::prev(Iterator itr) {
	assert(itr.first != _text->beginSentinel());
	if (itr.second == _pos.begin()) _pos.push_front(Vec2());
	return { _text->prev(itr.first), --itr.second };
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

//TODO: NULL文字は_fontに渡さずに処理したほうが良いかも
//TODO: (first, last]にする
void GlyphArrangement::arrange(Iterator first, Iterator last) {
	Vec2 pen = *first.second;
	Iterator itr = first;
	while (itr != last) {
		pen += itr.first->glyph->getAdvance();
		if (_text->isNewline(itr.first) || _area.y + _area.h < pen.y) {
			pen = Vec2(pen.x - _lineInterval, _area.y);
		}
		itr.first = _text->next(itr.first);
		itr.second++;
		*itr.second = pen;
	}
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

std::pair<TextWindow::Iterator, int> GlyphArrangement::lineHead(Iterator itr) {
	auto [lhead, cnt] = _text->lineHead(itr.first);
	return { advanced(itr, -cnt), cnt };
}

std::pair<TextWindow::Iterator, int> GlyphArrangement::nextLineHead(Iterator itr) {
	auto [nlhead, cnt] = _text->nextLineHead(itr.first);
	return { advanced(itr, cnt), cnt };
}

GlyphArrangement::Iterator GlyphArrangement::calcDrawBegin() {
	Iterator ret = _cacheEnd;
	while (true) {
		Iterator prv = prevExtended(ret);
		if (prv.first == _begin || lowerTextArea(prv)) break;
		ret = prv;
	}
	while (ret.first != _end && lowerTextArea(ret))
		ret = nextExtended(ret);
	Iterator lhead = lineHead(ret).first;
	_pos.erase(_pos.begin(), lhead.second);
	_cacheBegin = prev(lhead);
	return ret;
}

GlyphArrangement::Iterator GlyphArrangement::calcDrawEnd() {
	Iterator ret = next(_cacheBegin);
	while (ret.first != _end && !upperTextArea(ret))
		ret = nextExtended(ret);
	while (true) {
		Iterator prv = prevExtended(ret);
		if (prv.first == _begin || !upperTextArea(prv)) break;
		ret = prv;
	}
	Iterator nlhead = nextLineHead(prev(ret)).first;
	_pos.erase(std::next(nlhead.second), _pos.end());
	_cacheEnd = nlhead;
	return ret;
}

GlyphArrangement::Iterator GlyphArrangement::prevExtended(Iterator itr) {
	assert(itr.first != _text->beginSentinel());
	if (itr != next(_cacheBegin) || _cacheBegin.first == _text->beginSentinel()) return prev(itr); //グリフ位置キャッシュ済み or キャッシュの必要なし

	Vec2 p = *itr.second;
	Iterator lhead = lineHead(_cacheBegin).first;
	_cacheBegin = prev(lhead);
	*lhead.second = Vec2(0, 0);
	arrange(lhead, itr);
	double d = p.x - itr.second->x;
	std::transform(_pos.begin(), std::next(itr.second), _pos.begin(), [d](Vec2 v) { return v + Vec2(d, 0); });
	return prev(itr);
}

GlyphArrangement::Iterator GlyphArrangement::nextExtended(Iterator itr) {
	assert(itr.first != _text->endSentinel());
	if (itr != _cacheEnd) return next(itr); //グリフ位置キャッシュ済み

	//itrは必ず行頭
	Iterator nlhead = nextLineHead(itr).first;
	_cacheEnd = nlhead;
	arrange(itr, nlhead);
	return next(itr);
}

GlyphArrangement::Iterator GlyphArrangement::prevExtended(Iterator itr, int cnt) {
	for (int i = 0; i < cnt; i++) itr = prevExtended(itr);
	return itr;
}

GlyphArrangement::Iterator GlyphArrangement::nextExtended(Iterator itr, int cnt) {
	for (int i = 0; i < cnt; i++) itr = nextExtended(itr);
	return itr;
}

TextWindow::TextWindow(SP<Text::Text> text, const RectF& area, double lineInterval, Vec2 originPos)
: GlyphArrangement(text, area, lineInterval, originPos)
, _text(text)
, _cursor()
, _editedCount(0)
, _unsettledCount(0)
, _isEditing(false) {
	_cursor = next(_cacheBegin);
}

//TODO: _posに全てinsertしてしまってるけどOK？
TextWindow::Iterator TextWindow::insertText(Iterator itr, const String &s, bool rearranges) {
	auto titr = _text->insert(itr.first, s);
	auto pitr = _pos.insert(itr.second, s.size(), *itr.second);
	Iterator ret(titr, pitr);
	if (rearranges) {
		_cacheEnd = nextLineHead(ret).first;
		//int cb = std::distance(_pos.begin(), _cacheBegin.second);
		//int ce = std::distance(_pos.begin(), _cacheEnd.second);
		arrange(lineHead(ret).first, _cacheEnd);
	}
	else {
		_cacheEnd = lineHead(itr).first;
	}
	return { titr, pitr };
}

TextWindow::Iterator TextWindow::eraseText(Iterator first, Iterator last, bool rearranges) {
	Vec2 p = *first.second;
	auto titr = _text->erase(first.first, last.first);
	auto pitr = _pos.erase(first.second, last.second);
	Iterator ret{ titr, pitr };
	*pitr = p;
	if (rearranges) {
		_cacheEnd = nextLineHead(ret).first;
		arrange(lineHead(ret).first, _cacheEnd);
	}
	else {
		_cacheEnd = lineHead(ret).first;
	}
	return ret;
}

SP<const Text::Text> TextWindow::text() {
	return _text;
}

UP<GlyphArrangement> TextWindow::startEditing() {
	assert(_isEditing == false);
	_isEditing = true;
	//floatingした文字列と元の文字列の間に隙間ができるので、"隙間へのカーソル"を表現するためにNULL文字を置いておく
	Vec2 p = *_cursor.second;
	Iterator floatingBegin = _cursor;
	_cursor = insertText(_cursor, {  Text::Text::NULL_CHAR }, false);
	*_cursor.second = p;
	return UP<GlyphArrangement>(new GlyphArrangement(*this, floatingBegin));
}

void TextWindow::stopEditing() {
	assert(_isEditing == true);
	_isEditing = false;
	_cursor = eraseText(_cursor, next(_cursor), false);
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
	eraseText(editedBegin(), _cursor, false);
	_unsettledCount += addend.size();
	_editedCount = editing.size();
	insertText(_cursor, addend + editing, true);
}

bool TextWindow::cursorNext() {
	if (_isEditing || _cursor.first == _end) return false;
	_cursor = nextExtended(_cursor);
	return true;
}

bool TextWindow::cursorPrev() {
	if (_isEditing) return false;
	Iterator prv = prev(_cursor);
	if (prv.first == _text->beginSentinel()) return false;
	_cursor = prv;
	return true;
}

}
}