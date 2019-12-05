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

//TODO: 禁則処理などでareaの外に出るケースは考慮できていない
Vec2 WholeView::floatingTextOut(Vec2 source, Vec2 target, double t, int i)
{
	RectF area = _textArea;

	double rate = EaseOut(Easing::Expo, 3.0, 1.0, std::min(1.0, i / 200.0));
	t = std::min(1.0, t*rate);

	double d1 = source.y - area.y;
	double d2 = area.h;
	double d3 = area.y + area.h - target.y;
	double delta = EaseOut(Easing::Expo, t) * (d1 + d2 + d3);
	if (delta <= d1) return Vec2(source.x, source.y - delta);
	delta -= d1;
	if (delta <= d2) return Vec2((source.x + target.x) / 2.0, area.y + area.h - delta);
	delta -= d2;
	return Vec2(target.x, area.y + area.h - delta);
}

//TODO: 最初からRectで受け取る
WholeView::WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font)
: _area(pos, size)
, _textArea(_area.pos.asPoint() + font->getFontSize() * Point(1, 1), _area.size.asPoint() - 2 * font->getFontSize() * Point(1, 1))
, _lineInterval((int)(font->getFontSize()*1.25))
, _font(font)
, _ga(_font, _lineInterval, _textArea.h)
, _ju()
, _floatingStep(FloatingStep::Inactive)
, _floatingProgress()
, _scrollDelta(_lineInterval) {
}

void WholeView::setText(const String &text) {
	_ga.insertText(*_ga.cursor(), text);
}

