#pragma once
#include "Font\FixedFont.h"
#include "Text.h"
#include <list>
#include <cmath>


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
	bool isUnsettled() { return _count > 0; }
	bool isSettled() { return _count == 0; }

	//1フレーム一回
	//unsettledStrSize: 現在の未確定文字列の長さ
	void update(int unsettledStrLength) {
		if (unsettledStrLength == 0) _count = std::max(_count - 1, 0);
		else _count = 5;
	}
};



//テキストを表示するためのグリフ配置を計算する
//テキストの更新はできず、テキストが削除された部分のText::Iteratorは壊れる（更新に対応するにはTextWindowを使う）
//外部で勝手にiteratorを動かしたり参照先に書き込んだりしちゃいけない
//TODO: ↑本来は型で制限すべき←このクラスを介さないとアクセスできない独自イテレータを定義
//見かけ上の窓（_beginから_endまで）はXXXExtendedによってしか広がらない（内部で勝手に伸縮しない）
//ある文字の位置を決めるのに絶対にその前の改行までは遡らなければいけないから折り返し文字量に応じて計算量が増えるのは仕方ない
//文字でなく仕切りを指すイテレータはNULL文字と一緒に動く（文字を挿入などするとき仕切りの位置を動かさなくていい嬉しさがある）
//TODO; NULL文字を適宜消す
//TODO: beginとendの役割分かりづれーー
//TODO: 区間の扱いもうちょっとなんとかならんか
class GlyphArrangement {
public:
	using Iterator = std::pair<Text::Text::Iterator, std::list<Vec2>::iterator>;
private:
	SP<Text::Text> _text;
	RectF _area;
	double _lineInterval;
protected:
	std::list<Vec2> _pos; //グリフ位置キャッシュ。要素が増えても減ることはない（イテレータ壊れない）

	//NULL文字に対応するイテレータ群
	//普通の文字同様、_cacheBeginと_cacheEndの間にある限りはグリフ位置が計算される
	Iterator _cacheBegin, _cacheEnd; //(_cachBegin, _cacheEnd]はキャッシュ済みの区間（書き間違いではない）
	Text::Text::Iterator _begin, _end; //(_begin, _end]は外部からアクセスする範囲

	Iterator next(Iterator itr);
	Iterator prev(Iterator itr);
	Iterator advanced(Iterator itr, int d);
	void arrange(Iterator first, Iterator last); //firstのグリフ位置を基準にして(first, last]のグリフ位置を計算
	bool onTextArea(Iterator itr) const; //描画エリア内に被る可能性があればon
	bool upperTextArea(Iterator itr) const;
	bool lowerTextArea(Iterator itr) const;
	//Iterator eraseSafe(Iterator first, Iterator last); //NULL文字を避けて削除

	//適宜_posを拡張するがグリフ位置の計算まではしない
	std::pair<Iterator, int> lineHead(Iterator itr);
	std::pair<Iterator, int> nextLineHead(Iterator itr);
public:
	GlyphArrangement(SP<Text::Text> text, const RectF& area, double lineInterval, Vec2 originPos);
	GlyphArrangement(GlyphArrangement& ga, Iterator begin);
	~GlyphArrangement();
	GlyphArrangement(GlyphArrangement&&) = default;
	GlyphArrangement& operator=(GlyphArrangement&&) = default;
	//ConstText::Iterator begin();
	//ConstText::Iterator end();
	RectF area() const;
	double lineInterval() const;
	void scroll(double delta); //内部で持ってる要素分時間かかる
	Iterator calcDrawBegin(); //描画開始位置 [drawBegin,
	Iterator calcDrawEnd(); //描画終了位置 , drawEnd)

	//XXXExtended:テキストをはみ出さない限りは窓を拡張して有効なIteratorを返すことを保証する
	//（Iterator::firstがend()でないのにIterator::secondがend()であるようなイテレータを返すことはない）
	//nullSkipがtrueだと開始イテレータ以外のNULL文字をスキップして移動する
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
	SP<Text::Text> _text;
	Iterator _cursor; //視覚的なカーソルというより編集位置と思ったほうがよさそう
	int _editedCount;
	int _unsettledCount;
	bool _isEditing;
	
	//destroyed: destroyed以降が破壊されることを示す
	//edit: 文字列を編集しdestroyedに代わるイテレータを返す
	//Iterator editText(Iterator destroyed, std::function<Iterator()> edit);
public:
	TextWindow(SP<Text::Text> text, const RectF& area, double lineInterval, Vec2 originPos);
	Iterator insertText(Iterator itr, const String &s, bool rearranges = true);
	Iterator eraseText(Iterator first, Iterator last, bool rearranges = true);
	SP<const Text::Text> text();
	//UP<GlyphArrangement> cloneBack(Iterator newBegin); //[newBegin のGlyphArrangementを作る
	UP<GlyphArrangement> startEditing();
	void stopEditing();
	bool isEditing();
	Iterator cursor();
	Iterator editedBegin();
	Iterator unsettledBegin();
	void inputText(const String& addend, const String& editing); //編集中でなければ何もしない
	void eraseUnsettled();

	//編集中はカーソルを勝手に動かせない
	//範囲外にも動かせない
	//動かせない場合falseを返す
	bool cursorNext();
	bool cursorPrev();
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
	AnimationProgress(): _animationTime(), _sw(), _progress(0), _step(Step::Inactive) {}
	Step getStep() { return _step; }
	double getProgress() { return _progress; }
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


class ScrollDelta {
public:
	enum class Step { NotScrolling, Scrolling };
private:
	int _direction;
	double _used;
	Step _step;
	Stopwatch _sw;
public:
	ScrollDelta() : _step(Step::NotScrolling) {}
	Step step() { return _step; }
	int direction() { return _direction; }
	void startScroll(int direction) { //-1 or +1
		if (_step == Step::Scrolling) return;
		_direction = direction;
		_used = 0;
		_sw.restart();
		_step = Step::Scrolling;
	}
	void stopScroll() {
		if (_step == Step::NotScrolling) return;
		_step = Step::NotScrolling;
	}
	double useDelta() {
		//y = log(x + a) + log(1 / a)の積分
		auto f = [](double x) {
			double a = 0.1;
			return -x*std::log(a) + (x + a)*std::log(x + a) - x - a*std::log(a);
		};
		double t = _sw.sF();
		double sum = 1000*t + 100*(t*t);
		double ret = sum - _used;
		_used = sum;
		return ret*_direction;
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
	SP<const Text::Text> _text;
	ScrollDelta _scrollDelta;

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