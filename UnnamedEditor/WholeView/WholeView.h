#pragma once
#include "Font\FixedFont.h"
#include <list>


namespace UnnamedEditor {
namespace WholeView {


using DevicePos = Vec2;


//未確定文字列入力モードかどうかを判定するクラス
//未確定文字列を削除した時に、既にその削除のために使われた入力が、確定文字列の削除をやってしまう（ラグのせい）ことへの対処ができる
//（確定文字列への編集を一定時間ブロックするだけ）
class JudgeUnsettled {
private:

	int _count;

public:

	JudgeUnsettled() : _count(0) {}

	//1フレーム一回
	//unsettledStrSize: 現在の未確定文字列の長さ
	void update(int unsettledStrLength) {
		if (unsettledStrLength == 0) _count = std::max(_count - 1, 0);
		else _count = 5;
	}

	bool isUnsettled() { return _count > 0; }

	bool isSettled() { return _count == 0; }
};

class InsertMode {
private:

	RectF _textArea;

	double _lineInterval;

	Vec2 _pos, _tail;

	int _insertIndex;

	SP<Font::FixedFont> _font;

	String _text, _unsettled;

	Stopwatch _sw;

	bool _isActive;
	
	const int animationMS = 700;

public:

	InsertMode(RectF textArea, double lineInterval, int insertIndex, Vec2 pos, SP<Font::FixedFont> font)
	: _textArea(textArea)
	, _lineInterval(lineInterval)
	, _pos(pos)
	, _tail()
	, _insertIndex(insertIndex)
	, _font(font)
	, _text()
	, _unsettled()
	, _sw()
	, _isActive(true) {
		
	}

	int getInsertIndex() { return _insertIndex; }
	
	void update(const String &addend, const String &unsettled, int cursorIndex, bool deleteSettledText) {
		if (!isActive()) return;

		_text += addend;
		_unsettled = unsettled;
		cursorIndex -= _insertIndex;
		if (_sw.ms() >= animationMS) _sw.pause();


		std::vector<SP<const Font::Glyph>> ret = _font->renderString((_text + _unsettled).toUTF16());
		_tail = _pos;
		for (int i = 0; i < (int)ret.size(); i++) {
			if (_textArea.y + _textArea.size.y < (_tail + ret[i]->getAdvance()).y) {
				_tail += Vec2(-_lineInterval, 0);
			}
			_tail += ret[i]->getAdvance();
		}

		if (cursorIndex < 0 || _text.length() + _unsettled.length() < cursorIndex) {
			_isActive = false;
			_sw.restart();
			return;
		}
		if (deleteSettledText && 0 < cursorIndex && cursorIndex <= _text.length()) {
			_text.erase(cursorIndex - 1, 1);
		}
	}

	//アニメーション中でもfalseになるので注意
	bool isActive() { return _isActive; }

	bool isAnimating() { return _sw.isRunning(); }

	Vec2 getNextPen() {
		Vec2 start = _tail, end = _pos + Vec2(-_lineInterval*2, 0);
		if (!isActive()) std::swap(start, end);
		double t = std::max(1.0, _sw.ms()/(double)animationMS);
		return EaseOut(Easing::Back, start, end, t);
	}

	void draw() {
		Vec2 pen(_pos);
		std::vector<SP<const Font::Glyph>> ret = _font->renderString((_text + _unsettled).toUTF16());
		for (int i = 0; i < (int)ret.size(); i++) {
			if (_textArea.y + _textArea.size.y < (pen + ret[i]->getAdvance()).y) {
				pen += Vec2(-_lineInterval, 0);
			}
			Color color = i < _text.length() ? Palette::Red : Color(Palette::Red, 50);
			pen = ret[i]->draw(pen, color);
		}
	}
};


class WholeView {
private:

	DevicePos _borderPos, _borderSize;

	Vec2 _pos, _size;

	SP<Font::FixedFont> _font;

	String _text;

	std::vector<SP<const Font::Glyph>> _glyphs;

	int _pageCount;

	double _lineInterval;

	int _cursorIndex;

	JudgeUnsettled _ju;

	SP<InsertMode> _insertMode;



	int deleteChar(int index);

public:

	//font: verticalでなければならない(これ設計汚くない？)
	WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font);

	void setText(const String &text);

	void update();

	Vec2 getCharPos(int charIndex) const;

	Vec2 getCursorPos(int cursorIndex) const;

};


}
}