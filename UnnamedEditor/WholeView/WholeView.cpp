#include "stdafx.h"
#include "WholeView.h"
#include <iterator>
#include <optional>


namespace UnnamedEditor {


namespace WholeView {


std::pair<TG::Vec2OnText, TG::Vec2OnText> WholeView::drawBody(const RenderTexture& maskee, const RenderTexture& foreground) {
	BlendState bs = BlendState::Default;
	bs.srcAlpha = Blend::One;
	bs.dstAlpha = Blend::InvSrcAlpha;
	const ScopedRenderStates2D renderState(bs);
	const ScopedRenderTarget2D target(_maskee);
	Point textOrigin = _textArea.origin - _textArea.realPos(_textDir);

	auto drawCursor = [&](TG::Vec2OnText tp) {
		ScopedRenderTarget2D tmp(_foreground);
		Vec2 p = tp.toRealPos(_textDir) + _textArea.origin;
		double t = Scene::Time() * 2 * Math::Pi / 2.5;
		_font->getCursor(p).draw(1, Color(Palette::Black, (Sin(t) + 1) / 2 * 255));
	};

	using GA = GlyphArrangement2;
	auto sitr = _ga->origin();
	bool flt = false;
	SP<FloatingAnimation> floating = _inputManager.floatingAnimation();
	auto cccursor = _inputManager.cleanCopyCursor();
	TG::PointOnText sectionOrigin = _ga->originPos();
	TG::Vec2OnText maskStart, maskEnd;
	//Print << sectionOrigin.prp;

	while (sectionOrigin.prp < _textArea.size.prp +_lineInterval && sitr != _ga->end()) {
		if (_cursor->pos().first == sitr) {
			drawCursor(_cursor->drawingPos(sectionOrigin));
			auto cp = sectionOrigin + _cursor->pos().second->pos;
			_scrollDelta.scroll((cp.prp - _textArea.size.prp / 2) / _lineInterval);
		}

		auto citr = _ga->sectionBegin(sitr);
		while (citr != _ga->sectionEnd(sitr)) {
			if (!floating->isInactive() && floating->floatingBegin() == citr) flt = true;

			TG::Vec2OnText tp = (sectionOrigin + citr.second->pos).toTextVec2();
			if (flt) tp = sectionOrigin.toTextVec2() + floating->getPos(citr);
			Vec2 p = tp.toRealPos(_textDir) + textOrigin;
			//Circle(p, 3).draw(Palette::Red);
			if (_maskee.region().stretched(_lineInterval, _font->getFontSize()).contains(p)) {
				if (!CharAnimation::IsEmpty(citr.second->animation)) {
					citr.second->animation->draw(p, citr.second->glyph);
				}
				else
					citr.second->glyph->draw(p);
			}
			if (citr == _cursor->pos()) {
				maskEnd = tp;
			}
			if (floating->isFloating() && citr == cccursor->drawingPos().first) {
				//TODO: 横書き対応
				TG::Vec2OnText cp = tp + TG::Vec2OnText(cccursor->drawingPos().second, 0);
				drawCursor(cp);
				maskStart = cp;
			}
			citr = _ga->tryNext(citr);
		}
		sectionOrigin.prp += sitr->wrapCount * _lineInterval;
		sitr = _ga->tryNext(sitr);
	}
	return { maskStart, maskEnd };
}

void WholeView::drawMasker(const RenderTexture& masker, TG::Vec2OnText maskStart, TG::Vec2OnText maskEnd) {
	const ScopedRenderTarget2D target(masker);
	const Transformer2D transformer(Mat3x2::Translate(_textArea.origin - _textArea.realPos(_textDir)));

	int lines = (int)((maskEnd.prp - maskStart.prp) / _lineInterval + 0.5);
	TG::Vec2OnText st = maskStart;
	ColorF color(Palette::Black, 0.5);
	double asd = _font->ascender(), dsd = _font->descender();
	double lineGap = _lineInterval - (asd - dsd);
	asd += lineGap / 2; dsd -= lineGap / 2;
	for (int i = 0; i < lines; i++) {
		TG::RectFOnText r({ st.prl, st.prp - asd }, { _textArea.size.prl - st.prl, asd - dsd });
		r.toRealRect(_textDir).draw(color);
		st = TG::Vec2OnText(0, st.prp + _lineInterval);
	}
	TG::RectFOnText r({ st.prl, st.prp - asd }, { maskEnd.prl - st.prl, asd - dsd });
	r.toRealRect(_textDir).draw(color);
}

WholeView::WholeView(const Rect area, SP<Font::FixedFont> font, TG::Direction textDir)
: _area(area)
, _textArea(Rect(_area.pos.asPoint() + font->getFontSize() * Point(1, 1), _area.size.asPoint() - 2 * font->getFontSize() * Point(1, 1)), textDir)
, _lineInterval((int)(font->getFontSize()*1.25))
, _textDir(textDir)
, _font(font)
, _ga(new GlyphArrangement2(_font, _lineInterval, _textArea.size.prl, textDir))
, _ju()
, _scrollDelta(_lineInterval)
, _cursor(new EditCursor(_ga, _ga->sectionBegin(_ga->begin())))
, _inputManager(_ga, _cursor, _lineInterval, _textArea.size.prl)
, _masker(_textArea.realSize(textDir))
, _maskee(_textArea.realSize(textDir))
, _foreground(area.size)
, _maskPS(U"example/shader/2d/multi_texture_mask" SIV3D_SELECT_SHADER(U".hlsl", U".frag"), { { U"PSConstants2D", 0 } }) {
	jump(_ga->begin());
}

void WholeView::setText(const String &text) {
	auto citr = _ga->insertText(_cursor->pos(), text);
	_cursor->move(citr);
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

	TG::PointOnText arrowKey = [&]() {
		Point p(0, 0);
		if (KeyLeft.down()) p.x--;
		if (KeyRight.down()) p.x++;
		if (KeyUp.down()) p.y--;
		if (KeyDown.down()) p.y++;
		return TG::PointOnText(p, _textDir);
	}();
	//if (arrowKey.prp > 0 && _ga->nextLineHead(_cursor->pos()).first == _ga->end()) {
	//	arrowKey.prp = 0;
	//}
	//if (arrowKey.prp < 0 && _ga->lineHead(_cursor->pos()) == _ga->sectionBegin(_ga->begin())) {
	//	arrowKey.prp = 0;
	//}

	if (!_inputManager.isInputing() && (addend.size() > 0 || editing.size() > 0 || KeyBackspace.down())) {
		_inputManager.startInputting();
	}
	auto cccursor = _inputManager.cleanCopyCursor();

	if (_inputManager.isInputing() && editing.size() == 0) {
		bool onEnd = _cursor->pos() == cccursor->drawingPos().first && cccursor->isStable();
		if (!cccursor->isStable()) addend = U"";
		if (KeyEnter.down() && !onEnd && _ju.isEditing()) cccursor->startAdvancing();
		else if (KeyEnter.up()) cccursor->stop();
		else if (_ju.isEditing() && KeyBackspace.down()) {
			if (cccursor->pos() == _cursor->pos())
				cccursor->startRetreating();
			else
				_inputManager.deleteLightChar();
		}
		else if (KeyBackspace.up()) cccursor->stop();
		if (!cccursor->isStable()) addend = U"";
		if (arrowKey.prl != 0 || arrowKey.prp != 0) {
			_inputManager.deleteLightChar();
			_inputManager.stopInputting();
		}
	}
	bool movesCursor = false;
	if (!_inputManager.isInputing()) {
		if (arrowKey.prl != 0 || arrowKey.prp != 0) movesCursor = true;
	}
	if (movesCursor) {
		auto sitr = _ga->origin();
		auto sectionOrigin = _ga->originPos();
		while (sectionOrigin.prp < _textArea.size.prp +_lineInterval && sitr != _ga->end()) {
			if (_cursor->pos().first == sitr) break;
			sectionOrigin.prp += sitr->wrapCount * _lineInterval;
			sitr = _ga->tryNext(sitr);
		}
		_cursor->moveAnimation(arrowKey, sectionOrigin);
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
	double offset = 0;
	if (_scrollDelta.isScrolling()) {
		auto [delta, offset_] = _scrollDelta.useDelta();
		offset = offset_;
		_ga->scroll(-delta);
		_cursor->scroll(-delta);
	}

	_inputManager.update(addend, editing);
	if (cccursor) cccursor->update(_tmpData);

	_tmpData.update();
	_cursor->update();

	if (addend.includes(U'\n')) _inputManager.stopInputting();

	_masker.clear(Palette::White);
	_maskee.clear(ColorF(0, 0, 0, 0));
	_area.draw(Palette::White);
	_foreground.clear(ColorF(0, 0, 0, 0));
	Vec2 offsetVec = (TG::Vec2OnText(0, -1)*offset).toRealPos(_textDir);
	const Transformer2D transform(Mat3x2::Translate(offsetVec));
	{
		auto [maskStart, maskEnd] = drawBody(_maskee, _foreground);
		if (floating->isFloating()) {
			drawMasker(_masker, maskStart, maskEnd);
		}
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

void WholeView::jump(GlyphArrangement2::SectionIterator sitr) {
	if (sitr == _ga->end()) sitr = _ga->tryPrev(sitr);
	if (_inputManager.isInputing()) _inputManager.stopInputting();
	_ga->resetOrigin(sitr, TG::PointOnText(0, _textArea.size.prp / 2));
	_cursor->move(_ga->sectionBegin(sitr));
}

SP<const EditCursor> WholeView::cursor() const {
	return _cursor;
}

GlyphArrangement2::SectionIterator GlyphArrangement2::initSection(SectionIterator sitr) {
	TG::PointOnText pen(0, 0);
	decltype(sitr->cd)::iterator itr = sitr->cd.begin();
	int wrapCount = 0;
	while (itr != sitr->cd.end()) {
		if (pen.parallel > _maxLineLnegth - _font->getFontSize()) {
			pen = TG::PointOnText(0, pen.perpendicular + _lineInterval);
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
		pen.parallel += (int)(itr->glyph->advance() + 0.5);
		for (auto itrtrtritrtr = itr->itrs.begin(); itrtrtritrtr != itr->itrs.end();) {
			if (itrtrtritrtr->use_count() <= 1) itrtrtritrtr = itr->itrs.erase(itrtrtritrtr);
			else {
				**itrtrtritrtr = CharIterator(sitr, itr);
				++itrtrtritrtr;
			}
		}
		if (itr->code == Text::Text::NULL_CHAR && itr->itrs.empty()) { //要らなくなったNULL文字の削除
			itr = sitr->cd.erase(itr);
		}
		else itr = std::next(itr);
	}
	if (sitr->cd.empty()) return _data.erase(sitr);
	sitr->wrapCount = wrapCount + 1;
	return sitr;
}

void GlyphArrangement2::initBucket(SectionIterator first, SectionIterator last) {
	int fs = _font->getFontSize();
	double fs_ = _minimapFontSize;
	double sr = ((double)fs_) / fs;
	int normalSize = 1000 * fs / _maxLineLnegth;

	if (first == last) return;
	first = bucket(first);
	last = nextBucket(last);
	SectionIterator prevBucket = bucket(tryPrev(first));
	if (first->bucketHeader && prevBucket->bucketHeader &&
		first->bucketHeader->wrapCount + prevBucket->bucketHeader->wrapCount < normalSize) {
		first = prevBucket;
	}

	SectionIterator head = first;
	bool checkedLast = false;
	while (head != end() && !checkedLast) {
		head->bucketHeader.reset(new BucketHeader());
		SectionIterator bl = head;
		int size = head->wrapCount;
		bl = tryNext(bl);
		while (bl != end()) {
			if (bl == last) checkedLast = true;
			int add = bl->wrapCount;
			SectionIterator next = tryNext(bl);
			if (bl->bucketHeader) {
				add = bl->bucketHeader->wrapCount;
				next = nextBucket(next);
			}
			if (normalSize < size + add) break;
			size += add;
			bl = next;
		}
		int textureLength = (int)(sr * (size + 1) * _lineInterval + 1);
		TG::TextArea ta(Point(0, 0), TG::PointOnText((int)(sr * _maxLineLnegth + 1), textureLength));
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
			SectionIterator b = head;
			TG::Vec2OnText sectionOrigin(0, ascender);
			while (b != bl) {
				for (auto c : b->cd) {
					TG::Vec2OnText tp = (sectionOrigin + c.pos.toTextVec2() * sr);
					c.blurred->draw(ta.origin + tp.toRealPos(_textDir), Palette::White, 0, sr);
				}
				sectionOrigin.prp += sr * b->wrapCount * _lineInterval;
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

GlyphArrangement2::SectionIterator GlyphArrangement2::tryNext(SectionIterator sitr, int cnt) {
	for (int i = 0; i < cnt && sitr != end(); i++) ++sitr;
	return sitr;
}

GlyphArrangement2::SectionIterator GlyphArrangement2::tryPrev(SectionIterator sitr, int cnt) {
	for (int i = 0; i < cnt && sitr != begin(); i++) --sitr;
	return sitr;
}

void GlyphArrangement2::registerItr(SP<CharIterator> itr) {
	itr->second->itrs.push_back(itr);
}

void GlyphArrangement2::moveItr(SP<CharIterator> itr, CharIterator citr) {
	if (*itr == sectionBegin(begin())) VAIN_STATEMENT;
	if (citr == sectionBegin(begin())) VAIN_STATEMENT;
	if (*itr == citr) return;
	SectionIterator oldSection = itr->first;
	auto& itrs = itr->second->itrs;
	for (auto i = itrs.begin(); i != itrs.end();) {
		if (itr == *i) i = itrs.erase(i);
		else ++i;
	}
	*itr = citr;
	itr->second->itrs.push_back(itr);
	initSection(oldSection);
}

GlyphArrangement2::ManagedIterator GlyphArrangement2::registerItr(CharIterator citr) {
	return ManagedIterator(*this, citr);
}

void GlyphArrangement2::removeItr(SP<CharIterator> itr) {
	auto& itrs = itr->second->itrs;
	for (auto i = itrs.begin(); i != itrs.end();) {
		if (itr == *i) i = itrs.erase(i);
		else ++i;
	}
	initSection(itr->first);
}

GlyphArrangement2::GlyphArrangement2(SP<Font::FixedFont> font, int lineInterval, int maxLineLength, TG::Direction textDir)
	: _font(font)
	, _blurredFont(new Font::FixedFont(font->ftLibrary(), font->ftFace(), font->getFontSize(), font->isVertical()))
	, _data()
	, _lineInterval(lineInterval)
	, _maxLineLnegth(maxLineLength)
	, _textDir(textDir)
	, _origin()
	, _originPos(0, 0)
	, _lastNewline([&]() {
		_data.push_back(SectionData());
		CharData cd;
		cd.code = Text::Text::NEWLINE;
		_data.back().cd.push_back(cd);
		return registerItr(sectionBegin(--_data.end()));
	}())
	, _endChar([&]() {
		_data.push_back(SectionData());
		return makeNull(sectionEnd(--_data.end()));
	}()) {

	auto blur = [&](Image& img) {
		img.gaussianBlur(2, 2);
	};
	_blurredFont->setRetouchImage(blur);
}

GlyphArrangement2::CharIterator GlyphArrangement2::insertText(CharIterator citr, const String& s) {
	if (s.empty()) return citr;
	SectionIterator sitr = citr.first;
	if (!s.includes(Text::Text::NEWLINE)) {
		std::vector<CharData> cds;
		for (auto c : s) {
			cds.push_back(CharData());
			cds.back().code = c;
		}
		citr.second = sitr->cd.insert(citr.second, cds.begin(), cds.end());
		initSection(sitr);
		initBucket(sitr, tryNext(sitr));
		return citr;
	}
	std::vector<CharData> tail(citr.second, sitr->cd.end());
	int idx = citr.second - sitr->cd.begin();
	sitr->cd.erase(citr.second, sitr->cd.end());
	SectionIterator ret0 = sitr;
	for (auto c : s) {
		CharData cd;
		cd.code = c;
		sitr->cd.push_back(cd);
		if (c == Text::Text::NEWLINE) {
			initSection(sitr);
			sitr = std::next(sitr);
			SectionData ld;
			sitr = _data.insert(sitr, ld);
		}
	}
	sitr->cd.insert(sitr->cd.end(), tail.begin(), tail.end());
	initSection(sitr);
	initBucket(citr.first, tryNext(sitr));
	return { ret0, ret0->cd.begin() + idx };
}

GlyphArrangement2::CharIterator GlyphArrangement2::eraseText(CharIterator first, CharIterator last) {
	if (first == last) return first;
	
	//管理されたイテレータの退避
	for (auto itr = first; itr != last; itr = tryNext(itr, true)) {
		for (auto managedItr : itr.second->itrs) {
			*managedItr = last;
			last.second->itrs.push_back(managedItr);
		}
	}
	if (first.first == last.first) {
		auto ret1 = first.first->cd.erase(first.second, last.second);
		auto sitr = initSection(first.first);
		initBucket(sitr, tryNext(sitr));
		return { sitr, ret1 };
	}
	SectionIterator sitr = std::next(first.first);
	while (sitr != last.first) {
		sitr = _data.erase(sitr);
	}
	first.first->cd.erase(first.second, first.first->cd.end());
	auto lfirst = initSection(first.first);
	auto ret1 = last.first->cd.erase(last.first->cd.begin(), last.second);
	if (last.first->cd.empty()) ret1 = std::next(last.first->cd.begin());
	auto llast = initSection(last.first);
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

GlyphArrangement2::CharIterator GlyphArrangement2::tryNext(CharIterator citr, bool overSection, int cnt) {
	for (int i = 0; i < cnt; i++) {
		if (citr == sectionEnd(citr.first)) return citr;
		if (overSection && citr == *_endChar) return citr;
		citr.second = std::next(citr.second);
		if (overSection && citr.second == citr.first->cd.end()) {
			citr.first = std::next(citr.first);
			citr.second = citr.first->cd.begin();
		}
	}
	return citr;
}

GlyphArrangement2::CharIterator GlyphArrangement2::tryPrev(CharIterator citr, bool overSection, int cnt) {
	for (int i = 0; i < cnt; i++) {
		if (citr == beginChar()) return citr;
		if (!overSection && citr.second == citr.first->cd.begin()) return citr;
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

GlyphArrangement2::SectionIterator GlyphArrangement2::origin() const {
	return _origin;
}

TG::PointOnText GlyphArrangement2::originPos() const {
	return _originPos;
}

GlyphArrangement2::SectionIterator GlyphArrangement2::begin() {
	return _data.begin();
}

GlyphArrangement2::SectionIterator GlyphArrangement2::end() {
	return _endChar->first;
}

GlyphArrangement2::CharIterator GlyphArrangement2::sectionBegin(SectionIterator sitr) const {
	return { sitr, sitr->cd.begin() };
}

GlyphArrangement2::CharIterator GlyphArrangement2::sectionEnd(SectionIterator sitr) const {
	return { sitr, sitr->cd.end() };
}

GlyphArrangement2::CharIterator GlyphArrangement2::beginChar() {
	return sectionBegin(begin());
}

GlyphArrangement2::CharIterator GlyphArrangement2::endChar() {
	return *_endChar;
}

GlyphArrangement2::CharIterator GlyphArrangement2::lastNewline() {
	return *_lastNewline;
}

GlyphArrangement2::SectionIterator GlyphArrangement2::bucket(SectionIterator i) {
	while (i != _data.begin() && !i->bucketHeader) {
		i = tryPrev(i);
	}
	return i;
}

GlyphArrangement2::SectionIterator GlyphArrangement2::nextBucket(SectionIterator i) {
	while (i != end() && !i->bucketHeader) {
		i = tryNext(i);
	}
	return i;
}

double GlyphArrangement2::minimapLineInterval() const {
	return _minimapFontSize / (double)_font->getFontSize() * _lineInterval;
}

void GlyphArrangement2::resetOrigin(SectionIterator origin, TG::PointOnText pos) {
	_origin = origin;
	_originPos = pos;
	scroll(0);
}

TG::Direction GlyphArrangement2::textDirection() const {
	return _textDir;
}

GlyphArrangement2::CharIterator GlyphArrangement2::lineHead(CharIterator citr) {
	while (citr != sectionBegin(citr.first)) {
		auto prv = tryPrev(citr);
		if (prv.second->pos.prp != citr.second->pos.prp) break;
		citr = prv;
	}
	return citr;
}

GlyphArrangement2::CharIterator GlyphArrangement2::nextLineHead(CharIterator citr) {
	assert(citr.first != _data.end());
	while (true) {
		auto nxt = tryNext(citr, true);
		if (citr.first != nxt.first || citr.second->pos.prp != nxt.second->pos.prp) {
			return nxt;
		}
		citr = nxt;
	}
	assert(false);
}

GlyphArrangement2::ManagedIterator GlyphArrangement2::makeNull(CharIterator citr) {
	CharData cd;
	cd.code = Text::Text::NULL_CHAR;
	citr.second = citr.first->cd.insert(citr.second, cd);
	auto ret = registerItr(citr);
	initSection(citr.first);
	return ret;
}

TG::Vec2OnText FloatingAnimation::easeOverLine(TG::Vec2OnText current, TG::Vec2OnText target, double& velocity) {
	int lineDiff = std::round((target.prp - current.prp) / _lineInterval);
	double sum = target.prl + _maxLineLength * lineDiff - current.prl;
	double delta = Math::SmoothDamp(0, sum, velocity, 0.3, Scene::DeltaTime());
	double flr = std::floor((delta + current.prl + 1) / _maxLineLength);
	return TG::Vec2OnText(current.prl + delta - flr * _maxLineLength, current.prp + flr * _lineInterval);
}

FloatingAnimation::FloatingAnimation(SP<GA> ga, int lineInterval, int maxLineLength)
: _ga(ga), _lineInterval(lineInterval), _maxLineLength(maxLineLength) {
	
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
	if (_step == Step::Inactive) throw "inactive";
	return *_floatingBegin.value();
}

void FloatingAnimation::startFloating(GA& ga, GA::CharIterator floatingBegin) {
	_floatingBegin.emplace(ga.registerItr(floatingBegin));
	GA::SectionIterator head = floatingBegin.first;
	_tailPrp = 0;
	_oldAdvance = head->wrapCount * _lineInterval;
	int size = std::distance(floatingBegin.second, head->cd.end());
	_headPos.resize(size);
	_headVelocity.resize(size);
	for (int i = 0; i < size; i++) {
		auto ritr = std::next(floatingBegin.second, i);
		_headPos[i] = ritr->pos.toTextVec2();
	}
	_step = Step::Floating;
	_inOutAP.start(1.5);
}

void FloatingAnimation::stopFloating() {
	_step = Step::Stopping;
	_inOutAP.start(1.5);
}

void FloatingAnimation::update() {
	if (_step == Step::Inactive) return;
	GA::SectionIterator head = _floatingBegin.value()->first;
	int newAdvance = head->wrapCount * _lineInterval;
	_tailPrp -= newAdvance - _oldAdvance;
	_tailPrp = Math::SmoothDamp(_tailPrp, 0, _tailVelocity, 0.3, Scene::DeltaTime());
	_oldAdvance = newAdvance;
	for (int i = 0; i < _headPos.size(); i++) {
		auto ritr = std::next(_floatingBegin.value()->second, i);
		_headPos[i] = easeOverLine(_headPos[i], ritr->pos.toTextVec2(), _headVelocity[i]);
	}
	_inOutAP.update();
}

TG::Vec2OnText FloatingAnimation::getPos(GA::CharIterator citr) {
	assert(_step != Step::Inactive);
	TG::Vec2OnText d(0, 2 * _lineInterval);
	if (_step == Step::Floating) {
		d *= EaseOut(Easing::Expo, _inOutAP.getProgress());
	}
	else if (_step == Step::Stopping) {
		d *= EaseIn(Easing::Expo, 1 - _inOutAP.getProgress());
	}

	if (citr.first != _floatingBegin.value()->first) {
		return citr.second->pos.toTextVec2() + d + TG::Vec2OnText(0, _tailPrp);
	}
	int idx = citr.second - _floatingBegin.value()->second;
	return _headPos[idx] + d;
}

InputManager::InputManager(SP<GA> ga, SP<EditCursor> cursor, int lineInterval, int maxLineLength)
: _fa(new FloatingAnimation(ga, lineInterval, maxLineLength))
, _ga(ga)
, _cursor(cursor) {

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

void InputManager::update(String addend, String editing) {
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
	int eraseSize = _editing.size() - prefixLength;
	if (eraseSize > 0 || !replaced.empty()) {
		auto first = _ga->tryPrev(_cursor->pos(), true, eraseSize);
		bool flg = first == _cccursor->pos();
		auto newItr = _ga->replaceText(first, _cursor->pos(), replaced);
		if (flg) {
			_cccursor->changeItr(newItr);
		}
	}
	_editing = editing;
	_fa->update();
}

void InputManager::stopInputting() {
	_isInputing = false;
	_editing = U"";
	_fa->stopFloating();
	_cccursor.reset();
	_cursor->increasePrl(1);
}

void InputManager::startInputting() {
	_isInputing = true;
	auto null = _ga->makeNull(_cursor->pos());
	_cursor->decreasePrl(1);
	_cccursor.reset(new CleanCopyCursor(_ga, _cursor));
	_fa->startFloating(*_ga, _ga->tryNext(_cursor->pos()));
}

void InputManager::deleteLightChar() {
	_cccursor->changeItr(_ga->eraseText(_cccursor->pos(), _cursor->pos()));
}

CleanCopyCursor::CleanCopyCursor(SP<GA> ga, SP<EditCursor> editCursor)
: _step(Step::Stable)
, _ga(ga)
, _editCursor(editCursor)
, _drawingPos(ga->registerItr(editCursor->pos()), 0) {
}

GlyphArrangement2::CharIterator CleanCopyCursor::pos() const {
	if (_step != Step::Retreating) return *_drawingPos.first;
	return _ga->tryNext(*_drawingPos.first);
}

std::pair<GlyphArrangement2::CharIterator, double> CleanCopyCursor::drawingPos() const {
	return { *_drawingPos.first, _drawingPos.second };
}

void CleanCopyCursor::changeItr(GA::CharIterator newItr) {
	_drawingPos.first.move(newItr);
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
	if (*_drawingPos.first == _ga->sectionBegin(_ga->begin())) return;
	if (_step != Step::Retreating)
		_drawingPos.first.prev();
	_step = Step::Retreating;
	_drawingPos.second = 0;
	_sw.restart();
}

void CleanCopyCursor::stop() {
	if (_step == Step::Retreating && *_drawingPos.first != _editCursor->pos() && _sw.isRunning())
		_drawingPos.first.next();
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
			if (*_drawingPos.first == _editCursor->pos()) {
				_drawingPos.second = 0;
				_sw.pause();
				break; //_stepはStableにしない
			}
			if (_drawingPos.second < advance) break;
			registerPaint(tmpData, *_drawingPos.first);
			_drawingPos.first.next();
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
			if (*_drawingPos.first == _ga->sectionBegin(_drawingPos.first->first)) {
				_drawingPos.second = 0;
				_sw.pause();
				return; //_stepはStableにしない
			}
			_drawingPos.first.prev();
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

MinimapView::MinimapView(Rect area, SP<GA> ga)
: _area(area)
, _ga(ga)
, _mapDir(TG::SwapAxis(ga->textDirection()))
, _body(area.stretched(-10), _mapDir) {
}

GlyphArrangement2::SectionIterator MinimapView::draw(GA::SectionIterator editSction) {
	GA::SectionIterator ret = _ga->end();
	Rect editRect;

	BlendState bs = BlendState::Default;
	bs.srcAlpha = Blend::One;
	bs.dstAlpha = Blend::InvSrcAlpha;
	const ScopedRenderStates2D state(bs);
	_area.draw(Palette::White);
	{
		const ScopedViewport2D viewport(_body.toRect(_mapDir));
		const Transformer2D inv(Mat3x2::Translate(-_body.realPos(_mapDir)), Mat3x2::Identity());
		ColorF c = Palette::Blueviolet;
		const Transformer2D transformer(Mat3x2::Translate(_body.origin), Mat3x2::Translate(_body.origin));
		//ScopedColorAdd2D colorAdd(c);
		//ScopedColorMul2D colorMul(ColorF(1, 1, 1) - c);
		double li = _ga->minimapLineInterval();
		GA::SectionIterator bucket = _ga->begin();
		TG::PointOnText pen(0, 0);
		GA::SectionIterator editBucket = _ga->bucket(editSction);
		//Circle(pen.toRealPos(_mapDir), 100).draw(ColorF(Palette::Red, 0.4));
		//Circle(Cursor::Pos(), 100).draw(ColorF(Palette::Red, 0.4));
		while (bucket != _ga->end()) {
			if (_body.size.prp < pen.prp) break;
			GA::SectionIterator nextBucket = _ga->nextBucket(_ga->tryNext(bucket));
			SP<GA::BucketHeader> header = bucket->bucketHeader;
			const auto& map = header->minimap;
			
			while (true) {
				//Circle(pen.toRealPos(_mapDir), 3).draw(Palette::Blue);
				TG::RectOnText mapRect(pen, G::Abs(TG::PointOnText(map.size(), _mapDir)));
				map.draw(mapRect.toRealRect(_mapDir).pos, Palette::Black);
				//mapRect.toRealRect(_mapDir).drawFrame(2.0, Palette::Red);
				if (bucket == editBucket) {
					GA::SectionIterator sitr = bucket;
					double prl = pen.prl;
					while (sitr != editSction) {
						prl += sitr->wrapCount * li;
						sitr = _ga->tryNext(sitr);
					}
					editRect = TG::RectFOnText(TG::Vec2OnText(prl, pen.prp), TG::Vec2OnText(sitr->wrapCount * li, mapRect.size.prp)).toRealRect(_mapDir);
				}
				if (mapRect.toRealRect(_mapDir).contains(Cursor::Pos())) {
					//header->minimap.region(pen + Point(delta - header->minimap.width(), 0)).draw(Palette::Orange);
					//RectF(xBegin, pen.y, xEnd - xBegin, map.height()).draw(Palette::Red);
					GA::SectionIterator sitr = bucket;
					double prl = pen.prl;
					double nprl = prl;
					while (sitr != nextBucket) {
						nprl = prl + sitr->wrapCount * li;
						if (nprl > TG::Vec2OnText(Cursor::PosF(), _mapDir).prl) break;
						prl = nprl;
						sitr = _ga->tryNext(sitr);
					}

					if (sitr != nextBucket) {
						ret = sitr;
						if (MinimapHighlight::IsEmpty(sitr->highlight)) {
							//TG::RectFOnText tr(TG::Vec2OnText(prl, pen.prp) - mapRect.pos.toTextVec2(), TG::Vec2OnText(nprl - prl, mapRect.size.prp));
							RectF r = TG::RectFOnText(TG::Vec2OnText(prl, pen.prp), TG::Vec2OnText(nprl - prl, mapRect.size.prp)).toRealRect(_mapDir);
							//Circle(r.pos, 3).draw(Palette::Red);
							RectF region = r.movedBy(-mapRect.toRealRect(_mapDir).pos);
							sitr->highlight.reset(new MinimapHighlight(map, region, r.pos));
							_tmpManager.registerPointer(sitr->highlight);
						}
						sitr->highlight->keep();
					}
				}
				if (pen.prl + mapRect.size.prl <= _body.size.prl) break;
				pen += TG::PointOnText(-_body.size.prl, mapRect.size.prp + (int)(li * 2));
			}

			pen.prl += header->advance;
			bucket = nextBucket;
		}
		_tmpManager.update();
	}

	double t = Scene::Time() * 2 * Math::Pi / 2.5;
	ScopedViewport2D viewport(_area);
	const Transformer2D inv(Mat3x2::Translate(-_area.pos), Mat3x2::Identity());
	ColorF c = Palette::Blueviolet;
	const Transformer2D transformer(Mat3x2::Translate(_body.origin), Mat3x2::Translate(_body.origin));
	editRect.drawShadow(Vec2::Zero(), 25, 0, Color(Palette::Black, (Sin(t) + 1) / 2 * 192));

	return ret;
}

}
}