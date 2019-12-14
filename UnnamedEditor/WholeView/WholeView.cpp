#include "stdafx.h"
#include "WholeView.h"
#include <iterator>
#include <optional>


namespace UnnamedEditor {
namespace WholeView {


WholeView::WholeView(const Rect area, SP<Font::FixedFont> font)
: _area(area)
, _textArea(_area.pos.asPoint() + font->getFontSize() * Point(1, 1), _area.size.asPoint() - 2 * font->getFontSize() * Point(1, 1))
, _lineInterval((int)(font->getFontSize()*1.25))
, _font(font)
, _ga(new GlyphArrangement2(_font, _lineInterval, _textArea.h))
, _ju()
, _scrollDelta(_lineInterval)
, _floating()
, _inputManager(_lineInterval, _textArea.h)
, _masker(_textArea.size)
, _maskee(_textArea.size)
, _foreground(area.size)
, _maskPS(U"example/shader/2d/multi_texture_mask" SIV3D_SELECT_SHADER(U".hlsl", U".frag"), { { U"PSConstants2D", 0 } }) {
	_floating = _inputManager.floatingAnimation();
}

void WholeView::setText(const String &text) {
	_ga->insertText(*_ga->cursor(), U"ぽ");
	auto itr = *_ga->cursor();
	_ga->prev(_ga->cursor());
	_ga->insertText(itr, text);
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

	if (!_inputManager.isInputing() && (addend.size() > 0 || editing.size() > 0 || KeyBackspace.down())) {
		_inputManager.startInputing(_ga);
	}
	auto cccursor = _inputManager.cleanCopyCursor();

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
	if (_inputManager.isInputing()) {
		bool onEnd = *_ga->cursor() == cccursor->drawingPos().first && cccursor->isStable();
		if (!cccursor->isStable()) addend = U"";
		if (KeyEnter.down() && !onEnd && _ju.isSettled()) cccursor->startAdvancing();
		else if (KeyEnter.up()) cccursor->stop();
		else if (KeyBackspace.down()) {
			if (cccursor->pos() == *_ga->cursor())
				cccursor->startRetreating();
			else
				_inputManager.deleteLightChar(_ga);
		}
		else if (KeyBackspace.up()) cccursor->stop();
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
		_ga->scroll(-delta);
	}

	_inputManager.inputText(_ga, addend, editing);
	if (_inputManager.isInputing()) cccursor->update(_tmpData);

	_tmpData.update();

	Vec2 maskStart, maskEnd;
	_masker.clear(Palette::White);
	_maskee.clear(ColorF(0, 0, 0, 0));
	_area.draw(Palette::White);
	_foreground.clear(ColorF(0, 0, 0, 0));
	{
		BlendState bs = BlendState::Default;
		bs.srcAlpha = Blend::One;
		bs.dstAlpha = Blend::InvSrcAlpha;
		ScopedRenderStates2D renderState(bs);
		ScopedRenderTarget2D target(_maskee);

		auto drawCursor = [&](Vec2 pos) {
			ScopedRenderTarget2D tmp(_foreground);
			double t = Scene::Time() * 2 * Math::Pi / 2.5;
			_font->getCursor(pos + _textArea.pos).draw(1, Color(Palette::Black, (Sin(t) + 1) / 2 * 255));
		};

		using GA = GlyphArrangement2;
		auto litr = _ga->origin();
		bool flt = false;
		Point lineOrigin = Point(_textArea.w - _lineInterval / 2, 0) + _ga->originPos();
		while (lineOrigin.x > -_lineInterval && litr != _ga->end()) {
			auto citr = _ga->lineBegin(litr);
			while (citr != _ga->lineEnd(litr)) {
				if (!_floating->isInactive() && _floating->floatingBegin() == citr) flt = true;

				Vec2 p = lineOrigin + citr.second->pos;
				if (flt) p = lineOrigin + _floating->getPos(citr);
				if (_maskee.region().stretched(_lineInterval, 0).contains(p)) {
					if (!CharAnimation::IsEmpty(citr.second->animation)) {
						citr.second->animation->draw(p, citr.second->glyph);
					}
					else citr.second->glyph->draw(p);
				}
				if (citr == *_ga->cursor()) {
					drawCursor(p);
					maskEnd = p;
				}
				if (!_floating->isInactive() && citr == cccursor->drawingPos().first) {
					Vec2 cp = p + Vec2(0, cccursor->drawingPos().second);
					drawCursor(cp);
					maskStart = cp;
				}
				citr = _ga->next(citr);
			}
			lineOrigin.x -= litr->wrapCount * _lineInterval;
			litr = _ga->tryNext(litr);
		}
	}
	if (!_floating->isInactive()) {
		ScopedRenderTarget2D target(_masker);
		int lines = (int)(-(maskEnd.x - maskStart.x) / _lineInterval + 0.5);
		Vec2 st = maskStart;
		ColorF color(Palette::Black, 0.5);
		double hfs = _font->getFontSize() / 2.0;
		for (int i = 0; i < lines; i++) {
			RectF r(st.x - hfs, st.y, 2*hfs, _textArea.h - st.y);
			r.draw(color);
			st = Vec2(st.x - _lineInterval, 0);
		}
		RectF r(st.x - hfs, st.y, 2*hfs, maskEnd.y - st.y);
		r.draw(color);
	}
	{
		Graphics2D::SetTexture(1, _masker);
		ScopedCustomShader2D shader(_maskPS); //TODO: _maskPSのプリコンパイル
		_maskee.draw(_textArea.pos);
		Graphics2D::SetTexture(1, none);
	}
	Graphics2D::Flush();
	_foreground.resolve();
	_foreground.draw(_area.pos);
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

void WholeView::minimapTest() {
	_ga->begin()->bucketHeader->minimap.draw();
	_ga->begin()->bucketHeader->minimap.draw(Vec2(0.5, 300));
}

SP<GlyphArrangement2> WholeView::GlyphArrangement() const {
	return _ga;
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
	int wrapCount = 0;
	while (itr != litr->cd.end()) {
		if (pen.y > _maxLineLnegth - _font->getFontSize()) {
			pen = Point(pen.x - _lineInterval, 0);
			wrapCount++;
		}
		itr->pos = pen;
		if (!itr->glyph) {
			if (itr->code == Text::Text::NEWLINE || itr->code == Text::Text::NULL_CHAR) {
				itr->glyph = itr->blurred = Font::Glyph::EmptyGlyph();
			}
			else {
				itr->glyph = _font->renderChar(itr->code);
				itr->blurred = _blurredFont->renderChar(itr->code);
			}
		}
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
	litr->wrapCount = wrapCount + 1;
	return litr;
}

void GlyphArrangement2::initBucket(LineIterator first, LineIterator last) {
	int fs = _font->getFontSize();
	double fs_ = _minimapFontSize;
	double sr = ((double)fs_) / fs;
	int normalSize = 1000 * fs / _maxLineLnegth;

	if (first == last) return;
	first = bucket(first);
	last = nextBucket(last);
	LineIterator prevBucket = bucket(tryPrev(first));
	if (first->bucketHeader && prevBucket->bucketHeader &&
		first->bucketHeader->wrapCount + prevBucket->bucketHeader->wrapCount < normalSize) {
		first = prevBucket;
	}

	LineIterator head = first;
	bool checkedLast = false;
	while (head != end() && !checkedLast) {
		head->bucketHeader.reset(new BucketHeader());
		LineIterator bl = head;
		int size = head->wrapCount;
		bl = tryNext(bl);
		while (bl != end()) {
			if (bl == last) checkedLast = true;
			int add = bl->wrapCount;
			LineIterator next = tryNext(bl);
			if (bl->bucketHeader) {
				add = bl->bucketHeader->wrapCount;
				next = nextBucket(next);
			}
			if (normalSize < size + add) break;
			size += add;
			bl = next;
		}
		int textureWidth = (int)(sr * (size + 1) * _lineInterval + 1);
		double ascender = fs_ / 2.0;
		MSRenderTexture msrt(Size(textureWidth, (int)(sr * _maxLineLnegth + 1)));
		msrt.clear(ColorF(Palette::White, 0));
		{
			double a = 3;
			ScopedColorMul2D mul(1, a);
			ScopedRenderTarget2D target(msrt);
			BlendState bs = BlendState::Default;
			bs.srcAlpha = Blend::One;
			bs.dstAlpha = Blend::InvSrcAlpha;
			ScopedRenderStates2D state(bs);
			LineIterator b = head;
			Vec2 lineOrigin(textureWidth - ascender, 0);
			while (b != bl) {
				for (auto c : b->cd) {
					c.blurred->draw(lineOrigin + sr * c.pos, Palette::White, 0, sr);
				}
				lineOrigin.x -= sr * b->wrapCount * _lineInterval;
				b = tryNext(b);
			}
		}
		Graphics2D::Flush();
		msrt.resolve();

		head->bucketHeader->minimap = msrt;
		head->bucketHeader->wrapCount = size;
		head->bucketHeader->advance = (int)(sr * size * _lineInterval + 1);
		head->bucketHeader->origin = Vec2(textureWidth - ascender, 0);

		head = bl;
	}
}

GlyphArrangement2::LineIterator GlyphArrangement2::tryNext(LineIterator litr, int cnt) const {
	for (int i = 0; i < cnt && litr != _data.end(); i++) ++litr;
	return litr;
}

GlyphArrangement2::LineIterator GlyphArrangement2::tryPrev(LineIterator litr, int cnt) const {
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
, _blurredFont(new Font::FixedFont(font->ftLibrary(), font->ftFace(), font->getFontSize(), font->isVertical()))
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

	auto blur = [&](Image& img) {
		img.gaussianBlur(2, 2);
	};
	_blurredFont->setRetouchImage(blur);
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
		initBucket(litr, tryNext(litr));
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
	initBucket(citr.first, tryNext(litr));
	return { ret0, ret0->cd.begin() + idx };
}

GlyphArrangement2::CharIterator GlyphArrangement2::eraseText(CharIterator first, CharIterator last) {
	if (first == last) return first;
	
	//管理されたイテレータの退避
	for (auto itr = first; itr != last; itr = next(itr, true)) {
		for (auto managedItr : itr.second->itrs) {
			*managedItr = last;
			last.second->itrs.push_back(managedItr);
		}
	}
	if (first.first == last.first) {
		auto ret1 = first.first->cd.erase(first.second, last.second);
		auto litr = initLine(first.first);
		initBucket(litr, tryNext(litr));
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
	initBucket(lfirst, tryNext(llast));
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
		while (_origin != _data.end() && _lineInterval < _originPos.x - _origin->wrapCount * _lineInterval) {
			_originPos.x -= _origin->wrapCount * _lineInterval;
			_origin = std::next(_origin);
		}
	}
	else {
		while (_origin != _data.begin() && _lineInterval > _originPos.x) {
			_origin = std::prev(_origin);
			_originPos.x += _origin->wrapCount * _lineInterval;
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

GlyphArrangement2::LineIterator GlyphArrangement2::bucket(LineIterator i) const {
	while (i != _data.begin() && !i->bucketHeader) {
		i = tryPrev(i);
	}
	return i;
}

GlyphArrangement2::LineIterator GlyphArrangement2::nextBucket(LineIterator i) const {
	while (i != _data.end() && !i->bucketHeader) {
		i = tryNext(i);
	}
	return i;
}

double GlyphArrangement2::minimapLineInterval() const {
	return _minimapFontSize / (double)_font->getFontSize() * _lineInterval;
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
	_oldAdvance = _newAdvance = head->wrapCount * _lineInterval;
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
	_newAdvance = head->wrapCount * _lineInterval;
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

InputManager::InputManager(int lineInterval, int maxLineLength)
: _fa(new FloatingAnimation(lineInterval, maxLineLength)) {

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

void InputManager::inputText(SP<GA> ga, const String& addend, const String& editing) {
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
	auto cursor = ga->cursor();
	int eraseSize = _editing.size() - prefixLength;
	if (eraseSize > 0 || !replaced.empty()) {
		auto first = ga->prev(*cursor, true, eraseSize);
		bool flg = first == _cccursor->pos();
		auto newItr = ga->replaceText(first, *cursor, replaced);
		if (flg) {
			_cccursor->changeItr(newItr);
		}
		_fa->updatePos(*ga);
	}
	_editing = editing;
}

void InputManager::stopInputing() {
	_isInputing = false;
	_editing = U"";
	_fa->stopFloating();
}

void InputManager::startInputing(SP<GA> ga) {
	_isInputing = true;
	_cursor = ga->makeNull(*ga->cursor());
	auto floatingBegin = *ga->cursor();
	ga->prev(ga->cursor());
	assert(*_cursor == *ga->cursor());
	_cccursor.reset(new CleanCopyCursor(ga, *_cursor));
	_fa->startFloating(*ga, floatingBegin);
}

void InputManager::deleteLightChar(SP<GA> ga) {
	_cccursor->changeItr(ga->eraseText(_cccursor->pos(), *ga->cursor()));
}

//void InputManager::stopInputing() {
//	_isInputing = false;
//}
//
//void InputManager::startInputing(const GlyphArrangement2& ga) {
//	_isInputing = true;
//	_fa->floatingStart(ga);
//}

CleanCopyCursor::CleanCopyCursor(SP<GA> ga, GA::CharIterator citr)
: _step(Step::Stable)
, _ga(ga) {
	_drawingPos.first.reset(new GA::CharIterator(citr));
	ga->registerItr(_drawingPos.first);
	_drawingPos.second = 0;
}

GlyphArrangement2::CharIterator CleanCopyCursor::pos() const {
	if (_step != Step::Retreating) return *_drawingPos.first;
	return _ga->next(*_drawingPos.first);
}

std::pair<GlyphArrangement2::CharIterator, double> CleanCopyCursor::drawingPos() const {
	return { *_drawingPos.first, _drawingPos.second };
}

void CleanCopyCursor::changeItr(GA::CharIterator newItr) {
	_ga->removeItr(_drawingPos.first);
	_drawingPos.first.reset(new GA::CharIterator(newItr));
	_ga->registerItr(_drawingPos.first);
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
	if (*_drawingPos.first == _ga->lineBegin(_ga->begin())) return;
	if (_step != Step::Retreating)
		_ga->prev(_drawingPos.first);
	_step = Step::Retreating;
	_drawingPos.second = 0;
	_sw.restart();
}

void CleanCopyCursor::stop() {
	if (_step == Step::Retreating && *_drawingPos.first != *_ga->cursor() && _sw.isRunning())
		_ga->next(_drawingPos.first);
	_step = Step::Stable;
	_drawingPos.second = 0;
	_sw.reset();
}

void CleanCopyCursor::update(TemporaryData::Manager& tmpData) {
	if (isStable()) return;
	double velocity = 30; //TODO: フォントサイズに比例
	if (_step == Step::Retreating) velocity *= 4;
	if (_step == Step::Advancing) {
		while (true) {
			_drawingPos.second = velocity * _sw.sF();
			double advance = _drawingPos.first->second->glyph->getAdvance().y;
			if (*_drawingPos.first == *_ga->cursor()) {
				_drawingPos.second = 0;
				_sw.pause();
				break; //_stepはStableにしない
			}
			if (_drawingPos.second < advance) break;
			registerPaint(tmpData, *_drawingPos.first);
			_ga->next(_drawingPos.first);
			_drawingPos.second -= advance;
			_sw.set(_sw.elapsed() - SecondsF(advance / velocity));
		}
	}
	else if (_step == Step::Retreating) {
		while (true) {
			double advance = _drawingPos.first->second->glyph->getAdvance().y;
			_drawingPos.second = advance - velocity * _sw.sF();
			if (_drawingPos.second > 0) break;
			registerUnpaint(tmpData, *_drawingPos.first);
			if (*_drawingPos.first == _ga->lineBegin(_ga->begin())) {
				_drawingPos.second = 0;
				_sw.pause();
				return; //_stepはStableにしない
			}
			_ga->prev(_drawingPos.first);
			double newAdvance = _drawingPos.first->second->glyph->getAdvance().y;
			_drawingPos.second += newAdvance;
			_sw.set(_sw.elapsed() - SecondsF(advance / velocity));
		}
	}
}

void CleanCopyCursor::registerPaint(TemporaryData::Manager& tmpData, GA::CharIterator citr) {
	auto paint = [&](Vec2 pos, SP<const Font::Glyph> glyph, double t) {
		glyph->draw(pos, Palette::Black, 0, 1 + 0.4*Sin(t*Math::Pi));
	};
	citr.second->animation.reset(new CharAnimation(paint, 1));
	tmpData.registerPointer(citr.second->animation);
}

void CleanCopyCursor::registerUnpaint(TemporaryData::Manager& tmpData, GA::CharIterator citr) {
	auto unpaint = [&](Vec2 pos, SP<const Font::Glyph> glyph, double t) {
		glyph->draw(pos, Palette::Black, 0, 1 - 0.4*Sin(t*Math::Pi));
	};
	citr.second->animation.reset(new CharAnimation(unpaint, 0.5));
	tmpData.registerPointer(citr.second->animation);
}

MinimapView::MinimapView(RectF area, SP<GA> ga)
: _area(area)
, _body(area)
, _ga(ga) {
}

void MinimapView::draw() {
	BlendState bs = BlendState::Default;
	bs.srcAlpha = Blend::One;
	bs.dstAlpha = Blend::InvSrcAlpha;
	ScopedRenderStates2D state(bs);
	_area.draw(Palette::White);
	{
		ScopedViewport2D viewport(_body);
		Transformer2D transform(Mat3x2::Identity(), Mat3x2::Translate(_body.pos));
		ColorF c = Palette::Blueviolet;
		//ScopedColorAdd2D colorAdd(c);
		//ScopedColorMul2D colorMul(ColorF(1, 1, 1) - c);
		double li = _ga->minimapLineInterval();
		GA::LineIterator bucket = _ga->begin();
		Point pen(_body.w, 0);
		while (bucket != _ga->end()) {
			if (_body.h < pen.y) break;
			GA::LineIterator nextBucket = _ga->nextBucket(_ga->tryNext(bucket));
			SP<GA::BucketHeader> header = bucket->bucketHeader;
			const auto& map = header->minimap;
			
			while (true) {
				header->minimap.draw(Arg::topRight(pen), Palette::Black);
				int xEnd = pen.x, xBegin = std::max(0, pen.x - header->advance);
				if (Rect(xBegin, pen.y, xEnd - xBegin, map.height()).contains(Cursor::Pos())) {
					//header->minimap.region(pen + Point(delta - header->minimap.width(), 0)).draw(Palette::Orange);
					//RectF(xBegin, pen.y, xEnd - xBegin, map.height()).draw(Palette::Red);
					GA::LineIterator litr = bucket;
					double x = pen.x;
					double nx = x;
					while (litr != nextBucket) {
						nx = x - litr->wrapCount * li;
						if (nx < Cursor::PosF().x) break;
						x = nx;
						litr = _ga->tryNext(litr);
					}

					if (litr != nextBucket) {
						if (MinimapHighlight::IsEmpty(litr->highlight)) {
							RectF r(nx - (pen.x - map.width()), 0, x - nx, map.height());
							litr->highlight.reset(new MinimapHighlight(map, r, Vec2(x, pen.y)));
							_tmpManager.registerPointer(litr->highlight);
						}
						litr->highlight->keep();
					}
				}
				if (pen.x - header->minimap.width() >= 0) break;
				pen = Point(_body.w + pen.x, pen.y + header->minimap.height() + (int)(li * 2));
			}

			pen.x -= header->advance;
			bucket = nextBucket;
		}
	}
	_tmpManager.update();
}

//void MinimapHighlight::draw() {
//	for (auto c : _litr->cd) {
//		Vec2 p = _origin + _minimapScale * c.pos;
//		p = _ap.getProgress()
//		c.glyph->draw(p, Palette::Black, 0, _minimapScale);
//	}
//}

}
}