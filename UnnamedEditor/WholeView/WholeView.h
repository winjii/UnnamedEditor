#pragma once
#include "Font\FixedFont.h"
#include "Text.h"
#include "Geometry.h"
#include "TextGeometry.h"
#include <list>
#include <cmath>
#include <optional>
#include <limits>


namespace UnnamedEditor {
namespace WholeView {

namespace G = Geometry;
namespace TG = TextGeometry;


//未確定文字列入力モードかどうかを判定するクラス
//未確定文字列を削除した時に、既にその削除のために使われた入力が、確定文字列の削除をやってしまう（ラグのせい）ことへの対処ができる
//（確定文字列への編集を一定時間ブロックするだけ）
class JudgeEditing {
private:
	int _count;
public:
	JudgeEditing() : _count(0) {}
	bool isNotEditing() { return _count > 0; }
	bool isEditing() { return _count == 0; }

	//1フレーム一回
	//editLength: 現在の未確定文字列の長さ
	void update(int editLength) {
		if (editLength == 0) _count = std::max(_count - 1, 0);
		else _count = 5;
	}
};


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
	AnimationProgress(double progress) : _animationTime(), _sw(), _progress(progress)
		, _step(Step::Inactive) { }
	AnimationProgress() : AnimationProgress(0) {}
	Step getStep() const { return _step; }
	bool isAnimating() const { return _step == Step::Animating; }
	bool isStable() const { return _step == Step::Stable; }
	double getProgress() const { return _progress; }
	void start(double animationTime) {
		if (animationTime == 0) {
			_progress = 1;
			_step = Step::Stable;
			_sw.reset();
			return;
		}
		_animationTime = animationTime;
		_progress = 0;
		_sw.restart();
		_step = Step::Animating;
	}
	void update() {
		if (_step != Step::Animating) return;
		if (_sw.sF() >= _animationTime) {
			_progress = 1;
			_sw.reset();
			_step = Step::Stable;
		}
		else _progress = _sw.sF() / _animationTime;
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
				_sw.set((SecondsF)(maxTime - std::max(_sw.sF(), maxTime)));
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

//TODO: 末尾の改行消さないようにする
//TODO: cursor持つのこいつか？
//TODO: 管理されたイテレータは自分でremoveItrを呼び出すようにすればいいのでは

//グリフ位置を計算&キャッシュする。ミニマップ(縮小表示版)も計算&キャッシュしておく
class GlyphArrangement2 {

	//ミニマップについて:
	//あるsectionからn（1000とか10000とか）文字以下のsectionまでのミニマップをキャッシュ（バケット法）
	//↑section1つのミニマップもキャッシュ
	//↑ミニマップ描画をshortcutできるイメージ
	//↑文字列編集されたときはどうする？
	//↑あるバケットが更新されたらその一つ手前のバケットから再計算→統合できるのにされていない隣接バケットが生まれない→例えばn=1000だとしたら、最悪ケースでも500と501や1000と1のバケットが交互に並ぶだけ→バケットの最大個数は2√nくらいで抑えられる

public:
	struct SectionData;
	struct CharData;
	using SectionIterator = std::list<SectionData>::iterator;
	using CharIterator = std::pair<SectionIterator, std::vector<CharData>::iterator>;
	class ManagedIterator;
	friend ManagedIterator;
	class ManagedIterator {
	private:
		GlyphArrangement2& _ga;
		SP<CharIterator> _itr;
	public:
		ManagedIterator(GlyphArrangement2& ga, CharIterator itr)
			: _ga(ga), _itr(new CharIterator(itr)) {
			_ga.registerItr(_itr);
		}
		ManagedIterator(const ManagedIterator& itr) = default;
		~ManagedIterator() {
			if (_itr.use_count() == 1) _ga.removeItr(_itr);
		}
		void move(CharIterator citr) {
			_ga.moveItr(_itr, citr);
		}
		void next(int cnt) {
			CharIterator newPos = *_itr;
			for (int i = 0; i < cnt; i++) {
				auto nxt = _ga.next(newPos, true);
				if (nxt.first == _ga.end()) return;
				newPos = nxt;
			}
			move(newPos);
		}
		void prev(int cnt) {
			CharIterator newPos = *_itr;
			for (int i = 0; i < cnt; i++) {
				if (newPos == _ga.sectionBegin(_ga.begin())) return;
				newPos = _ga.prev(newPos, true);
			}
			move(newPos);
		}
		void next() { next(1); }
		void prev() { prev(1); }
		const CharIterator* operator->() const { return _itr.get(); }
		const CharIterator& operator*() const { return *_itr; }
	};
	struct CharData {
		char16_t code;
		SP<const Font::Glyph> glyph;
		SP<const Font::Glyph> blurred;
		std::list<SP<CharIterator>> itrs;
		TG::PointOnText pos; //section内での相対的な位置
		SP<CharAnimation> animation;
	};
	struct BucketHeader {
		int wrapCount;
		MSRenderTexture minimap;
		int advance;
	};
	struct SectionData {
		std::vector<CharData> cd; //テキスト末尾に改行
		int wrapCount;
		SP<BucketHeader> bucketHeader;
		SP<MinimapHighlight> highlight;
	};
private:
	SP<Font::FixedFont> _font;
	SP<Font::FixedFont> _blurredFont;
	std::list<SectionData> _data;
	int _lineInterval;
	int _maxLineLnegth;
	TG::Direction _textDir;

	SectionIterator _origin;
	TG::PointOnText _originPos;
	SP<MSRenderTexture> _bufferTexture0;
	SP<RenderTexture> _bufferTexture1;
	const double _minimapFontSize = 0.8;

	//glyph, posを計算する。cd.empty()だったら削除
	SectionIterator initSection(SectionIterator sitr);

	void initBucket(SectionIterator first, SectionIterator last);
	void registerItr(SP<CharIterator> itr);
	void moveItr(SP<CharIterator> itr, CharIterator citr);
	void removeItr(SP<CharIterator> itr);
public:
	GlyphArrangement2(SP<Font::FixedFont> font, int lineInterval, int maxLineLength, TG::Direction textDir);

	SP<Font::FixedFont> font();
	ManagedIterator registerItr(CharIterator citr);
	SectionIterator tryNext(SectionIterator sitr, int cnt = 1) const;
	SectionIterator tryPrev(SectionIterator sitr, int cnt = 1) const;
	CharIterator insertText(CharIterator citr, const String &s);
	CharIterator eraseText(CharIterator first, CharIterator last); //管理されたイテレータはlastにどかす
	CharIterator replaceText(CharIterator first, CharIterator last, const String& s);
	void scroll(int delta);
	CharIterator next(CharIterator citr, bool overSection = false, int cnt = 1) const;
	CharIterator prev(CharIterator citr, bool overSection = false, int cnt = 1) const;
	void next(SP<CharIterator> itr); //管理されたイテレータを動かす
	void prev(SP<CharIterator> itr); //管理されたイテレータを動かす
	SectionIterator origin() const;
	TG::PointOnText originPos() const;
	SectionIterator begin();
	SectionIterator end();
	CharIterator sectionBegin(SectionIterator sitr) const;
	CharIterator sectionEnd(SectionIterator sitr) const;
	SectionIterator bucket(SectionIterator sitr) const;
	SectionIterator nextBucket(SectionIterator sitr) const;
	double minimapLineInterval() const;
	double minimapScale() const;
	void resetOrigin(SectionIterator origin, TG::PointOnText pos);
	TG::Direction textDirection() const;
	CharIterator lineHead(CharIterator citr) const;
	CharIterator nextLineHead(CharIterator citr) const;

	//NULL文字を挿入すると番兵などに使える
	//NULL文字は参照が切れたら自動で削除される
	ManagedIterator makeNull(CharIterator citr);
};


class FloatingAnimation {
	using GA = GlyphArrangement2;
public:
	enum class Step {
		Inactive, Floating, Stopping
	};
private:
	SP<GA> _ga;
	Step _step;
	int _lineInterval;
	int _maxLineLength;
	std::optional<GA::ManagedIterator> _floatingBegin;
	std::vector<TG::Vec2OnText> _headPos;
	std::vector<double> _headVelocity;
	AnimationProgress _inOutAP;
	int _oldAdvance;
	double _tailPrp;
	double _tailVelocity;

	TG::Vec2OnText easeOverLine(TG::Vec2OnText current, TG::Vec2OnText target, double &velocity);
public:
	FloatingAnimation(SP<GA> ga, int lineInterval, int maxLineLength);
	Step step() const;
	bool isInactive() const;
	bool isFloating() const;
	GA::CharIterator floatingBegin() const; //Inactiveで呼び出さない
	//bool isStable() const; //文字が整数座標に静止しているか
	void startFloating(GA& ga, GA::CharIterator floatingBegin);
	void stopFloating();

	//GA::CharIterator progressOfCalc();

	////各行の行頭の静止座標からの相対位置を返す（描画する側で行頭の静止座標を足す）
	//TG::Vec2OnText resetCalc();
	//TG::Vec2OnText advanceCalc();

	void update();

	//citrはfloatingStart以降であること
	//Inactiveのときに呼び出さない
	//各行の行頭の静止座標からの相対位置を返す（描画する側で行頭の静止座標を足す）
	TG::Vec2OnText getPos(GA::CharIterator citr);
};


class EditCursor {
	using GA = GlyphArrangement2;
private:
	SP<GA> _ga;
	GA::ManagedIterator _pos;
	TG::PointOnText _oldPos;
	AnimationProgress _ap;
	double _virtualPosPrl;

	int gap(GA::CharIterator citr) {
		return std::abs(citr.second->pos.prl - _virtualPosPrl);
	}

	GA::CharIterator gapMin(GA::CharIterator first, GA::CharIterator last) {
		auto ret = first;
		int minGap = std::numeric_limits<int>::max();
		for (auto citr = first; citr != last; citr = _ga->next(citr, true)) {
			if (gap(citr) < minGap) {
				ret = citr;
				minGap = gap(citr);
			}
		}
		return ret;
	}

	void increasePrlImpl(int cnt) {
		_pos.next(cnt);
		_virtualPosPrl = _pos->second->pos.prl;
	}
	void decreasePrlImpl(int cnt) {
		_pos.prev(cnt);
		_virtualPosPrl = _pos->second->pos.prl;
	}
	void increasePrpImpl() {
		auto nlh = _ga->nextLineHead(*_pos);
		if (nlh.first == _ga->end()) {
			_pos.move(_ga->prev(nlh, true));
			return;
		}
		auto nnlh = _ga->nextLineHead(nlh);
		_pos.move(gapMin(nlh, nnlh));
	}
	void decreasePrpImpl() {
		auto lh = _ga->lineHead(*_pos);
		if (lh == _ga->sectionBegin(_ga->begin())) {
			_pos.move(lh);
			return;
		}
		auto plh = _ga->lineHead(_ga->prev(lh, true));
		_pos.move(gapMin(plh, lh));
	}
public:
	EditCursor(SP<GA> ga, GA::CharIterator pos)
		: _ga(ga)
		, _pos(_ga->registerItr(pos))
		, _oldPos(0, 0)
		, _virtualPosPrl(_pos->second->pos.prl)
		, _ap(1) { }
	GA::CharIterator pos() const {
		return *_pos;
	}
	TG::Vec2OnText drawingPos(TG::PointOnText sectionOrigin) {
		TG::Vec2OnText p = (_pos->second->pos + sectionOrigin).toTextVec2();
		TG::Vec2OnText q = _oldPos.toTextVec2();
		return (p - q) * EaseOut(Easing::Expo, _ap.getProgress()) + q;
	}
	void increasePrl(int cnt) {
		increasePrlImpl(cnt);
		_ap.start(0);
	}
	void decreasePrl(int cnt) {
		decreasePrlImpl(cnt);
		_ap.start(0);
	}
	void increasePrp() {
		increasePrpImpl();
		_ap.start(0);
	}
	void decreasePrp() {
		decreasePrpImpl();
		_ap.start(0);
	}
	void increasePrlAnimation(int cnt, TG::PointOnText sectionOrigin) {
		_oldPos = sectionOrigin + _pos->second->pos;
		increasePrlImpl(cnt);
		_ap.start(0.25);
	}
	void decreasePrlAnimation(int cnt, TG::PointOnText sectionOrigin) {
		_oldPos = sectionOrigin + _pos->second->pos;
		decreasePrlImpl(cnt);
		_ap.start(0.25);
	}
	void increasePrpAnimation(TG::PointOnText sectionOrigin) {
		_oldPos = sectionOrigin + _pos->second->pos;
		increasePrpImpl();
		_ap.start(0.25);
	}
	void decreasePrpAnimation(TG::PointOnText sectionOrigin) {
		_oldPos = sectionOrigin + _pos->second->pos;
		decreasePrpImpl();
		_ap.start(0.25);
	}
	void moveAnimation(TG::PointOnText deltaInChars, TG::PointOnText sectionOrigin) {
		_oldPos = sectionOrigin + _pos->second->pos;
		for (int i = 0; i < std::abs(deltaInChars.prp); i++) {
			if (deltaInChars.prp > 0) increasePrpImpl();
			else decreasePrpImpl();
		}
		if (deltaInChars.prl > 0) increasePrlImpl(deltaInChars.prl);
		else if (deltaInChars.prl < 0) decreasePrlImpl(-deltaInChars.prl);
		_ap.start(0.25);
	}
	void move(TG::PointOnText deltaInChars) {
		for (int i = 0; i < std::abs(deltaInChars.prp); i++) {
			if (deltaInChars.prp > 0) increasePrpImpl();
			else decreasePrpImpl();
		}
		if (deltaInChars.prl > 0) increasePrlImpl(deltaInChars.prl);
		else if (deltaInChars.prl < 0) decreasePrlImpl(-deltaInChars.prl);
		_ap.start(0);
	}
	void move(GA::CharIterator citr) {
		_pos.move(citr);
		_ap.start(0);
	}
	void update() {
		_ap.update();
	}
	void scroll(int delta) {
		_oldPos.prp += delta;
	}
};


class CleanCopyCursor {
	using GA = GlyphArrangement2;
public:
	enum class Step {
		Advancing, Retreating, Stable
	};
private:
	Step _step;
	std::pair<GA::ManagedIterator, double> _drawingPos;
	Stopwatch _sw;
	SP<GA> _ga;
	SP<EditCursor> _editCursor;
public:
	CleanCopyCursor(SP<GA> ga, SP<EditCursor> editCursor);
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
	SP<GA> _ga;
	SP<EditCursor> _cursor;
	SP<CleanCopyCursor> _cccursor;
public:
	InputManager(SP<GA> ga, SP<EditCursor> cursor, int lineInterval, int maxLineLength);
	bool isInputing() const;
	SP<FloatingAnimation> floatingAnimation() const;
	SP<CleanCopyCursor> cleanCopyCursor() const;
	void update(String addend, String editing);
	void stopInputting();
	void startInputting();
	void deleteLightChar();
	void startAdvancingCCC();
	void startRetreatingCCC();
	void stopCCC();
};


class ScrollDelta {
private:
	int _delta;
	int _used;
	AnimationProgress _ap;
	const int _lineInterval;
public:
	ScrollDelta(int lineInterval) : _lineInterval(lineInterval) {}
	bool isScrolling() { return _ap.isAnimating(); }
	int direction() { return _delta; }
	void scroll(int delta) {
		if (delta == 0) return;
		if (isScrolling()) {
			_used = _used - _lineInterval * _delta;
		}
		else _used = 0;
		_delta = delta;
		_ap.start(0.5);
	}
	std::pair<int, double> useDelta() {
		_ap.update();
		if (!isScrolling()) return { 0, 0.0 };
		double t = _ap.getProgress();
		double sum = _delta * _lineInterval * EaseOut(Easing::Quad, t);
		int ret = (int)sum - _used;
		_used = (int)sum;
		return { ret, sum - (double)(int)sum };
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
	JudgeEditing _ju;
	ScrollDelta _scrollDelta;
	SP<EditCursor> _cursor;
	InputManager _inputManager;
	RenderTexture _masker;
	RenderTexture _maskee;
	MSRenderTexture _foreground;
	const PixelShader _maskPS;
	TemporaryData::Manager _tmpData;
	
	std::pair<TG::Vec2OnText, TG::Vec2OnText> drawBody(const RenderTexture& maskee, const RenderTexture& foreground);
	void drawMasker(const RenderTexture& masker, TG::Vec2OnText maskStart, TG::Vec2OnText maskEnd);

public:

	WholeView(Rect area, SP<Font::FixedFont> font, TG::Direction textDir);

	void setText(const String &text);
	void draw();
	void minimapTest();
	SP<GlyphArrangement2> glyphArrangement() const;
	void jump(GlyphArrangement2::SectionIterator litr);
	SP<const EditCursor> cursor() const;
};


class MinimapView {
	using GA = GlyphArrangement2;
private:
	Rect _area;
	SP<GA> _ga;
	TG::Direction _mapDir;
	TG::TextArea _body;
	TemporaryData::Manager _tmpManager;
public:
	MinimapView(Rect area, SP<GA> ga);
	GA::SectionIterator draw(GA::SectionIterator editSection);
};


}
}