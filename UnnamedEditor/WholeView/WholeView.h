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


struct CharData {
	char16_t code;
	SP<const Font::Glyph> glyph;
};
class Text {
public:
	using Iterator = std::list<CharData>::iterator;
	Iterator begin();
	Iterator end();
	Iterator next(Iterator itr);
	Iterator prev(Iterator itr);
	void insert(Iterator itr, String s);
	void erase(Iterator first, Iterator last);

};


//TODO: reject関数要らない説(テキストの更新くるまでメモリに乗っけっぱなしでもテキスト本体以上のメモリは食わない)
//テキストを表示するためのグリフ配置を計算する
//テキスト更新すると壊れるので破棄すべし（更新するにはTextWindowを使う）
class GlyphArrangement {
public:
	using Iterator = std::pair<Text::Iterator, std::deque<Vec2>::iterator>;
	int originIndex() const;
	Text::Iterator origin() const;
	Iterator begin() const;
	Iterator end() const;
	Iterator windowStart() const;
	Iterator isOnWindow() const;
	Iterator ensurePrev(Iterator itr);
	Iterator ensureNext(Iterator itr);
	void rejectFront(Iterator itr);
	void translate(Vec2 delta);
};


//テキストのうちある範囲（窓内）におけるグリフ配置と内容編集を担い、整合性を保つ
//実際に窓内のテキストを表示するには窓の範囲に応じた描画コストが最大毎フレームかかるため、それと比べてネックにならないコストで働けばいい
class TextWindow : GlyphArrangement {
public:
	void rejectFront(Iterator itr);
	void translate(Vec2 delta);
	void insertText(Iterator itr, String s);
	void eraseText(Iterator first, Iterator last);
};


//TODO: startまで空でもよくないか
class Animation {
public:

	enum class State {
		Inactive,
		Animating,
		Stable,
	};

private:

	Stopwatch _sw;

	double _animationTime, _progress;

	State _state;

public:

	Animation(
		std::vector<Vec2> &sourcePos,
		std::vector<Vec2> &targetPos,
		double animationTime)
	: _animationTime(animationTime)
	, _sw()
	, _progress(0)
	, _state(State::Inactive) {
	}

	State getState() { return _state; }

	double getProgress() { return _progress; }

	void start() {
		if (_state != State::Inactive) throw "not inactive";
		_sw.restart();
		_state = State::Animating;
	}

	void update() {
		if (_state != State::Animating) return;
		_progress = _sw.sF() / _animationTime;
		if (_progress > 1) {
			_progress = 1;
			_sw.reset();
			_state = State::Stable;
		}
	}
};

class FloatingText {
public:

	enum class State {
		Inactive,
		AnimatingIn,
		AnimatingOut,
		Stable
	};

private:

	const RectF _area;

	const std::vector<SP<const Font::Glyph>> _glyphs;

	double _lineInterval;

	Vec2 _start, _end;

	std::vector<Vec2> _startGlyphPos, _endGlyphPos;

	Stopwatch _sw;

	State _state;

	const double _animationMSIn = 700, _animationMSOut = 1000;

	
	Vec2 getStartGlyphPos(int index) {
		if (_startGlyphPos.size() > index) return _startGlyphPos[index];
		Vec2 ret = getStartGlyphPos(index - 1) + _glyphs[index - 1]->getAdvance();
		if (ret.y + _glyphs[index - 1]->getAdvance().y > _area.y + _area.h)
			ret = Vec2(ret.x - _lineInterval, _area.y);
		_startGlyphPos.push_back(ret);
		return ret;
	}

	Vec2 getEndGlyphPos(int index) {
		if (_endGlyphPos.size() > index) return _endGlyphPos[index];
		Vec2 ret = getEndGlyphPos(index - 1) + _glyphs[index - 1]->getAdvance();
		if (ret.y + _glyphs[index - 1]->getAdvance().y > _area.y + _area.h)
			ret = Vec2(ret.x - _lineInterval, _area.y);
		_endGlyphPos.push_back(ret);
		return ret;
	}

