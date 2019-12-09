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


//↓xとyを取り替えるだけでなく正負なども変えなきゃいけない...
//仮想的なテキスト内座標を作って描画時に変換したほうが良さそう
//class TextDirection {
//private:
//	bool _isVertical;
//public:
//	bool isVertical() { return _isVertical; }
//	int& perpendicular(Point& p) {
//		if (_isVertical) return p.x;
//		return p.y;
//	}
//	int& parallel(Point& p) {
//		if (_isVertical) return p.y;
//		return p.x;
//	}
//	double& perpendicular(Vec2& p) {
//		if (_isVertical) return p.x;
//		return p.y;
//	}
//	double& parallel(Vec2& p) {
//		if (_isVertical) return p.y;
//		return p.x;
//	}
//};



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

	//カーソル移動先が安全な場合のみ正常に動く。いずれ置き換える
	void setCursorUnsafe(Iterator cursor);
};


//文字の見た目を一時的に変化させる
class TemporaryCharRender {
public:
	class Pointer : protected SP<TemporaryCharRender> {
		using Base = SP<TemporaryCharRender>;
	public:
		bool isValid() const { return operator bool(); }
		void destroy() { Base::reset(); }
		TemporaryCharRender* operator->() const { return Base::operator->(); }
		TemporaryCharRender& operator*() const { return Base::operator*(); }
	};
	class Manager { //Pointerを管理して自動で無効にする
	private:
		std::list<SP<TemporaryCharRender::Pointer>> _data;
	public:
		void update() {
			auto itr = _data.begin();
			while (itr != _data.end()) {
				if (!(**itr)->update()) {
					(*itr)->destroy();
					itr = _data.erase(itr);
				}
				else ++itr;
			}
		}
		void registerPointer(SP<TemporaryCharRender::Pointer> p) {
			_data.push_back(p);
		}
	};

	virtual bool update() = 0; //殺されたくないうちはtrueを差し出す
	virtual void draw(Vec2 pos) = 0;
};


//TODO: あるlineからn（1000とか10000とか）文字以下のlineまでのミニマップをキャッシュ（バケット法）
//↑line1つのミニマップもキャッシュ
//↑ミニマップ描画をshortcutできるイメージ
//↑文字列編集されたときはどうする？
//↑あるバケットが更新されたらその一つ手前のバケットから再計算→統合できるのにされていない隣接バケットが生まれない→例えばn=1000だとしたら、最悪ケースでも500と501や1000と1のバケットが交互に並ぶだけ→バケットの最大個数は2√nくらいで抑えられる
//テキストの変更をまたいで使うイテレータはちゃんとregisterItr()する。ただしこれらのイテレータを含む範囲を削除してはいけない...
//TODO: 末尾の改行消さないようにする
//TODO: 管理されたイテレータが削除されるケースをどうにかしたい
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
		std::list<SP<CharIterator>> itrs;
		Point pos; //line内での相対的な位置
	};
	struct BucketHeader {
		int wrapCount;
		MSRenderTexture minimap;
	};
	struct LineData {
		std::vector<CharData> cd; //テキスト末尾に改行
		int advance;
		SP<BucketHeader> bucketHeader;
	};
private:
	SP<Font::FixedFont> _font;
	std::list<LineData> _data;
	int _lineInterval;
	int _maxLineLnegth;
	LineIterator _origin;
	Point _originPos;
	SP<CharIterator> _cursor;
	SP<MSRenderTexture> _bufferTexture0;
	SP<RenderTexture> _bufferTexture1;

	//glyph, posを計算する。cd.empty()だったら削除
	LineIterator initLine(LineIterator litr);

	void initBucket(LineIterator first, LineIterator last);
public:
	GlyphArrangement2(SP<Font::FixedFont> font, int lineInterval, int maxLineLength);

	void registerItr(SP<CharIterator> itr);
	void removeItr(SP<CharIterator> itr);
	LineIterator tryNext(LineIterator litr, int cnt = 1);
	LineIterator tryPrev(LineIterator litr, int cnt = 1);
	CharIterator insertText(CharIterator citr, const String &s);
	CharIterator eraseText(CharIterator first, CharIterator last);
	CharIterator replaceText(CharIterator first, CharIterator last, const String& s);
	void scroll(int delta);
	CharIterator next(CharIterator citr, bool overLine = false, int cnt = 1) const;
	CharIterator prev(CharIterator citr, bool overLine = false, int cnt = 1) const;
	void next(SP<CharIterator> citr); //管理されたイテレータを動かす
	void prev(SP<CharIterator> citr); //管理されたイテレータを動かす
	LineIterator origin() const;
	Point originPos() const;
	SP<CharIterator> cursor() const;
	LineIterator begin();
	LineIterator end();
	CharIterator lineBegin(LineIterator litr) const;
	CharIterator lineEnd(LineIterator litr) const;

	//NULL文字を挿入すると番兵などに使える。ただしそれを含む範囲をeraseしないよう注意
	//NULL文字は参照が切れていたらinitLine時に自動で削除される
	//↑しかしいつ削除されるか分からないのでdeleteNullしたほうがいい
	SP<CharIterator> makeNull(CharIterator citr);
	void deleteNull(SP<CharIterator> nullItr); //イテレータは壊れる
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
	Step getStep() const { return _step; }
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
	std::vector<Vec2> _oldHeadPos;
	std::vector<Vec2> _newHeadPos;
	double _oldAdvance; //headのadvance
	double _newAdvance;

	Vec2 easeOverLine(Vec2 source, Vec2 target, double t, int i);
public:
	FloatingAnimation(int lineInterval, int maxLineLength);
	Step step() const;
	bool isInactive() const;
	GA::CharIterator floatingBegin() const;
	bool isStable() const; //文字が整数座標に静止しているか
	void startFloating(GA& ga, GA::CharIterator floatingBegin);
	void stopFloating();
	void updateTime();
	void updatePos(const GA& ga); //変更が起きたときのみ呼び出す

	//citrはfloatingStart以降であること
	//Inactiveのときに呼び出さない
	//各行の行頭の静止座標からの相対位置を返す（描画する側で行頭の静止座標を足す）
	Vec2 getPos(GA::CharIterator citr);
};


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
public:
	CleanCopyCursor(GA& ga, GA::CharIterator citr);
	GA::CharIterator pos(const GA& ga) const;
	std::pair<GA::CharIterator, double> drawingPos() const;
	void changeItr(GA& ga, GA::CharIterator newItr);
	bool isStable();
	void startAdvancing();
	void startRetreating();
	void stop(GA& ga);
	void update(GA& ga);
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
	InputManager(SP<FloatingAnimation> fa);
	bool isInputing() const;
	SP<FloatingAnimation> floatingAnimation() const;
	SP<CleanCopyCursor> cleanCopyCursor() const;
	void inputText(GA &ga, const String& addend, const String& editing);
	void stopInputing();
	void startInputing(GA& ga);
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
	Rect _textArea;
	int _lineInterval;
	SP<Font::FixedFont> _font;
	GlyphArrangement2 _ga;
	JudgeUnsettled _ju;
	ScrollDelta _scrollDelta;
	SP<FloatingAnimation> _floating;
	InputManager _inputManager;
	RenderTexture _masker;
	RenderTexture _maskee;
	const PixelShader _maskPS;

	Vec2 floatingTextIn(Vec2 source, Vec2 target, double t, int i);
	Vec2 floatingTextOut(Vec2 source, Vec2 target, double t, int i);

public:

	//font: verticalでなければならない(これ設計汚くない？)
	WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font);

	void setText(const String &text);
	void draw();
	void minimapTest();
};


}
}