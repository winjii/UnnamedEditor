#include "Text.h"
#include <exception>


namespace UnnamedEditor {
namespace Text {


Text::Text(SP<Font::FixedFont> font) : _data(new std::list<CharData>()), _font(font) {
	insert({ _data->begin(), _data }, { NULL_CHAR, NULL_CHAR });
}

Text::Iterator Text::beginSentinel() const {
	return { _data->begin(), _data };
}

Text::Iterator Text::endSentinel() const {
	return { std::prev(_data->end()), _data };
}

Text::Iterator Text::next(const Iterator& itr) const {
	return std::next(itr);
}

Text::Iterator Text::prev(const Iterator& itr) const {
	return std::prev(itr);
}

bool Text::isNewline(const Iterator &itr) const {
//TODO: 色々な改行に対応する
//統一的な内部表現に変換してしまった方が楽？
return itr->code == NEWLINE;
}

Text::Iterator Text::insert(Iterator itr, const String& s) {
	for (auto c : s.reversed()) {
		CharData cd;
		cd.code = c;
		cd.glyph = _font->renderChar(c);
		itr = Iterator(_data->insert(itr, cd), _data);
	}
	return itr;
}

Text::Iterator Text::erase(const Iterator& first, const Iterator& last) {
	return { _data->erase(first, last), _data };
}

int Text::idx(const Iterator& itr) const {
	int ret = 0;
	Iterator i = beginSentinel();
	while (i != itr) {
		i = next(i);
		ret++;
	}
	return ret;
}

std::pair<Text::Iterator, int> Text::lineHead(const Iterator& aitr) const {
	int ret = 0;
	Iterator itr = aitr;
	if (itr == beginSentinel()) return { itr, ret };
	while (true) {
		Iterator itr_ = prev(itr);
		if (itr_ == beginSentinel() || isNewline(itr_)) break;
		ret++;
		itr = itr_;
	}
	return { itr, ret };
}

std::pair<Text::Iterator, int> Text::nextLineHead(const Iterator& aitr) const {
	int ret = 0;
	Iterator itr = aitr;
	while (true) {
		if (itr == endSentinel()) break;
		bool flg = isNewline(itr);
		itr = next(itr);
		ret++;
		if (flg) break;
	}
	return { itr, ret };
}

TextWithHead::TextWithHead(const Text &text, Iterator headEnd, Iterator first)
: Text(text)
, _head([&]() {
	SP<std::list<CharData>> ret(new std::list<CharData>(first, headEnd));
	CharData cd;
	cd.code = NULL_CHAR;
	cd.glyph = _font->renderChar(NULL_CHAR);
	ret->push_front(cd);
	return ret;
	}())
, _headEnd(headEnd) {
	
}

TextWithHead::Iterator TextWithHead::headEnd() const {
	return _headEnd;
}

TextWithHead::Iterator TextWithHead::beginSentinel() const {
	return { _head->begin(), _head };
}

TextWithHead::Iterator TextWithHead::next(const Iterator& itr) const {
	Iterator ret = std::next(itr);
	if (ret == Iterator(_head->end(), _head)) return _headEnd;
	return ret;
}

TextWithHead::Iterator TextWithHead::prev(const Iterator& itr) const {
	if (itr == _headEnd) return std::prev(Iterator(_head->end(), _head));
	return std::prev(itr);
}

}
}