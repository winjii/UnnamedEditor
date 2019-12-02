#include "stdafx.h"
#include "WholeView.h"
#include <iterator>
#include <optional>


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
, _textWindow(SP<Text::Text>(new Text::Text(font)), _area, _lineInterval, _area.tr())
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

	_floatingProgress.update();
	if (_floatingProgress.getStep() == AnimationProgress::Step::Stable) {
		if (_floatingStep == FloatingStep::AnimatingIn)
			_floatingStep = FloatingStep::Stable;
		else if (_floatingStep == FloatingStep::AnimatingOut) {
			_floatingStep = FloatingStep::Inactive;
			_floatingArrangement = nullptr;
		}
	}
	bool isFloating = _floatingStep != FloatingStep::Inactive;

	if (_textWindow.isEditing() && (KeyDown.down() || KeyUp.down())) {
		// Floating終了を開始
		_textWindow.stopEditing();
		_floatingStep = FloatingStep::AnimatingOut;
		_floatingProgress.start(10);
	}
	

	//if (KeyDown.down()) _textWindow.setCursor(_textWindow.nextExtended(_textWindow.cursor()));
	//if (KeyUp.down()) _textWindow.setCursor(_textWindow.prevExtended(_textWindow.cursor()));

	if (!_textWindow.isEditing() && (addend.size() > 0 || editing.size() > 0)) {
		_textWindow.startEditing();
		_floatingArrangement = _textWindow.cloneBack(_textWindow.cursor());
		_floatingStep = FloatingStep::AnimatingIn;
		_floatingProgress.start(1.5);
		isFloating = true;
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
	auto itr = _textWindow.calcDrawBegin();
	auto twEnd = _textWindow.calcDrawEnd();
	std::optional<GlyphArrangement::Iterator> fBegin;
	if (isFloating) fBegin = _floatingArrangement->calcDrawBegin();
	int cnt = 0;
	while (itr != twEnd) {
		if (isFloating && itr.first == fBegin.value().first) break;
		itr.first->glyph->draw(*(itr.second));
		drawCursor(itr);
		itr = _textWindow.nextExtended(itr);
		cnt++;
	}
	if (isFloating) {
		auto fitr = fBegin.value();
		auto fEnd = _floatingArrangement->calcDrawEnd();
		int cnt = 0;
		while (fitr != fEnd) {
			Vec2 p;
			double t = _floatingProgress.getProgress();
			if (_floatingStep == FloatingStep::AnimatingOut) {
				p = floatingTextOut(*(itr.second), *(fitr.second), t, cnt);
			}
			else {
				p = floatingTextIn(*(fitr.second), *(fitr.second) - Vec2(_lineInterval * 2, 0), t, cnt);
			}
			fitr.first->glyph->draw(p);
			drawCursor(fitr);
			fitr = _floatingArrangement->nextExtended(fitr);
			itr = _textWindow.nextExtended(itr);
			cnt++;
		}
	}
}

//ConstText::ConstText(SP<Font::FixedFont> font)
//: _data()
//, _font(font) {
//	CharData cd;
//	cd.code = NULL_CHAR;
//	cd.glyph = font->renderChar(cd.code);
//	_data.push_back(cd);
//}
//
//ConstText::Iterator ConstText::begin() const {
//	return _data.begin();
//}
//
//ConstText::Iterator ConstText::end() const {
//	return _data.end();
//}
//
//ConstText::Iterator ConstText::next(Iterator itr) const {
//	return ++itr;
//}
//
//ConstText::Iterator ConstText::prev(Iterator itr) const {
//	return --itr;
//}
//
//ConstText::Iterator ConstText::insert(Iterator itr, String s) {
//	for (auto c : s.reversed()) {
//		CharData cd;
//		cd.code = c;
//		cd.glyph = _font->renderChar(c);
//		itr = _data.insert(itr, cd);
//	}
//	return itr;
//}
//
//ConstText::Iterator ConstText::erase(Iterator first, Iterator last) {
//	return _data.erase(first, last);
//}
//
//ConstText::Iterator ConstText::erase(Iterator itr) {
//	return _data.erase(itr);
//}
//
//bool ConstText::isNewline(Iterator itr) const
//{
//	//TODO: 色々な改行に対応する
//	//統一的な内部表現に変換してしまった方が楽？
//	return itr->code == NEWLINE;
//}
//
//std::pair<ConstText::Iterator, int> ConstText::lineHead(Iterator itr) const
//{
//	int ret = 0;
//	while (true) {
//		if (itr == begin()) break;
//		Iterator itr_ = prev(itr);
//		if (isNewline(itr_)) break;
//		ret++;
//		itr = itr_;
//	}
//	return { itr, ret };
//}
//
//std::pair<ConstText::Iterator, int> ConstText::nextLineHead(Iterator itr) const
//{
//	int ret = 0;
//	while (true) {
//		if (itr == end()) break;
//		bool flg = isNewline(itr);
//		itr = next(itr);
//		ret++;
//		if (flg) break;
//	}
//	return { itr, ret };
//}

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
	auto lhead = ga.lineHead(begin).first;
	_text.reset(new Text::TextWithHead(*ga._text, begin.first, lhead.first));
	_begin = _text->prev(begin.first);
	_end = _text->endSentinel();
	Iterator nlhead = ga.nextLineHead(begin).first;
	_pos = std::list<Vec2>(lhead.second, std::next(nlhead.second));
	_pos.push_front(Vec2());
	_cacheBegin = { _text->prev(lhead.first), _pos.begin() };
	_cacheEnd = { nlhead.first, std::prev(_pos.end()) };
}

