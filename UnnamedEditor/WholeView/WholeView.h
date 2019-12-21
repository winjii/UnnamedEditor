#pragma once
#include "Font\FixedFont.h"
#include "Text.h"
#include "Geometry.h"
#include "TextGeometry.h"
#include <list>
#include <cmath>


namespace UnnamedEditor {
namespace WholeView {

namespace G = Geometry;
namespace TG = TextGeometry;


//未確定文字列入力モードかどうかを判定するクラス
//未確定文字列を削除した時に、既にその削除のために使われた入力が、確定文字列の削除をやってしまう（ラグのせい）ことへの対処ができる
//（確定文字列への編集を一定時間ブロックするだけ）
//TODO: 未確定文字はunsettledじゃなくeditingで名前統一
class JudgeUnsettled {
private:
	int _count;
public:
	JudgeUnsettled() : _count(0) {}
	bool isUnsettled() { return _count > 0; }
	bool isSettled() { return _count == 0; }

	//1フレーム一回
	//unsettledStrSize: 現在の未確定文字列の長さ
	void update(int unsettledStrLength) {
		if (unsettledStrLength == 0) _count = std::max(_count - 1, 0);
		else _count = 5;
	}
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
	AnimationProgress() : _animationTime(), _sw(), _progress(0), _step(Step::Inactive) {}
	Step getStep() const { return _step; }
	bool isAnimating() const { return _step == Step::Animating; }
	bool isStable() const { return _step == Step::Stable; }
	double getProgress() const { return _progress; }
	void start(double animationTime) {
		_animationTime = animationTime;
		_progress = 0;
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


//自力で消滅して欲しいデータ
class TemporaryData {
public:
	//殺されたくないうちはtrueを差し出す
	//1フレーム1回
	virtual bool update() = 0;
	
	virtual void destroy() = 0;
	virtual bool isEmpty() = 0;

	static bool IsEmpty(SP<TemporaryData> p) {
		return !p || p->isEmpty();
	}

	class Manager { //Pointerを管理して自動で無効にする
	private:
		std::list<SP<TemporaryData>> _data;
	public:
		void update() { //1フレーム1回
			auto itr = _data.begin();
			while (itr != _data.end()) {
				if (!(*itr)->update()) {
					(*itr)->destroy();
					itr = _data.erase(itr);
				}
				else ++itr;
			}
		}
		void registerPointer(SP<TemporaryData> p) {
			_data.push_back(p);
		}
	};
};


class MinimapHighlight : public TemporaryData {
public:
	enum class Step { In, Out };
private:
	class Inner {
		friend MinimapHighlight;
		Texture _texture;
		RectF _region;
		Vec2 _pos;
		Step _step;
		Step _nextStep;
		Stopwatch _sw;
		double _scale;
		ColorF _color;
		bool update() {
			double maxTime = 0.35;
			double r = _sw.sF() / maxTime;
			if (r > 1) {
				r = 1;
				_sw.pause();
			}
			double t = (_step == Step::In) ? r : 1 - r;
			if (_step == Step::Out && _sw.isPaused()) return false;
			double delta = 0.05;
			_scale = 1 + delta * EaseOut(Easing::Expo, t);
			_color = EaseOut(Easing::Linear, Palette::Black, Palette::Orangered, t);
			if (_step != _nextStep) {
				_sw.pause();
				_sw.set((SecondsF)(maxTime - _sw.sF()));
				_sw.start();
			}
			_step = _nextStep;
			_nextStep = Step::Out;

			RectF(_pos, _region.w, _region.h).draw(Palette::White);
			Vec2 c = Vec2(_pos.x + _region.w / 2.0, _pos.y + _region.h / 2.0);
			_texture(_region).scaled(_scale).draw(Arg::center(c), Palette::Red);
			return true;
		}
		void keep() {
			_nextStep = Step::In;
		}
	};
	UP<Inner> _inner;
public:
	MinimapHighlight(Texture texture, RectF region, Vec2 pos) {
		_inner.reset(new Inner());
		_inner->_texture = texture;
		_inner->_region = region;
		_inner->_pos = pos;
		_inner->_step = Step::In;
		_inner->_nextStep = Step::In;
		_inner->_sw.start();
	}
	virtual bool update() { return _inner->update(); }
	virtual void destroy() { _inner.reset(); }
	virtual bool isEmpty() { return _inner == nullptr; }
	void keep() { _inner->keep(); }
};


class CharAnimation : public TemporaryData {
public:
	using Drawer = std::function<void(Vec2, SP<const Font::Glyph>, double)>;
private:
	class Inner {
		friend CharAnimation;
		AnimationProgress _ap;
		Drawer _drawer;
		bool update() {
			_ap.update();
			if (_ap.isStable()) return false;
			return true;
		}
		void draw(Vec2 pos, SP<const Font::Glyph> glyph) {
			_drawer(pos, glyph, _ap.getProgress());
		}
	};
	UP<Inner> _inner;
public:
	CharAnimation(Drawer drawer, double animationTime) {
		_inner.reset(new Inner());
		_inner->_drawer = drawer;
		_inner->_ap.start(animationTime);
	}
	virtual bool update() { return _inner->update(); }
	virtual void destroy() { _inner.reset(); }
	virtual bool isEmpty() { return _inner == nullptr; }
	void draw(Vec2 pos, SP<const Font::Glyph> glyph) { _inner->draw(pos, glyph); }
};


//TODO: あるlineからn（1000とか10000とか）文字以下のlineまでのミニマップをキャッシュ（バケット法）
//↑line1つのミニマップもキャッシュ
//↑ミニマップ描画をshortcutできるイメージ
//↑文字列編集されたときはどうする？
//↑あるバケットが更新されたらその一つ手前のバケットから再計算→統合できるのにされていない隣接バケットが生まれない→例えばn=1000だとしたら、最悪ケースでも500と501や1000と1のバケットが交互に並ぶだけ→バケットの最大個数は2√nくらいで抑えられる
//テキストの変更をまたいで使うイテレータはちゃんとregisterItr()する。ただしこれらのイテレータを含む範囲を削除してはいけない...
//TODO: 末尾の改行消さないようにする
//TODO: cursor持つのこいつか？
//TODO: 管理されたイテレータは自分でremoveItrを呼び出すようにすればいいのでは
class GlyphArrangement2 {
public:
	struct LineData;
	struct CharData;
	using LineIterator = std::list<LineData>::iterator;
	using CharIterator = std::pair<LineIterator, std::vector<CharData>::iterator>;
	struct CharData {
		char16_t code;
		SP<const Font::Glyph> glyph;
		SP<const Font::Glyph> blurred;
		std::list<SP<CharIterator>> itrs;
		TG::PointOnText pos; //line内での相対的な位置
		SP<CharAnimation> animation;
	};
	struct BucketHeader {
		int wrapCount;
		MSRenderTexture minimap;
		int advance;
	};
	struct LineData {
		std::vector<CharData> cd; //テキスト末尾に改行
		int wrapCount;
		SP<BucketHeader> bucketHeader;
		SP<MinimapHighlight> highlight;
	};
private:
	SP<Font::FixedFont> _font;
	SP<Font::FixedFont> _blurredFont;
	std::list<LineData> _data;
	int _lineInterval;
	int _maxLineLnegth;
	TG::Direction _textDir;

	LineIterator _origin;
	TG::PointOnText _originPos;
	SP<CharIterator> _cursor;
	SP<MSRenderTexture> _bufferTexture0;
	SP<RenderTexture> _bufferTexture1;
	const double _minimapFontSize = 0.8;

	//glyph, posを計算する。cd.empty()だったら削除
	LineIterator initLine(LineIterator litr);

	void initBucket(LineIterator first, LineIterator last);
public:
	GlyphArrangement2(SP<Font::FixedFont> font, int lineInterval, int maxLineLength, TG::Direction textDir);

	SP<Font::FixedFont> font();
	void registerItr(SP<CharIterator> itr);
	void removeItr(SP<CharIterator> itr);
	LineIterator tryNext(LineIterator litr, int cnt = 1) const;
	LineIterator tryPrev(LineIterator litr, int cnt = 1) const;
	CharIterator insertText(CharIterator citr, const String &s);
	CharIterator eraseText(CharIterator first, CharIterator last); //管理されたイテレータはlastにどかす
	CharIterator replaceText(CharIterator first, CharIterator last, const String& s);
	void scroll(int delta);
	CharIterator next(CharIterator citr, bool overLine = false, int cnt = 1) const;
	CharIterator prev(CharIterator citr, bool overLine = false, int cnt = 1) const;
	void next(SP<CharIterator> citr); //管理されたイテレータを動かす
	void prev(SP<CharIterator> citr); //管理されたイテレータを動かす
	LineIterator origin() const;
	TG::PointOnText originPos() const;
	SP<CharIterator> cursor() const;
	LineIterator begin();
	LineIterator end();
	CharIterator lineBegin(LineIterator litr) const;
	CharIterator lineEnd(LineIterator litr) const;
	LineIterator bucket(LineIterator litr) const;
	LineIterator nextBucket(LineIterator litr) const;
	double minimapLineInterval() const;
	double minimapScale() const;
	void resetOrigin(LineIterator origin, TG::PointOnText pos);
	TG::Direction textDirection() const;

	//NULL文字を挿入すると番兵などに使える
	//NULL文字はremoveItr時に参照が切れたら自動で削除される
	SP<CharIterator> makeNull(CharIterator citr);
};


class FloatingAnimation {
	using GA = GlyphArrangement2;
public:
	enum class Step {
		Inactive, Floating, Stopping
	};
private:
	Step _step;
	int _lineInterval;
	int _maxLineLength;
	SP<GA::CharIterator> _floatingBegin; //管理されたイテレータ
	AnimationProgress _inOutAP;
	AnimationProgress _updateAP;
	std::vector<TG::Vec2OnText> _oldHeadPos;
	std::vector<TG::Vec2OnText> _newHeadPos;
	double _oldAdvance; //headのadvance
	double _newAdvance;

	TG::Vec2OnText easeOverLine(TG::Vec2OnText source, TG::Vec2OnText target, double t, int i);
public:
	FloatingAnimation(int lineInterval, int maxLineLength);
	Step step() const;
	bool isInactive() const;
	bool isFloating() const;
	GA::CharIterator floatingBegin() const;
	bool isStable() const; //文字が整数座標に静止しているか
	void startFloating(GA& ga, GA::CharIterator floatingBegin);
	void stopFloating();
	void updateTime();
	void updatePos(const GA& ga); //変更が起きたときのみ呼び出す

	//citrはfloatingStart以降であること
	//Inactiveのときに呼び出さない
	//各行の行頭の静止座標からの相対位置を返す（描画する側で行頭の静止座標を足す）
	TG::Vec2OnText getPos(GA::CharIterator citr);
};


//class EditingCursor {
//	using GA = GlyphArrangement2;
//private:
//	
//public:
//	GA::CharIterator pos() const;
//	Vec2OnText drawingPos();
//	void move()
//};


class CleanCopyCursor {
	using GA = GlyphArrangement2;
public:
	enum class Step {
		Advancing, Retreating, Stable
	};
private:
	Step _step;
	std::pair<SP<GA::CharIterator>, double> _drawingPos;
	Stopwatch _sw;
	SP<GA> _ga;
public:
	CleanCopyCursor(SP<GA> ga, GA::CharIterator citr);
	GA::CharIterator pos() const;
	std::pair<GA::CharIterator, double> drawingPos() const;
	void changeItr(GA::CharIterator newItr);
	bool isStable();
	void startAdvancing();
	void startRetreating();
	void stop();
	void update(TemporaryData::Manager& tmpData);
	void registerPaint(TemporaryData::Manager& tmpData, GA::CharIterator citr);
	void registerUnpaint(TemporaryData::Manager& tmpData, GA::CharIterator citr);
};


//入力状態のときに働く機能を取りまとめる
class InputManager {
	using GA = GlyphArrangement2;
private:
	SP<FloatingAnimation> _fa;
	String _editing;
	bool _isInputing;
	SP<GA::CharIterator> _cursor;
	SP<CleanCopyCursor> _cccursor;
public:
	InputManager(int lineInterval, int maxLineLength);
	bool isInputing() const;
	SP<FloatingAnimation> floatingAnimation() const;
	SP<CleanCopyCursor> cleanCopyCursor() const;
	void update(SP<GA> ga, String addend, String editing);
	void stopInputting();
	void startInputting(SP<GA> ga);
	void deleteLightChar(SP<GA> ga);
	void startAdvancingCCC();
	void startRetreatingCCC();
	void stopCCC();
};


class ScrollDelta {
public:
	enum class Step { NotScrolling, Scrolling, StoppingScroll };
private:
	int _direction;
	int _used;
	Step _step;
	Stopwatch _sw;
	const int _lineInterval;
public:
	ScrollDelta(int lineInterval) : _step(Step::NotScrolling), _lineInterval(lineInterval) {}
	Step step() { return _step; }
	int direction() { return _direction; }
	void startScroll(int direction) { //-1 or +1
		_used = 0;
		if (_step != Step::NotScrolling) {
			_used = (_used - std::round(_used / _lineInterval)*_lineInterval)*(_direction*direction);
		}
		_direction = direction;
		_sw.restart();
		_step = Step::Scrolling;
	}
	void stopScroll() {
		if (_step != Step::Scrolling) return;
		_step = Step::StoppingScroll;
	}
	std::pair<int, double> useDelta() {
		//y = log(x + a) + log(1 / a)の積分
		auto f = [](double x) {
			double a = 0.1;
			return -x*std::log(a) + (x + a)*std::log(x + a) - x - a*std::log(a);
		};
		double t = _sw.sF();
		double sum = 1000*t;
		if (_step == Step::StoppingScroll) {
			sum = std::round(_used / _lineInterval) * _lineInterval;
			_step = Step::NotScrolling;
		}
		//int p = 20*std::floor(sum/20);
		//sum = EaseOut(Easing::Expo, p, p + 20, (sum - p)/20);
		int ret = (int)sum - _used;
		_used = (int)sum;
		return { ret * _direction, sum - (double)(int)sum };
	}
};


class WholeView {
private:
	RectF _area;
	TG::TextArea _textArea;
	int _lineInterval;
	TG::Direction _textDir;
	SP<Font::FixedFont> _font;
	SP<GlyphArrangement2> _ga;
	JudgeUnsettled _ju;
	ScrollDelta _scrollDelta;
	InputManager _inputManager;
	RenderTexture _masker;
	RenderTexture _maskee;
	MSRenderTexture _foreground;
	const PixelShader _maskPS;
	TemporaryData::Manager _tmpData;

public:

	WholeView(Rect area, SP<Font::FixedFont> font, TG::Direction textDir);

	void setText(const String &text);
	void draw();
	void minimapTest();
	SP<GlyphArrangement2> glyphArrangement() const;
	void jump(GlyphArrangement2::LineIterator litr);
};


class MinimapView {
	using GA = GlyphArrangement2;
private:
	RectF _area;
	SP<GA> _ga;
	TG::Direction _mapDir;
	TG::TextArea _body;
	TemporaryData::Manager _tmpManager;
public:
	MinimapView(RectF area, SP<GA> ga);
	GA::LineIterator draw();
};


}
}