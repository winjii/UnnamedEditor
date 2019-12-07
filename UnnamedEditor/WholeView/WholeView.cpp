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
//Vec2 WholeView::floatingTextOut(Vec2 source, Vec2 target, double t, int i)
//{
//	RectF area = _textArea;
//
//	double rate = EaseOut(Easing::Expo, 3.0, 1.0, std::min(1.0, i / 200.0));
//	t = std::min(1.0, t*rate);
//
//	double d1 = source.y - area.y;
//	double d2 = area.h;
//	double d3 = area.y + area.h - target.y;
//	double delta = EaseOut(Easing::Expo, t) * (d1 + d2 + d3);
//	if (delta <= d1) return Vec2(source.x, source.y - delta);
//	delta -= d1;
//	if (delta <= d2) return Vec2((source.x + target.x) / 2.0, area.y + area.h - delta);
//	delta -= d2;
//	return Vec2(target.x, area.y + area.h - delta);
//}

//TODO: 最初からRectで受け取る
WholeView::WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font)
: _area(pos, size)
, _textArea(_area.pos.asPoint() + font->getFontSize() * Point(1, 1), _area.size.asPoint() - 2 * font->getFontSize() * Point(1, 1))
, _lineInterval((int)(font->getFontSize()*1.25))
, _font(font)
, _ga(_font, _lineInterval, _textArea.h)
, _ju()
, _scrollDelta(_lineInterval)
, _floating(new FloatingAnimation(_lineInterval, _textArea.h))
, _inputManager(_floating) {
}

void WholeView::setText(const String &text) {
	_ga.insertText(*_ga.cursor(), U"ぽ");
	auto itr = *_ga.cursor();
	_ga.prev(_ga.cursor());
	_ga.insertText(itr, text);
}