//TODO: 実装（NULL文字が増殖するだけで致命的なバグが起こるわけではないので後回し）
GlyphArrangement::~GlyphArrangement() {
	_pos.clear();
}

//GlyphArrangement::PartitionIterator GlyphArrangement::makePartitionFront(Iterator itr, bool ensuresPos) {
//	Vec2 p;
//	if (ensuresPos) p = *itr.second;
//	itr.first = _text->insert(itr.first, { NULL_CHAR });
//	itr.second = _pos.insert(itr.second, p);
//	return itr;
//}
//
//GlyphArrangement::PartitionIterator GlyphArrangement::makePartitionBack(Iterator itr) {
//	if (itr.first == _end) std::exception("out of range");
//	return makePartitionFront(nextExtended(itr, false), true);
//}
//
//void GlyphArrangement::partitionNext(PartitionIterator& itr) {
//	auto itr_ = makePartitionBack(nextExtended(itr, false));
//	finalizePartition(itr);
//	itr = itr_;
//}
//
//void GlyphArrangement::partitionPrev(PartitionIterator& itr) {
//	auto itr_ = makePartitionFront(prevExtended(itr, false), true);
//	finalizePartition(itr);
//	itr = itr_;
//}
//
//void GlyphArrangement::finalizePartition(PartitionIterator& itr) {
//	_text->erase(itr.first);
//	_pos.erase(itr.second);
//}

GlyphArrangement::Iterator GlyphArrangement::next(Iterator itr) {
	if (itr.first == _end) throw std::exception("out of range");
	if (itr.second == --_pos.end()) _pos.push_back(Vec2());
	return { _text->next(itr.first), ++itr.second };
}

GlyphArrangement::Iterator GlyphArrangement::prev(Iterator itr) {
	if (itr.first == _begin) throw std::exception("out of range");
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

//GlyphArrangement::Iterator GlyphArrangement::eraseSafe(Iterator first, Iterator last) {
//	Iterator itr = first;
//	while (itr != last) {
//		if (itr.first->code != NULL_CHAR) {
//			itr.first = _text->erase(itr.first);
//			itr.second = _pos.erase(itr.second);
//		}
//		else itr = next(itr);
//	}
//	return last;
//}

//TODO: それでいいのか？
//ConstText::Iterator GlyphArrangement::begin() {
//	return _begin;
//}
//
//ConstText::Iterator GlyphArrangement::end() {
//	return _end;
//}

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
	int cb = std::distance(_pos.begin(), _cacheBegin.second);
	int ce = std::distance(_pos.begin(), _cacheEnd.second);
	Iterator ret = nextExtended(_cacheBegin);
	while (ret.first != _text->next(_begin) && !lowerTextArea(ret))
		ret = prevExtended(ret);
	while (ret.first != _end && lowerTextArea(ret))
		ret = nextExtended(ret);
	Iterator lhead = lineHead(ret).first;
	_pos.erase(_pos.begin(), lhead.second);
	_cacheBegin = prev(lhead);
	return ret;
}

GlyphArrangement::Iterator GlyphArrangement::calcDrawEnd() {
	Iterator ret = _cacheEnd;
	while (ret.first != _text->next(_begin) && upperTextArea(ret))
		ret = prevExtended(ret);
	while (ret.first != _end && !upperTextArea(ret))
		ret = nextExtended(ret);
	Iterator nlhead = nextLineHead(ret).first;
	_pos.erase(std::next(nlhead.second), _pos.end());
	_cacheEnd = nlhead;
	return ret;
}

GlyphArrangement::Iterator GlyphArrangement::prevExtended(Iterator itr) {
	if (itr.first == _text->next(_begin)) throw std::exception("out of range");
	if (itr != next(_cacheBegin)) return prev(itr); //グリフ位置キャッシュ済み

	Vec2 p = *itr.second;
	Iterator lhead = lineHead(_cacheBegin).first;
	_cacheBegin = prev(lhead);
	*lhead.second = Vec2(0, 0);
	arrange(lhead, itr);
	double d = p.x - itr.second->x;
	std::transform(_pos.begin(), itr.second, _pos.begin(), [d](Vec2 v) { return v + Vec2(d, 0); });
	return prev(itr);
}

GlyphArrangement::Iterator GlyphArrangement::nextExtended(Iterator itr) {
	if (itr.first == _end) throw std::exception("out of range");
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

//TODO: 無駄なイテレータ計算が多い
//TextWindow::Iterator TextWindow::editText(Iterator destroyed, std::function<Iterator()> edit) {
//	Vec2 origin = [&]() {
//		Iterator lhead = lineHead(destroyed).first;
//		if (lhead != _cacheBegin) return *lhead.second;
//		return *next(lhead).second;
//	}(); //先に座標とっておかないとdestroyed == lineHeadだった場合にどっかいっちゃうことがある
//	auto edited = edit();
//	auto lhead = lineHead(edited).first;
//	auto nlhead = nextLineHead(edited).first;
//	arrange(lhead, nlhead, origin);
//	return edited;
//}

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

UP<GlyphArrangement> TextWindow::cloneBack(Iterator newBegin) {
	return UP<GlyphArrangement>(new GlyphArrangement(*this, newBegin));
}

void TextWindow::startEditing() {
	_isEditing = true;
}

void TextWindow::stopEditing() {
	_isEditing = false;
	//setCursor(eraseText(unsettledBegin(), next(_cursor))); // NULL文字も消す
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

}
}