void WholeView::draw() {
	String addend, editing;
	TextInput::UpdateText(addend);
	editing = TextInput::GetEditingText();
	_ju.update(editing.length());

	if ((_floatingStep == FloatingStep::Stable || _floatingStep == FloatingStep::AnimatingIn) && (KeyDown.down() || KeyUp.down())) {
		// Floating終了を開始
		_floatingStep = FloatingStep::AnimatingOut;
		_floatingProgress.start(1);
	}
	

	//if (KeyDown.down()) _textWindow.cursorNext();
	//if (KeyUp.down()) _textWindow.cursorPrev();

	/*if (!_textWindow.isEditing() && (addend.size() > 0 || editing.size() > 0)) {
		_floatingArrangement = _textWindow.startEditing();
		_floatingStep = FloatingStep::AnimatingIn;
		_floatingProgress.start(1.5);
	}*/

	/*auto drawCursor = [&](const GlyphArrangement::Iterator& itr) {
		if (itr.first != _textWindow.cursor().first) return;
		Vec2 c = *(itr.second);
		int cnt = _textWindow.text()->idx(itr.first);
		Vec2 a(_lineInterval / 2, 0);
		double t = Scene::Time() * 2 * Math::Pi / 2.5;
		Line(c - a, c + a).draw(1, Color(Palette::Black, (Sin(t) + 1) / 2 * 255));
	};*/

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

	/*_floatingProgress.update();
	if (_floatingProgress.getStep() == AnimationProgress::Step::Stable) {
		if (_floatingStep == FloatingStep::AnimatingIn)
			_floatingStep = FloatingStep::Stable;
		else if (_floatingStep == FloatingStep::AnimatingOut) {
			_textWindow.stopEditing();
			_floatingStep = FloatingStep::Inactive;
			_floatingArrangement = nullptr;
		}
	}
	bool isFloating = _floatingStep != FloatingStep::Inactive;*/


	/*if (_floatingStep == FloatingStep::AnimatingIn || _floatingStep == FloatingStep::Stable)
		_textWindow.inputText(addend, editing);*/
	if (_scrollDelta.step() != ScrollDelta::Step::NotScrolling) {
		auto [delta, offset] = _scrollDelta.useDelta();
		_ga.scroll(-delta);
	}

	_area.draw(Palette::White);
	{
		using GA = UnnamedEditor::WholeView::GlyphArrangement2;
		auto litr = _ga.origin();
		Point lineOrigin = _textArea.tr() - Point(_lineInterval / 2, 0) + _ga.originPos();
		while (lineOrigin.x > -_lineInterval && litr != _ga.endLine()) {
			for (auto c : litr->cd) {
				c.glyph->draw(lineOrigin + c.pos);
			}
			lineOrigin.x -= litr->advance;
			litr = _ga.tryNext(litr);
		}
	}
	/*auto itr = _textWindow.calcDrawBegin();
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
	}*/
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
	*lhead.second = Vec2(0, _area.y);
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

void TextWindow::setCursorUnsafe(Iterator cursor) {
	_cursor = cursor;
}

GlyphArrangement2::LineIterator GlyphArrangement2::initLine(LineIterator litr) {
	if (litr->cd.empty()) {
		return _data.erase(litr);
	}
	Point pen(0, 0);
	decltype(litr->cd)::iterator itr = litr->cd.begin();
	while (itr != litr->cd.end()) {
		itr->pos = pen;
		itr->glyph = _font->renderChar(itr->code);
		//TODO: Glyphをちゃんと整数座標に対応
		pen += itr->glyph->getAdvance().asPoint();
		if (pen.y > _maxLineLnegth - _font->getFontSize()) {
			pen = Point(pen.x - _lineInterval, 0);
		}
		for (auto i : itr->itrs) {
			*i = CharIterator(litr, itr);
		}
		itr = std::next(itr);
	}
	litr->advance = -pen.x + _lineInterval;
	return litr;
}

void GlyphArrangement2::initBucket(LineIterator fisrt, LineIterator last) {
	//TODO:
}

GlyphArrangement2::LineIterator GlyphArrangement2::tryNext(LineIterator litr, int cnt) {
	for (int i = 0; i < cnt && litr != _data.end(); i++) ++litr;
	return litr;
}

GlyphArrangement2::LineIterator GlyphArrangement2::tryPrev(LineIterator litr, int cnt) {
	for (int i = 0; i < cnt && litr != _data.begin(); i++) --litr;
	return litr;
}

void GlyphArrangement2::registerItr(SP<CharIterator> itr) {
	itr->second->itrs.push_back(itr);
}

GlyphArrangement2::GlyphArrangement2(SP<Font::FixedFont> font, int lineInterval, int maxLineLength)
: _font(font)
, _data()
, _lineInterval(lineInterval)
, _maxLineLnegth(maxLineLength)
, _origin()
, _originPos(0, 0)
, _cursor(new CharIterator()) {
	_data.push_back(LineData());
	CharData cd;
	cd.code = Text::Text::NEWLINE;
	_data.back().cd.push_back(cd);
	_cursor->first = _origin = _data.begin();
	_cursor->second = _data.begin()->cd.begin();
	registerItr(_cursor);
}

void GlyphArrangement2::insertText(CharIterator citr, const String& s) {
	if (s.empty()) return;
	LineIterator litr = citr.first;
	std::vector<CharData> tail(citr.second, litr->cd.end());
	litr->cd.erase(citr.second, litr->cd.end());
	for (auto c : s) {
		CharData cd;
		cd.code = c;
		litr->cd.push_back(cd);
		if (c == Text::Text::NEWLINE) {
			initLine(litr);
			litr = std::next(litr);
			LineData ld;
			litr = _data.insert(litr, ld);
		}
	}
	litr->cd.insert(litr->cd.end(), tail.begin(), tail.end());
	initLine(litr);
	initBucket(tryPrev(citr.first), tryNext(litr, 2));
}

void GlyphArrangement2::eraseText(CharIterator first, CharIterator last) {
	if (first.first == last.first) {
		first.first->cd.erase(first.second, last.second);
		auto litr = initLine(first.first);
		initBucket(tryPrev(litr), tryNext(litr, 2));
		return;
	}
	LineIterator litr = std::next(first.first);
	while (litr != last.first) {
		litr = _data.erase(litr);
	}
	first.first->cd.erase(first.second, first.first->cd.end());
	auto lfirst = initLine(first.first);
	last.first->cd.erase(last.first->cd.begin(), last.second);
	auto llast = initLine(last.first);
	initBucket(tryPrev(lfirst), tryNext(llast, 2));
}

void GlyphArrangement2::scroll(int delta) {
	_originPos.x -= delta;
	if (delta < 0) {
		while (_origin != _data.end() && _lineInterval < _originPos.x - _origin->advance) {
			_originPos.x -= _origin->advance;
			_origin = std::next(_origin);
		}
	}
	else {
		while (_origin != _data.begin() && _lineInterval > _originPos.x) {
			_origin = std::prev(_origin);
			_originPos.x += _origin->advance;
		}
	}
}

GlyphArrangement2::CharIterator GlyphArrangement2::next(CharIterator citr) const {
	CharIterator ret(citr.first, std::next(citr.second));
	if (ret.second == citr.first->cd.end()) {
		ret.first = std::next(ret.first);
		ret.second = ret.first->cd.begin();
	}
	return ret;
}

GlyphArrangement2::CharIterator GlyphArrangement2::prev(CharIterator citr) const {
	if (citr.second == citr.first->cd.begin()) {
		auto litr = std::prev(citr.first);
		return { litr, std::prev(litr->cd.end()) };
	}
	return { citr.first, std::prev(citr.second) };
}

void GlyphArrangement2::next(SP<CharIterator> citr) {
	auto& itrs = citr->second->itrs;
	for (auto i = itrs.begin(); i != itrs.end();) {
		if (*i == citr) i = itrs.erase(i);
		else ++i;
	}
	*citr = next(*citr);
	itrs.push_back(citr);
}

void GlyphArrangement2::prev(SP<CharIterator> citr) {
	auto& itrs = citr->second->itrs;
	for (auto i = itrs.begin(); i != itrs.end();) {
		if (*i == citr) i = itrs.erase(i);
		else ++i;
	}
	*citr = prev(*citr);
	itrs.push_back(citr);
}

GlyphArrangement2::LineIterator GlyphArrangement2::origin() const {
	return _origin;
}

Point GlyphArrangement2::originPos() const {
	return _originPos;
}

SP<GlyphArrangement2::CharIterator> GlyphArrangement2::cursor() const {
	return _cursor;
}

GlyphArrangement2::LineIterator GlyphArrangement2::endLine() {
	return _data.end();
}

}
}