void WholeView::draw() {
	String addend, editing;
	TextInput::UpdateText(addend);
	editing = TextInput::GetEditingText();
	_ju.update(editing.length());

	_floating->updateTime();

	//if ((_floatingStep == FloatingStep::Stable || _floatingStep == FloatingStep::AnimatingIn) && (KeyDown.down() || KeyUp.down())) {
	//	// Floating終了を開始
	//	_floatingStep = FloatingStep::AnimatingOut;
	//	_floatingProgress.start(1);
	//}
	

	//if (KeyDown.down()) _textWindow.cursorNext();
	//if (KeyUp.down()) _textWindow.cursorPrev();

	if (!_inputManager.isInputing() && (addend.size() > 0 || editing.size() > 0)) {
		_inputManager.startInputing(_ga);
	}
	auto cccursor = _inputManager.cleanCopyCursor();

	auto drawCursor = [&](Vec2 pos) {
		double t = Scene::Time() * 2 * Math::Pi / 2.5;
		_font->getCursor(pos).draw(1, Color(Palette::Black, (Sin(t) + 1) / 2 * 255));
	};

	if (_floating->step() != FloatingAnimation::Step::Floating) {
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
	if (!_floating->isInactive()) {
		if (!cccursor->isStable()) addend = U"";
		if (KeyEnter.down()) cccursor->startAdvancing();
		else if (KeyEnter.up()) cccursor->stop(_ga);
		else if (KeyBackspace.down()) cccursor->startRetreating();
		else if (KeyBackspace.up()) cccursor->stop(_ga);
		if (!cccursor->isStable()) addend = U"";
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
	
	_inputManager.inputText(_ga, addend, editing);
	if (_inputManager.isInputing()) cccursor->update(_ga);

	_area.draw(Palette::White);
	{
		using GA = GlyphArrangement2;
		auto litr = _ga.origin();
		bool flt = false;
		Point lineOrigin = _textArea.tr() - Point(_lineInterval / 2, 0) + _ga.originPos();
		while (lineOrigin.x > -_lineInterval && litr != _ga.end()) {
			auto citr = _ga.lineBegin(litr);
			while (citr != _ga.lineEnd(litr)) {
				if (!_floating->isInactive() && _floating->floatingBegin() == citr) flt = true;

				Vec2 p = lineOrigin + citr.second->pos;
				if (flt) p = lineOrigin + _floating->getPos(citr);
				if (_textArea.stretched(_lineInterval, 0).contains(p))
					citr.second->glyph->draw(p);
				if (citr == *_ga.cursor()) drawCursor(p.asPoint());
				if (!_floating->isInactive() && citr == cccursor->drawingPos().first) {
					drawCursor(p + Vec2(0, cccursor->drawingPos().second));
				}
				citr = _ga.next(citr);
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
		if (pen.y > _maxLineLnegth - _font->getFontSize()) {
			pen = Point(pen.x - _lineInterval, 0);
		}
		itr->pos = pen;
		if (!itr->glyph) itr->glyph = _font->renderChar(itr->code);
		//TODO: Glyphをちゃんと整数座標に対応
		pen += itr->glyph->getAdvance().asPoint();
		for (auto itrtrtritrtr = itr->itrs.begin(); itrtrtritrtr != itr->itrs.end();) {
			if (itrtrtritrtr->use_count() <= 1) itrtrtritrtr = itr->itrs.erase(itrtrtritrtr);
			else {
				**itrtrtritrtr = CharIterator(litr, itr);
				++itrtrtritrtr;
			}
		}
		if (itr->code == Text::Text::NULL_CHAR && itr->itrs.empty()) //要らなくなったNULL文字の削除
			itr = litr->cd.erase(itr);
		else itr = std::next(itr);
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

void GlyphArrangement2::removeItr(SP<CharIterator> itr) {
	auto& itrs = itr->second->itrs;
	for (auto i = itrs.begin(); i != itrs.end();) {
		if (itr == *i) i = itrs.erase(i);
		else ++i;
	}
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

GlyphArrangement2::CharIterator GlyphArrangement2::insertText(CharIterator citr, const String& s) {
	if (s.empty()) return citr;
	LineIterator litr = citr.first;
	if (!s.includes(Text::Text::NEWLINE)) {
		std::vector<CharData> cds;
		for (auto c : s) {
			cds.push_back(CharData());
			cds.back().code = c;
		}
		citr.second = litr->cd.insert(citr.second, cds.begin(), cds.end());
		initLine(litr);
		initBucket(tryPrev(litr), tryNext(litr, 2));
		return citr;
	}
	std::vector<CharData> tail(citr.second, litr->cd.end());
	int idx = citr.second - litr->cd.begin();
	litr->cd.erase(citr.second, litr->cd.end());
	LineIterator ret0 = litr;
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
	return { ret0, ret0->cd.begin() + idx };
}

GlyphArrangement2::CharIterator GlyphArrangement2::eraseText(CharIterator first, CharIterator last) {
	if (first.first == last.first) {
		if (first.second == last.second) return first;
		auto ret1 = first.first->cd.erase(first.second, last.second);
		auto litr = initLine(first.first);
		initBucket(tryPrev(litr), tryNext(litr, 2));
		return { litr, ret1 };
	}
	LineIterator litr = std::next(first.first);
	while (litr != last.first) {
		litr = _data.erase(litr);
	}
	first.first->cd.erase(first.second, first.first->cd.end());
	auto lfirst = initLine(first.first);
	auto ret1 = last.first->cd.erase(last.first->cd.begin(), last.second);
	if (last.first->cd.empty()) ret1 = std::next(last.first->cd.begin());
	auto llast = initLine(last.first);
	initBucket(tryPrev(lfirst), tryNext(llast, 2));
	return { llast, ret1 };
}

//TODO: より効率的な実装をすべきかもしれない
GlyphArrangement2::CharIterator GlyphArrangement2::replaceText(CharIterator first, CharIterator last, const String& s) {
	auto itr = eraseText(first, last);
	return insertText(itr, s);
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

GlyphArrangement2::CharIterator GlyphArrangement2::next(CharIterator citr, bool overLine, int cnt) const {
	for (int i = 0; i < cnt; i++) {
		assert(citr.first != _data.end());
		citr.second = std::next(citr.second);
		if (overLine && citr.second == citr.first->cd.end()) {
			citr.first = std::next(citr.first);
			citr.second = citr.first->cd.begin();
		}
	}
	return citr;
}

GlyphArrangement2::CharIterator GlyphArrangement2::prev(CharIterator citr, bool overLine, int cnt) const {
	for (int i = 0; i < cnt; i++) {
		assert(!(!overLine && citr.second == citr.first->cd.begin()));
		if (citr.second == citr.first->cd.begin()) {
			citr.first = std::prev(citr.first);
			citr.second = std::prev(citr.first->cd.end());
		}
		else {
			citr.second = std::prev(citr.second);
		}
	}
	return citr;
}

void GlyphArrangement2::next(SP<CharIterator> citr) {
	removeItr(citr);
	*citr = next(*citr, true);
	citr->second->itrs.push_back(citr);
}

void GlyphArrangement2::prev(SP<CharIterator> citr) {
	removeItr(citr);
	*citr = prev(*citr, true);
	citr->second->itrs.push_back(citr);
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

GlyphArrangement2::LineIterator GlyphArrangement2::begin() {
	return _data.begin();
}

GlyphArrangement2::LineIterator GlyphArrangement2::end() {
	return _data.end();
}

GlyphArrangement2::CharIterator GlyphArrangement2::lineBegin(LineIterator litr) const {
	return { litr, litr->cd.begin() };
}

GlyphArrangement2::CharIterator GlyphArrangement2::lineEnd(LineIterator litr) const {
	return { litr, litr->cd.end() };
}

SP<GlyphArrangement2::CharIterator> GlyphArrangement2::makeNull(CharIterator citr) {
	CharData cd;
	cd.code = Text::Text::NULL_CHAR;
	citr.second = citr.first->cd.insert(citr.second, cd);
	SP<CharIterator> ret(new CharIterator(citr));
	registerItr(ret);
	initLine(citr.first);
	return ret;
}

void GlyphArrangement2::deleteNull(SP<CharIterator> nullItr) {
	nullItr->first->cd.erase(nullItr->second);
	initLine(nullItr->first);
}

Vec2 FloatingAnimation::easeOverLine(Vec2 source, Vec2 target, double t, int i) {
	double rate = EaseOut(Easing::Expo, 3.0, 1.0, std::min(1.0, i / 200.0));
	t = std::min(1.0, t * rate);

	int lineDiff = std::round(-(target.x - source.x) / _lineInterval);
	double sum = target.y + _maxLineLength * lineDiff - source.y;
	double delta = EaseInOut(Easing::Sine, t) * sum;
	double flr = std::floor((delta + source.y) / _maxLineLength);
	return Vec2(source.x - flr * _lineInterval, source.y + delta - flr * _maxLineLength);
}

FloatingAnimation::FloatingAnimation(int lineInterval, int maxLineLength)
: _lineInterval(lineInterval), _maxLineLength(maxLineLength) {
	
}

FloatingAnimation::Step FloatingAnimation::step() const {
	return _step;
}

bool FloatingAnimation::isInactive() const {
	return _step == Step::Inactive;
}

FloatingAnimation::GA::CharIterator FloatingAnimation::floatingBegin() const {
	return *_floatingBegin;
}

bool FloatingAnimation::isStable() const {
	bool b0 = _inOutAP.getStep() == AnimationProgress::Step::Stable;
	bool b1 = _updateAP.getStep() == AnimationProgress::Step::Stable;
	return b0 && b1;
}

void FloatingAnimation::startFloating(GA& ga, GA::CharIterator floatingBegin) {
	_floatingBegin.reset(new GA::CharIterator(floatingBegin));
	ga.registerItr(_floatingBegin);
	GA::LineIterator head = floatingBegin.first;
	_oldAdvance = _newAdvance = head->advance;
	int size = std::distance(floatingBegin.second, head->cd.end());
	_oldHeadPos.resize(size);
	_newHeadPos.resize(size);
	for (int i = 0; i < size; i++) {
		auto ritr = std::next(floatingBegin.second, i);
		_oldHeadPos[i] = _newHeadPos[i] = ritr->pos;
	}
	_step = Step::Floating;
	_inOutAP.start(1.5);
}

void FloatingAnimation::stopFloating() {
	_step = Step::Stopping;
	_inOutAP.start(1.5);
}

void FloatingAnimation::updateTime() {
	_inOutAP.update();
	_updateAP.update();
	if (_step == Step::Stopping && isStable()) {
		_step = Step::Inactive;
	}
}

void FloatingAnimation::updatePos(const GA& ga) {
	if (_step == Step::Inactive) return;
	GA::LineIterator head = _floatingBegin->first;
	_oldAdvance = _newAdvance;
	_newAdvance = head->advance;
	for (int i = 0; i < _oldHeadPos.size(); i++) {
		auto ritr = std::next(_floatingBegin->second, i);
		//TODO: アニメーション位置をoldにするとなめらかに遷移できる
		_oldHeadPos[i] = easeOverLine(_oldHeadPos[i], _newHeadPos[i], _updateAP.getProgress(), i);
		_newHeadPos[i] = ritr->pos;
	}
	_updateAP.start(2.0);
}

Vec2 FloatingAnimation::getPos(GA::CharIterator citr) {
	assert(_step != Step::Inactive);
	Vec2 d(-2 * _lineInterval, 0);
	if (_step == Step::Floating) {
		d *= EaseOut(Easing::Expo, _inOutAP.getProgress());
	}
	else if (_step == Step::Stopping) {
		d *= EaseIn(Easing::Expo, _inOutAP.getProgress());
	}

	if (citr.first != _floatingBegin->first) {
		double t = _updateAP.getProgress();
		Vec2 e = EaseOut(Easing::Expo, Vec2((-_oldAdvance) - (-_newAdvance), 0), Vec2(0, 0), t);
		return citr.second->pos + d + e;
	}
	int idx = citr.second - _floatingBegin->second;
	return easeOverLine(_oldHeadPos[idx], _newHeadPos[idx], _updateAP.getProgress(), idx) + d;
}

InputManager::InputManager(SP<FloatingAnimation> fa) : _fa(fa) {
}

bool InputManager::isInputing() const {
	return _isInputing;
}

SP<FloatingAnimation> InputManager::floatingAnimation() const {
	return _fa;
}

SP<CleanCopyCursor> InputManager::cleanCopyCursor() const {
	return _cccursor;
}

void InputManager::inputText(GA& ga, const String& addend, const String& editing) {
	if (!_isInputing) return;
	String all = addend + editing;
	int prefixLength = [&]() {
		int i = 0;
		for (; i < Min(_editing.size(), all.size()); i++) {
			if (_editing[i] != all[i]) break;
		}
		return i;
	}();
	String replaced = addend.dropped(Min((int)addend.size(), prefixLength));
	replaced += editing.dropped(Max(0, prefixLength - (int)addend.size()));
	auto cursor = ga.cursor();
	int eraseSize = _editing.size() - prefixLength;
	if (eraseSize > 0 || !replaced.empty()) {
		auto first = ga.prev(*cursor, true, eraseSize);
		bool flg = first == _cccursor->pos(ga);
		auto newItr = ga.replaceText(first, *cursor, replaced);
		if (flg) {
			_cccursor->changeItr(ga, newItr);
		}
		_fa->updatePos(ga);
	}
	_editing = editing;
}

void InputManager::stopInputing() {
	_isInputing = false;
	_editing = U"";
	_fa->stopFloating();
}

void InputManager::startInputing(GA& ga) {
	_isInputing = true;
	auto floatingBegin = *ga.cursor();
	_cursor = ga.makeNull(*ga.cursor());
	ga.prev(ga.cursor());
	assert(*_cursor == *ga.cursor());
	_cccursor.reset(new CleanCopyCursor(ga, *_cursor));
	_fa->startFloating(ga, floatingBegin);
}

//void InputManager::stopInputing() {
//	_isInputing = false;
//}
//
//void InputManager::startInputing(const GlyphArrangement2& ga) {
//	_isInputing = true;
//	_fa->floatingStart(ga);
//}

CleanCopyCursor::CleanCopyCursor(GA& ga, GA::CharIterator citr)
: _step(Step::Stable) {
	_drawingPos.first.reset(new GA::CharIterator(citr));
	ga.registerItr(_drawingPos.first);
	_drawingPos.second = 0;
}

GlyphArrangement2::CharIterator CleanCopyCursor::pos(const GA& ga) const {
	if (_step != Step::Retreating) return *_drawingPos.first;
	return ga.next(*_drawingPos.first);
}

std::pair<GlyphArrangement2::CharIterator, double> CleanCopyCursor::drawingPos() const {
	return { *_drawingPos.first, _drawingPos.second };
}

void CleanCopyCursor::changeItr(GA& ga, GA::CharIterator newItr) {
	ga.removeItr(_drawingPos.first);
	_drawingPos.first.reset(new GA::CharIterator(newItr));
	ga.registerItr(_drawingPos.first);
}

bool CleanCopyCursor::isStable() {
	return _step == Step::Stable;
}

void CleanCopyCursor::startAdvancing() {
	_step = Step::Advancing;
	_drawingPos.second = 0;
	_sw.restart();
}

void CleanCopyCursor::startRetreating() {
	_step = Step::Retreating;
	_drawingPos.second = 0;
	_sw.restart();
}

void CleanCopyCursor::stop(GA& ga) {
	if (_step == Step::Retreating && *_drawingPos.first != *ga.cursor())
		ga.next(_drawingPos.first);
	_step = Step::Stable;
	_drawingPos.second = 0;
	_sw.reset();
}

void CleanCopyCursor::update(GA& ga) {
	if (isStable()) return;
	double velocity = 15; //TODO: フォントサイズに比例
	if (_step == Step::Advancing) {
		while (true) {
			_drawingPos.second = velocity * _sw.sF();
			double advance = _drawingPos.first->second->glyph->getAdvance().y;
			if (*_drawingPos.first == *ga.cursor()) {
				stop(ga);
				break;
			}
			if (_drawingPos.second < advance) break;
			ga.next(_drawingPos.first);
			_drawingPos.second -= advance;
			_sw.set(_sw.elapsed() - SecondsF(advance / velocity));
		}
	}
	else if (_step == Step::Retreating) {
		while (true) {
			double advance = _drawingPos.first->second->glyph->getAdvance().y;
			_drawingPos.second = advance - velocity * _sw.sF();
			if (_drawingPos.second > 0) break;
			if (*_drawingPos.first == ga.lineBegin(ga.begin())) {
				stop(ga);
				return;
			}
			ga.prev(_drawingPos.first);
			double newAdvance = _drawingPos.first->second->glyph->getAdvance().y;
			_drawingPos.second += newAdvance;
			_sw.set(_sw.elapsed() - SecondsF(advance / velocity));
		}
	}
}

}
}