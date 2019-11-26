#pragma once
#include "Font\FixedFont.h"
#include <list>


namespace UnnamedEditor {
namespace WholeView {


using DevicePos = Vec2;
const char16_t ZERO_WIDTH_SPACE = U'\u200B';
const char16_t NEWLINE = U'\n';


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

//末尾文字の削除を避けるのはどこの役割か

struct CharData {
	char16_t code;
	SP<const Font::Glyph> glyph;
};
class Text {
public:
	using Iterator = std::list<CharData>::const_iterator;
private:
	std::list<CharData> _data;
	SP<Font::FixedFont> _font;
public:
	Text(SP<Font::FixedFont> font);

	Iterator begin() const;
	Iterator end() const;
	Iterator next(Iterator itr) const;
	Iterator prev(Iterator itr) const;
	Iterator insert(Iterator itr, String s);
	Iterator erase(Iterator first, Iterator last);
	bool isNewline(Iterator itr) const;
	std::pair<Iterator, int> lineHead(Iterator itr) const; //[先頭文字のIterator, 先頭文字までの距離]
	std::pair<Iterator, int> nextLineHead(Iterator itr) const; //改行が見つからなければend()までの距離になる
};


//テキストを表示するためのグリフ配置を計算する
//テキストの更新はできず、テキストが削除された部分のText::Iteratorは壊れる（更新に対応するにはTextWindowを使う）
//外部で勝手にiteratorを動かしたり参照先に書き込んだりしちゃいけない
//TODO: ↑本来は型で制限すべき←このクラスを介さないとアクセスできない独自イテレータを定義
//見かけ上の窓（_beginから_endまで）はXXXExtendedによってしか広がらない（内部で勝手に伸縮しない）
//ある文字の位置を決めるのに絶対にその前の改行までは遡らなければいけないから折り返し文字量に応じて計算量が増えるのは仕方ない
class GlyphArrangement {
public:
	using Iterator = std::pair<Text::Iterator, std::list<Vec2>::iterator>;
private:
	SP<const Text> _text;
	RectF _area;
	double _lineInterval;
protected:
	std::list<Vec2> _pos; //位置キャッシュ。要素が増えても減ることはない（イテレータ壊れない）
	Iterator _begin, _end;
	Iterator _drawStart;
	Text::Iterator _beginConstraints, _endConstraints;

	Iterator next(Iterator itr);
	Iterator prev(Iterator itr);
	Iterator advanced(Iterator itr, int d);
	void arrange(Iterator first, Iterator last, Vec2 origin);

	//適宜_posを拡張するがグリフ位置の計算まではしない
	std::pair<Iterator, int> lineHead(Iterator itr);
	std::pair<Iterator, int> nextLineHead(Iterator itr);

	//beginConstraintsやendConstraintsを超えてイテレータを返せる（超えても編集しない分には問題ない）
	Iterator prevForce(Iterator itr);
	Iterator nextForce(Iterator itr);
public:
	GlyphArrangement(SP<const Text> text, const RectF& area, double lineInterval, Vec2 originPos);
	GlyphArrangement(GlyphArrangement& ga, Iterator beginConstraints);
	GlyphArrangement(GlyphArrangement&&) = default;
	GlyphArrangement& operator=(GlyphArrangement&&) = default;
	Text::Iterator beginConstraints() const;
	Text::Iterator endConstraints() const;
	RectF area() const;
	double lineInterval() const;
	bool onTextArea(Iterator itr) const; //描画エリア内に被る可能性があればon
	bool upperTextArea(Iterator itr) const;
	bool lowerTextArea(Iterator itr) const;
	void scroll(double delta); //内部で持ってる要素分時間かかる
	void disable();
	Iterator drawStart(); //描画開始位置

	//XXXExtended:テキストをはみ出さない限りは窓を拡張して有効なIteratorを返すことを保証する
	//（Iterator::firstがend()でないのにIterator::secondがend()であるようなイテレータを返すことはない）
	Iterator prevExtended(Iterator itr);
	Iterator nextExtended(Iterator itr);
	Iterator prevExtended(Iterator itr, int cnt);
	Iterator nextExtended(Iterator itr, int cnt);
};


//グリフ配置と内容編集を担い、整合性を保つ
//遅延評価的に働きなるべく必要最小限の労力で配置を計算する
//実際にテキストを表示するには描画範囲に応じた描画コストが最大毎フレームかかるため、それと比べてネックにならないコストで働けばいい
class TextWindow : public GlyphArrangement {
private:
	SP<Text> _text;
	Iterator _cursor; //視覚的なカーソルというより編集位置と思ったほうがよさそう
	int _editedCount;
	int _unsettledCount;
	bool _isEditing;
	
	//destroyed: destroyed以降が破壊されることを示す
	//edit: 文字列を編集しdestroyedに代わるイテレータを返す
	Iterator editText(Iterator destroyed, std::function<Iterator()> edit);
public:
	TextWindow(SP<Text> text, const RectF& area, double lineInterval, Vec2 originPos);
	Iterator insertText(Iterator itr, const String &s);
	Iterator eraseText(Iterator first, Iterator last);
	SP<const Text> text();
	UP<GlyphArrangement> detachBack(Iterator partition);
	void undetatch(UP<GlyphArrangement> ga);
	bool setCursor(Iterator cursor); //編集中はカーソルを勝手に動かせない。falseを返す
	void startEditing();
	void stopEditing();
	bool isEditing();
	Iterator cursor();
	Iterator editedBegin();
	Iterator unsettledBegin();
	void inputText(const String& addend, const String& editing); //編集中でなければ何もしない
	void eraseUnsettled();
};


//TODO: startまで空でもよくないか
class AnimationProgress {
public:

	enum class Step {
		Inactive,
		Animating,
		Stable,
	};

private:

	Stopwatch _sw;

	double _animationTime, _progress;

	Step _step;

public:

	AnimationProgress()
	: _animationTime()
	, _sw()
	, _progress(0)
	, _step(Step::Inactive) {
	}

	Step getStep() { return _step; }

	double getProgress() { return _progress; }

	void start(double animationTime) {
		if (_step != Step::Inactive) throw "not inactive";
		_animationTime = animationTime;
		_sw.restart();
		_step = Step::Animating;
	}

	void update() {
		if (_step != Step::Animating) return;
		_progress = _sw.sF() / _animationTime;
		if (_progress > 1) {
			_progress = 1;
			_sw.reset();
			_step = Step::Stable;
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

	enum class FloatingStep {
		Inactive, AnimatingIn, Stable, AnimatingOut
	};

	RectF _area;
	double _lineInterval;
	SP<Font::FixedFont> _font;
	TextWindow _textWindow;
	UP<GlyphArrangement> _floatingArrangement;
	JudgeUnsettled _ju;
	FloatingStep _floatingStep;
	AnimationProgress _floatingProgress;
	SP<const Text> _text;

	Vec2 floatingTextIn(Vec2 source, Vec2 target, double t, int i);
	Vec2 floatingTextOut(Vec2 source, Vec2 target, double t, int i);

public:

	//font: verticalでなければならない(これ設計汚くない？)
	WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font);

	void setText(const String &text);
	void draw();
};


}
}