	void drawAnimationIn(double ms) {
		double t = ms/_animationMSIn;
		Vec2 delta = EaseOut(Easing::Back, _start, _end, t) - _end;
		for (int i = 0; i < (int)_glyphs.size(); i++) {

			Vec2 pos = getEndGlyphPos(i) + delta;
			if (_area.x + _area.w + _lineInterval < pos.x) continue;
			if (pos.x < _area.x) break;
			_glyphs[i]->draw(pos);
		}
	}

	void drawAnimationOut(double ms) {
		for (int i = 0; i < (int)_glyphs.size(); i++) {
			double animationMS = EaseOut(Easing::Expo, _animationMSOut/3.0, _animationMSOut, std::min(i, 200)/200.0);
			double t = std::min(1.0, ms/animationMS);

			Vec2 s = getStartGlyphPos(i), e = getEndGlyphPos(i);
			if (_area.x + _area.w + _lineInterval < s.x) continue;
			if (e.x < _area.x) break;
			double d1 = s.y - _area.y;
			double d2 = _area.h;
			double d3 = _area.y + _area.h - e.y;
			double delta = EaseOut(Easing::Quint, t)*(d1 + d2 + d3);
			Vec2 pos = [&](){
				if (delta <= d1) return Vec2(s.x, s.y - delta);
				delta -= d1;
				if (delta <= d2) return Vec2((s.x + e.x)/2.0, _area.y + _area.h - delta);
				delta -= d2;
				return Vec2(e.x, _area.y + _area.h - delta);
			}();
			_glyphs[i]->draw(pos);
		}
	}

public:

	FloatingText(const RectF &area, const std::vector<SP<const Font::Glyph>> &glyphs, double lineInterval, Vec2 pos)
	: _area(area)
	, _glyphs(glyphs)
	, _lineInterval(lineInterval)
	, _start(pos)
	, _end()
	, _startGlyphPos(0)
	, _endGlyphPos(1)
	, _sw()
	, _state(State::Inactive) {
		_startGlyphPos.reserve(_glyphs.size());
		_endGlyphPos.reserve(_glyphs.size());
	}

	State getState() { return _state; }

	void transitIn(double deltaX) {
		if (_state != State::Inactive) throw "not inactive";
		_state = State::AnimatingIn;
		_end = Vec2(_start.x + deltaX, _start.y);
		_startGlyphPos.clear();
		_endGlyphPos.assign(1, _end);
		_sw.restart();
	}

	void transitOut(Vec2 end) {
		if (_state != State::Stable) throw "not stable";
		_state = State::AnimatingOut;
		_end = end;
		_endGlyphPos.assign(1, _end);
		_sw.restart();
	}

	void update() {
		double ms = _sw.ms();
		
		if (_state == State::Inactive) return;
		if (_state == State::AnimatingIn) drawAnimationIn(ms);
		else if (_state == State::AnimatingOut) drawAnimationOut(ms);
		else {
			for (int i = 0; i < (int)_glyphs.size(); i++)
				_glyphs[i]->draw(getStartGlyphPos(i));
		}

		if (_state == State::AnimatingIn && ms > _animationMSIn) {
			_sw.reset();
			_state = State::Stable;
			std::swap(_startGlyphPos, _endGlyphPos);
			_start = _end;
		}
		else if (_state == State::AnimatingOut && ms > _animationMSOut) {
			_sw.reset();
			_state = State::Inactive;
			_startGlyphPos.clear();
			_endGlyphPos.clear();
			_start = _end;
		}
	}
};


class WholeView {
private:

	RectF _area;
	SP<Font::FixedFont> _font;
	GlyphArrangement _floatingArrangement;
	TextWindow _textWindow;
	double _lineInterval;
	JudgeUnsettled _ju;
	SP<Animation> _floatingAnimation;

	Vec2 floatingTextIn(Vec2 source, Vec2 target, double t, int i);
	Vec2 floatingTextOut(Vec2 source, Vec2 target, double t, int i);

public:

	//font: verticalでなければならない(これ設計汚くない？)
	WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font);

	void setText(const String &text);
	void update();

};


}
}