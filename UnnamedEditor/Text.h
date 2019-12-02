#pragma once
#include "Font\FixedFont.h"


namespace UnnamedEditor {
namespace Text {


struct CharData {
	char16_t code;
	SP<const Font::Glyph> glyph;
};


class Text {
public:
	const char16_t NULL_CHAR = 0;
	const char16_t NEWLINE = U'\n';
	class Iterator : public std::list<CharData>::iterator {
	public:
		using Base = std::list<CharData>::iterator;
	private:
		SP<const std::list<CharData>> _data;
	public:
		Iterator() {}
		Iterator(const Base& base, SP<const std::list<CharData>> data)
			: Base(base), _data(data) {}
		bool operator==(const Iterator& i) const {
			return _data == i._data && Base::operator==(i);
		}
		bool operator!=(const Iterator& i) const {
			return _data != i._data || Base::operator!=(i);
		}
	};
protected:
	SP<std::list<CharData>> _data;
	SP<Font::FixedFont> _font;
public:
	Text(SP<Font::FixedFont> font);
	virtual Iterator beginSentinel() const;
	virtual Iterator endSentinel() const;
	virtual Iterator next(const Iterator& itr) const;
	virtual Iterator prev(const Iterator& itr) const;
	bool isNewline(const Iterator& itr) const;
	Iterator insert(Iterator itr, const String& s);
	Iterator erase(const Iterator& first, const Iterator& last);
	int idx(const Iterator& itr) const; //�f�o�b�O�p

	//[�擪������Iterator, �擪�����܂ł̋���]
	//beginSentinel�͕Ԃ��Ȃ�
	std::pair<Iterator, int> lineHead(const Iterator& itr) const;
	
	//���s��������Ȃ����endSentinel��Ԃ�
	std::pair<Iterator, int> nextLineHead(const Iterator& itr) const;
};


//_head���܂ޔ͈͂�insert��erase�����Ɖ���...
class TextWithHead : public Text {
private:
	SP<std::list<CharData>> _head;
	Iterator _headEnd;
public:
	TextWithHead(const Text& text, Iterator headEnd, Iterator first);
	Iterator headEnd() const;
	virtual Iterator beginSentinel() const;
	virtual Iterator next(const Iterator& itr) const;
	virtual Iterator prev(const Iterator& itr) const;
};


}
}