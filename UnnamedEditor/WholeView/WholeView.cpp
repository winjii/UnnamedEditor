#include "stdafx.h"
#include "WholeView.h"
#include <iterator>
#include <optional>


namespace UnnamedEditor {
namespace WholeView {


WholeView::WholeView(const Rect area, SP<Font::FixedFont> font, TD::Direction textDir)
: _area(area)
, _textArea(Rect(_area.pos.asPoint() + font->getFontSize() * Point(1, 1), _area.size.asPoint() - 2 * font->getFontSize() * Point(1, 1)), textDir)
, _lineInterval((int)(font->getFontSize()*1.25))
, _textDir(textDir)
, _font(font)
, _ga(new GlyphArrangement2(_font, _lineInterval, _textArea.size.prl, textDir))
, _ju()
, _scrollDelta(_lineInterval)
, _inputManager(_lineInterval, _textArea.size.prl)
, _masker(_textArea.realSize(textDir))
, _maskee(_textArea.realSize(textDir))
, _foreground(area.size)
, _maskPS(U"example/shader/2d/multi_texture_mask" SIV3D_SELECT_SHADER(U".hlsl", U".frag"), { { U"PSConstants2D", 0 } }) {
	jump(_ga->begin());
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

	SP<FloatingAnimation> floating = _inputManager.floatingAnimation();

	//if ((_floatingStep == FloatingStep::Stable || _floatingStep == FloatingStep::AnimatingIn) && (KeyDown.down() || KeyUp.down())) {
	//	// Floating終了を開始
	//	_floatingStep = FloatingStep::AnimatingOut;
	//	_floatingProgress.start(1);
	//}


	//if (KeyDown.down()) _textWindow.cursorNext();
	//if (KeyUp.down()) _textWindow.cursorPrev();

	if (!_inputManager.isInputing() && (addend.size() > 0 || editing.size() > 0 || KeyBackspace.down())) {
		_inputManager.startInputting(_ga);
	}
	auto cccursor = _inputManager.cleanCopyCursor();

	if (floating->step() != FloatingAnimation::Step::Floating) {
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
	if (_inputManager.isInputing() && editing.size() == 0) {
		bool onEnd = *_ga->cursor() == cccursor->drawingPos().first && cccursor->isStable();
		if (!cccursor->isStable()) addend = U"";
		if (KeyEnter.down() && !onEnd && _ju.isSettled()) cccursor->startAdvancing();
		else if (KeyEnter.up()) cccursor->stop();
		else if (_ju.isSettled() && KeyBackspace.down()) {
			if (cccursor->pos() == *_ga->cursor())
				cccursor->startRetreating();
			else
				_inputManager.deleteLightChar(_ga);
		}
		else if (KeyBackspace.up()) cccursor->stop();
		if (!cccursor->isStable()) addend = U"";
		if (KeyDown.down() || KeyUp.down()) {
			_inputManager.deleteLightChar(_ga);
			_inputManager.stopInputting();
		}
	}
	if (!_inputManager.isInputing()) {
		if (KeyDown.down()) _ga->next(_ga->cursor());
		if (KeyUp.down()) _ga->prev(_ga->cursor());
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

	_inputManager.update(_ga, addend, editing);
	if (!floating->isInactive()) cccursor->update(_tmpData);

	_tmpData.update();

	Vec2OnText maskStart, maskEnd;
	_masker.clear(Palette::White);
	_maskee.clear(ColorF(0, 0, 0, 0));
	_area.draw(Palette::White);
	_foreground.clear(ColorF(0, 0, 0, 0));
	{
		BlendState bs = BlendState::Default;
		bs.srcAlpha = Blend::One;
		bs.dstAlpha = Blend::InvSrcAlpha;
		const ScopedRenderStates2D renderState(bs);
		const ScopedRenderTarget2D target(_maskee);
		const Transformer2D transformer(Mat3x2::Translate(_textArea.origin - _textArea.realPos(_textDir)));

		auto drawCursor = [&](Vec2 pos) {
			ScopedRenderTarget2D tmp(_foreground);
			pos += _textArea.realPos(_textDir);
			double t = Scene::Time() * 2 * Math::Pi / 2.5;
			_font->getCursor(pos).draw(1, Color(Palette::Black, (Sin(t) + 1) / 2 * 255));
		};

		using GA = GlyphArrangement2;
		auto litr = _ga->origin();
		bool flt = false;
		PointOnText lineOrigin = _ga->originPos();

		while (lineOrigin.prp < _textArea.size.prp +_lineInterval && litr != _ga->end()) {
			auto citr = _ga->lineBegin(litr);
			while (citr != _ga->lineEnd(litr)) {
				if (!floating->isInactive() && floating->floatingBegin() == citr) flt = true;

				Vec2OnText tp = (lineOrigin + citr.second->pos).toTextVec2();
				if (flt) tp = lineOrigin.toTextVec2() + floating->getPos(citr);
				Vec2 p = tp.toRealPos(_textDir);
				if (_maskee.region().stretched(_lineInterval, 0).contains(p)) {
					if (!CharAnimation::IsEmpty(citr.second->animation)) {
						citr.second->animation->draw(p, citr.second->glyph);
					}
					else citr.second->glyph->draw(p);
				}
				if (citr == *_ga->cursor()) {
					drawCursor(p);
					maskEnd = tp;
				}
				if (!floating->isInactive() && citr == cccursor->drawingPos().first) {
					//TODO: 横書き対応
					Vec2OnText cp = tp + Vec2OnText(cccursor->drawingPos().second, 0);
					drawCursor(cp.toRealPos(_textDir));
					maskStart = cp;
				}
				citr = _ga->next(citr);
			}
			lineOrigin.prp += litr->wrapCount * _lineInterval;
			litr = _ga->tryNext(litr);
		}
	}
	if (floating->isFloating()) {
		const ScopedRenderTarget2D target(_masker);
		const Transformer2D transformer(Mat3x2::Translate(_textArea.origin - _textArea.realPos(_textDir)));

		int lines = (int)((maskEnd.prp - maskStart.prp) / _lineInterval + 0.5);
		Vec2OnText st = maskStart;
		ColorF color(Palette::Black, 0.5);
		double asd = _font->ascender(), dsd = _font->descender();
		double lineGap = _lineInterval - (asd - dsd);
		asd += lineGap / 2; dsd -= lineGap / 2;
		for (int i = 0; i < lines; i++) {
			RectFOnText r({ st.prl, st.prp - asd }, { _textArea.size.prl - st.prl, asd - dsd });
			r.toRealRect(_textDir).draw(color);
			st = Vec2OnText(0, st.prp + _lineInterval);
		}
		RectFOnText r({ st.prl, st.prp - asd }, { maskEnd.prl - st.prl, asd - dsd });
		r.toRealRect(_textDir).draw(color);
	}
	{
		Graphics2D::SetTexture(1, _masker);
		const ScopedCustomShader2D shader(_maskPS); //TODO: _maskPSのプリコンパイル
		_maskee.draw(_textArea.realPos(_textDir));
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

SP<GlyphArrangement2> WholeView::glyphArrangement() const {
	return _ga;
}

void WholeView::jump(GlyphArrangement2::LineIterator litr) {
	_ga->resetOrigin(litr, PointOnText(0, _textArea.size.prp / 2));
}

GlyphArrangement2::LineIterator GlyphArrangement2::initLine(LineIterator litr) {
	if (litr->cd.empty()) {
		return _data.erase(litr);
	}
	PointOnText pen(0, 0);
	decltype(litr->cd)::iterator itr = litr->cd.begin();
	int wrapCount = 0;
	while (itr != litr->cd.end()) {
		if (pen.parallel > _maxLineLnegth - _font->getFontSize()) {
			pen = PointOnText(0, pen.perpendicular + _lineInterval);
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
		pen.parallel += itr->glyph->advance();
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
		int textureLength = (int)(sr * (size + 1) * _lineInterval + 1);
		TextArea ta(Point(0, 0), PointOnText((int)(sr * _maxLineLnegth + 1), textureLength));
		ta.origin -= ta.realPos(_textDir);
		double ascender = _font->ascender() * sr;
		MSRenderTexture msrt(ta.realSize(_textDir));
		msrt.clear(ColorF(Palette::White, 0));
		{
			double a = 3;
			const ScopedColorMul2D mul(1, a);
			const ScopedRenderTarget2D target(msrt);
			BlendState bs = BlendState::Default;
			bs.srcAlpha = Blend::One;
			bs.dstAlpha = Blend::InvSrcAlpha;
			const ScopedRenderStates2D state(bs);
			LineIterator b = head;
			Vec2OnText lineOrigin(0, ascender);
			while (b != bl) {
				for (auto c : b->cd) {
					Vec2OnText tp = (lineOrigin + c.pos.toTextVec2() * sr);
					c.blurred->draw(ta.origin + tp.toRealPos(_textDir), Palette::White, 0, sr);
				}
				lineOrigin.prp += sr * b->wrapCount * _lineInterval;
				b = tryNext(b);
			}
		}
		Graphics2D::Flush();
		msrt.resolve();

		head->bucketHeader->minimap = msrt;
		head->bucketHeader->wrapCount = size;
		head->bucketHeader->advance = (int)(sr * size * _lineInterval + 1);

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

GlyphArrangement2::GlyphArrangement2(SP<Font::FixedFont> font, int lineInterval, int maxLineLength, TD::Direction textDir)
: _font(font)
, _blurredFont(new Font::FixedFont(font->ftLibrary(), font->ftFace(), font->getFontSize(), font->isVertical()))
, _data()
, _lineInterval(lineInterval)
, _maxLineLnegth(maxLineLength)
, _textDir(textDir)
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
	_originPos.prp += delta;
	while (_origin != _data.end() && -_lineInterval > _originPos.prp + _origin->wrapCount * _lineInterval) {
		_originPos.prp += _origin->wrapCount * _lineInterval;
		_origin = std::next(_origin);
	}
	while (_origin != _data.begin() && _lineInterval < _originPos.prp) {
		_origin = std::prev(_origin);
		_originPos.prp -= _origin->wrapCount * _lineInterval;
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
	if (citr->first == _data.end()) return;
	removeItr(citr);
	*citr = next(*citr, true);
	citr->second->itrs.push_back(citr);
}

void GlyphArrangement2::prev(SP<CharIterator> citr) {
	if (*citr == lineBegin(begin())) return;
	removeItr(citr);
	*citr = prev(*citr, true);
	citr->second->itrs.push_back(citr);
}

GlyphArrangement2::LineIterator GlyphArrangement2::origin() const {
	return _origin;
}

PointOnText GlyphArrangement2::originPos() const {
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

void GlyphArrangement2::resetOrigin(LineIterator origin, PointOnText pos) {
	_origin = origin;
	_originPos = pos;
	scroll(0);
}

TextDirection::Direction GlyphArrangement2::textDirection() const {
	return _textDir;
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

Vec2OnText FloatingAnimation::easeOverLine(Vec2OnText source, Vec2OnText target, double t, int i) {
	double rate = EaseOut(Easing::Expo, 3.0, 1.0, std::min(1.0, i / 200.0));
	t = std::min(1.0, t * rate);

	int lineDiff = std::round((target.prp - source.prp) / _lineInterval);
	double sum = target.prl + _maxLineLength * lineDiff - source.prl;
	double delta = EaseInOut(Easing::Sine, t) * sum;
	double flr = std::floor((delta + source.prl) / _maxLineLength);
	return Vec2OnText(source.prl + delta - flr * _maxLineLength, source.prp + flr * _lineInterval);
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

bool FloatingAnimation::isFloating() const {
	return _step == Step::Floating;
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
		_oldHeadPos[i] = _newHeadPos[i] = ritr->pos.toTextVec2();
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
		_newHeadPos[i] = ritr->pos.toTextVec2();
	}
	_updateAP.start(2.0);
}

Vec2OnText FloatingAnimation::getPos(GA::CharIterator citr) {
	assert(_step != Step::Inactive);
	Vec2OnText d(0, 2 * _lineInterval);
	if (_step == Step::Floating) {
		d *= EaseOut(Easing::Expo, _inOutAP.getProgress());
	}
	else if (_step == Step::Stopping) {
		d *= EaseIn(Easing::Expo, 1 - _inOutAP.getProgress());
	}

	if (citr.first != _floatingBegin->first) {
		double t = _updateAP.getProgress();
		Vec2OnText e = Vec2OnText(0, (-_oldAdvance) - (-_newAdvance)) * EaseOut(Easing::Expo, t);
		return citr.second->pos.toTextVec2() + d + e;
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
	_fa->updateTime();
	return _fa;
}

SP<CleanCopyCursor> InputManager::cleanCopyCursor() const {
	return _cccursor;
}

void InputManager::update(SP<GA> ga, String addend, String editing) {
	if (!_isInputing) {
		addend = editing = U"";
	}
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

void InputManager::stopInputting() {
	_isInputing = false;
	_editing = U"";
	_fa->stopFloating();
}

void InputManager::startInputting(SP<GA> ga) {
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
	_fa->updatePos(*ga);
}

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
			double advance = _drawingPos.first->second->glyph->advance();
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
			double advance = _drawingPos.first->second->glyph->advance();
			_drawingPos.second = advance - velocity * _sw.sF();
			if (_drawingPos.second > 0) break;
			registerUnpaint(tmpData, *_drawingPos.first);
			if (*_drawingPos.first == _ga->lineBegin(_ga->begin())) {
				_drawingPos.second = 0;
				_sw.pause();
				return; //_stepはStableにしない
			}
			_ga->prev(_drawingPos.first);
			double newAdvance = _drawingPos.first->second->glyph->advance();
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
, _body(area, ga->textDirection())
, _ga(ga)
, _mapDir(TD::SwapAxis(_ga->textDirection())){
}

GlyphArrangement2::LineIterator MinimapView::draw() {
	GA::LineIterator ret = _ga->end();

	BlendState bs = BlendState::Default;
	bs.srcAlpha = Blend::One;
	bs.dstAlpha = Blend::InvSrcAlpha;
	const ScopedRenderStates2D state(bs);
	_area.draw(Palette::White);
	{
		const ScopedViewport2D viewport(_body.toRect(_mapDir));
		const Transformer2D inv(Mat3x2::Translate(-_body.realPos(_mapDir)), Mat3x2::Identity());
		ColorF c = Palette::Blueviolet;
		const Transformer2D transformer(Mat3x2::Translate(_body.origin));
		//ScopedColorAdd2D colorAdd(c);
		//ScopedColorMul2D colorMul(ColorF(1, 1, 1) - c);
		double li = _ga->minimapLineInterval();
		GA::LineIterator bucket = _ga->begin();
		PointOnText pen(0, 0);
		while (bucket != _ga->end()) {
			if (_body.size.prp < pen.prp) break;
			GA::LineIterator nextBucket = _ga->nextBucket(_ga->tryNext(bucket));
			SP<GA::BucketHeader> header = bucket->bucketHeader;
			const auto& map = header->minimap;
			
			while (true) {
				//Circle(pen.toRealPos(_mapDir), 3).draw(Palette::Red);
				RectOnText mapRect(pen, PointOnText(map.size(), _mapDir));
				map.draw(mapRect.toRealRect(_mapDir).pos, Palette::Black);
				//mapRect.toRealRect(_mapDir).drawFrame(2.0, Palette::Red);
				if (mapRect.toRealRect(_mapDir).contains(Cursor::Pos())) {
					//header->minimap.region(pen + Point(delta - header->minimap.width(), 0)).draw(Palette::Orange);
					//RectF(xBegin, pen.y, xEnd - xBegin, map.height()).draw(Palette::Red);
					GA::LineIterator litr = bucket;
					double prl = pen.prl;
					double nprl = prl;
					while (litr != nextBucket) {
						nprl = prl + litr->wrapCount * li;
						if (nprl > Vec2OnText(Cursor::PosF(), _mapDir).prl) break;
						prl = nprl;
						litr = _ga->tryNext(litr);
					}

					if (litr != nextBucket) {
						ret = litr;
						if (MinimapHighlight::IsEmpty(litr->highlight)) {
							RectFOnText tr(Vec2OnText(prl, pen.prp) - mapRect.pos.toTextVec2(), Vec2OnText(nprl - prl, mapRect.size.prp));
							RectF r = tr.toRealRect(_mapDir);
							Vec2 pos = Vec2OnText(prl, pen.prp).toRealPos(_mapDir);
							litr->highlight.reset(new MinimapHighlight(map, r, pos));
							_tmpManager.registerPointer(litr->highlight);
						}
						litr->highlight->keep();
					}
				}
				if (pen.prl + mapRect.size.prl <= _body.size.prl) break;
				pen += PointOnText(-_body.size.prl, mapRect.size.prp + (int)(li * 2));
			}

			pen.prl += header->advance;
			bucket = nextBucket;
		}
	}
	_tmpManager.update();
	return ret;
}

}
}