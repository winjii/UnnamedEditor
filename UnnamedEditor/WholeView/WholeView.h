#pragma once
#include "Font\FixedFont.h"
#include "Text.h"
#include <list>
#include <cmath>


namespace UnnamedEditor {
namespace WholeView {


using DevicePos = Vec2;


//���m�蕶������̓��[�h���ǂ����𔻒肷��N���X
//���m�蕶������폜�������ɁA���ɂ��̍폜�̂��߂Ɏg��ꂽ���͂��A�m�蕶����̍폜������Ă��܂��i���O�̂����j���Ƃւ̑Ώ����ł���
//�i�m�蕶����ւ̕ҏW����莞�ԃu���b�N���邾���j
//TODO: ���m�蕶����unsettled����Ȃ�editing�Ŗ��O����
class JudgeUnsettled {
private:
	int _count;
public:
	JudgeUnsettled() : _count(0) {}
	bool isUnsettled() { return _count > 0; }
	bool isSettled() { return _count == 0; }

	//1�t���[�����
	//unsettledStrSize: ���݂̖��m�蕶����̒���
	void update(int unsettledStrLength) {
		if (unsettledStrLength == 0) _count = std::max(_count - 1, 0);
		else _count = 5;
	}
};


//TODO: start�܂ŋ�ł��悭�Ȃ���
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


//��x��y�����ւ��邾���łȂ������Ȃǂ��ς��Ȃ��Ⴂ���Ȃ�...
//���z�I�ȃe�L�X�g�����W������ĕ`�掞�ɕϊ������ق����ǂ�����
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



//�e�L�X�g��\�����邽�߂̃O���t�z�u���v�Z����
//�e�L�X�g�̍X�V�͂ł����A�e�L�X�g���폜���ꂽ������Text::Iterator�͉���i�X�V�ɑΉ�����ɂ�TextWindow���g���j
//�O���ŏ����iterator�𓮂�������Q�Ɛ�ɏ������񂾂肵���Ⴂ���Ȃ�
//TODO: ���{���͌^�Ő������ׂ������̃N���X����Ȃ��ƃA�N�Z�X�ł��Ȃ��Ǝ��C�e���[�^���`
//��������̑��i_begin����_end�܂Łj��XXXExtended�ɂ���Ă����L����Ȃ��i�����ŏ���ɐL�k���Ȃ��j
//���镶���̈ʒu�����߂�̂ɐ�΂ɂ��̑O�̉��s�܂ł͑k��Ȃ���΂����Ȃ�����܂�Ԃ������ʂɉ����Čv�Z�ʂ�������͎̂d���Ȃ�
//�����łȂ��d�؂���w���C�e���[�^��NULL�����ƈꏏ�ɓ����i������}���Ȃǂ���Ƃ��d�؂�̈ʒu�𓮂����Ȃ��Ă���������������j
//TODO; NULL������K�X����
//TODO: begin��end�̖���������Â�[�[
//TODO: ��Ԃ̈�������������ƂȂ�Ƃ��Ȃ��
class GlyphArrangement {
public:
	using Iterator = std::pair<Text::Text::Iterator, std::list<Vec2>::iterator>;
private:
	SP<Text::Text> _text;
	RectF _area;
	double _lineInterval;
protected:
	std::list<Vec2> _pos; //�O���t�ʒu�L���b�V���B�v�f�������Ă����邱�Ƃ͂Ȃ��i�C�e���[�^���Ȃ��j

	//NULL�����ɑΉ�����C�e���[�^�Q
	//���ʂ̕������l�A_cacheBegin��_cacheEnd�̊Ԃɂ������̓O���t�ʒu���v�Z�����
	Iterator _cacheBegin, _cacheEnd; //(_cachBegin, _cacheEnd]�̓L���b�V���ς݂̋�ԁi�����ԈႢ�ł͂Ȃ��j
	Text::Text::Iterator _begin, _end; //(_begin, _end]�͊O������A�N�Z�X����͈�

	Iterator next(Iterator itr);
	Iterator prev(Iterator itr);
	Iterator advanced(Iterator itr, int d);
	void arrange(Iterator first, Iterator last); //first�̃O���t�ʒu����ɂ���(first, last]�̃O���t�ʒu���v�Z
	bool onTextArea(Iterator itr) const; //�`��G���A���ɔ��\���������on
	bool upperTextArea(Iterator itr) const;
	bool lowerTextArea(Iterator itr) const;
	//Iterator eraseSafe(Iterator first, Iterator last); //NULL����������č폜

	//�K�X_pos���g�����邪�O���t�ʒu�̌v�Z�܂ł͂��Ȃ�
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
	void scroll(double delta); //�����Ŏ����Ă�v�f�����Ԃ�����
	Iterator calcDrawBegin(); //�`��J�n�ʒu [drawBegin,
	Iterator calcDrawEnd(); //�`��I���ʒu , drawEnd)

	//XXXExtended:�e�L�X�g���͂ݏo���Ȃ�����͑����g�����ėL����Iterator��Ԃ����Ƃ�ۏ؂���
	//�iIterator::first��end()�łȂ��̂�Iterator::second��end()�ł���悤�ȃC�e���[�^��Ԃ����Ƃ͂Ȃ��j
	//nullSkip��true���ƊJ�n�C�e���[�^�ȊO��NULL�������X�L�b�v���Ĉړ�����
	Iterator prevExtended(Iterator itr);
	Iterator nextExtended(Iterator itr);
	Iterator prevExtended(Iterator itr, int cnt);
	Iterator nextExtended(Iterator itr, int cnt);
};


//�O���t�z�u�Ɠ��e�ҏW��S���A��������ۂ�
//�x���]���I�ɓ����Ȃ�ׂ��K�v�ŏ����̘J�͂Ŕz�u���v�Z����
//���ۂɃe�L�X�g��\������ɂ͕`��͈͂ɉ������`��R�X�g���ő喈�t���[�������邽�߁A����Ɣ�ׂăl�b�N�ɂȂ�Ȃ��R�X�g�œ����΂���
class TextWindow : public GlyphArrangement {
private:
	SP<Text::Text> _text;
	Iterator _cursor; //���o�I�ȃJ�[�\���Ƃ������ҏW�ʒu�Ǝv�����ق����悳����
	int _editedCount;
	int _unsettledCount;
	bool _isEditing;
	
	//destroyed: destroyed�ȍ~���j�󂳂�邱�Ƃ�����
	//edit: �������ҏW��destroyed�ɑ���C�e���[�^��Ԃ�
	//Iterator editText(Iterator destroyed, std::function<Iterator()> edit);
public:
	TextWindow(SP<Text::Text> text, const RectF& area, double lineInterval, Vec2 originPos);
	Iterator insertText(Iterator itr, const String &s, bool rearranges = true);
	Iterator eraseText(Iterator first, Iterator last, bool rearranges = true);
	SP<const Text::Text> text();
	//UP<GlyphArrangement> cloneBack(Iterator newBegin); //[newBegin ��GlyphArrangement�����
	UP<GlyphArrangement> startEditing();
	void stopEditing();
	bool isEditing();
	Iterator cursor();
	Iterator editedBegin();
	Iterator unsettledBegin();
	void inputText(const String& addend, const String& editing); //�ҏW���łȂ���Ή������Ȃ�
	void eraseUnsettled();

	//�ҏW���̓J�[�\��������ɓ������Ȃ�
	//�͈͊O�ɂ��������Ȃ�
	//�������Ȃ��ꍇfalse��Ԃ�
	bool cursorNext();
	bool cursorPrev();

	//�J�[�\���ړ��悪���S�ȏꍇ�̂ݐ���ɓ����B������u��������
	void setCursorUnsafe(Iterator cursor);
};


//���͂ŏ��ł��ė~�����f�[�^
class TemporaryData {
public:
	//�E���ꂽ���Ȃ�������true�������o��
	//1�t���[��1��
	virtual bool update() = 0;
	
	virtual void destroy() = 0;
	virtual bool isEmpty() = 0;

	static bool IsEmpty(SP<TemporaryData> p) {
		return !p || p->isEmpty();
	}

	class Manager { //Pointer���Ǘ����Ď����Ŗ����ɂ���
	private:
		std::list<SP<TemporaryData>> _data;
	public:
		void update() { //1�t���[��1��
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

			RectF(Vec2(_pos.x - _region.w, _pos.y), _region.w, _region.h).draw(Palette::White);
			Vec2 c = Vec2(_pos.x - _region.w / 2.0, _pos.y + _region.h / 2.0);
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


//TODO: ����line����n�i1000�Ƃ�10000�Ƃ��j�����ȉ���line�܂ł̃~�j�}�b�v���L���b�V���i�o�P�b�g�@�j
//��line1�̃~�j�}�b�v���L���b�V��
//���~�j�}�b�v�`���shortcut�ł���C���[�W
//��������ҏW���ꂽ�Ƃ��͂ǂ�����H
//������o�P�b�g���X�V���ꂽ�炻�̈��O�̃o�P�b�g����Čv�Z�������ł���̂ɂ���Ă��Ȃ��אڃo�P�b�g�����܂�Ȃ����Ⴆ��n=1000���Ƃ�����A�ň��P�[�X�ł�500��501��1000��1�̃o�P�b�g�����݂ɕ��Ԃ������o�P�b�g�̍ő����2��n���炢�ŗ}������
//�e�L�X�g�̕ύX���܂����Ŏg���C�e���[�^�͂�����registerItr()����B�����������̃C�e���[�^���܂ޔ͈͂��폜���Ă͂����Ȃ�...
//TODO: �����̉��s�����Ȃ��悤�ɂ���
//TODO: cursor���̂������H
//TODO: �Ǘ����ꂽ�C�e���[�^�͎�����removeItr���Ăяo���悤�ɂ���΂����̂ł�
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
		Point pos; //line���ł̑��ΓI�Ȉʒu
		SP<CharAnimation> animation;
	};
	struct BucketHeader {
		int wrapCount;
		MSRenderTexture minimap;
		int advance;
		Vec2 origin;
	};
	struct LineData {
		std::vector<CharData> cd; //�e�L�X�g�����ɉ��s
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
	LineIterator _origin;
	Point _originPos;
	SP<CharIterator> _cursor;
	SP<MSRenderTexture> _bufferTexture0;
	SP<RenderTexture> _bufferTexture1;
	const double _minimapFontSize = 4;

	//glyph, pos���v�Z����Bcd.empty()��������폜
	LineIterator initLine(LineIterator litr);

	void initBucket(LineIterator first, LineIterator last);
public:
	GlyphArrangement2(SP<Font::FixedFont> font, int lineInterval, int maxLineLength);

	SP<Font::FixedFont> font();
	void registerItr(SP<CharIterator> itr);
	void removeItr(SP<CharIterator> itr);
	LineIterator tryNext(LineIterator litr, int cnt = 1) const;
	LineIterator tryPrev(LineIterator litr, int cnt = 1) const;
	CharIterator insertText(CharIterator citr, const String &s);
	CharIterator eraseText(CharIterator first, CharIterator last); //�Ǘ����ꂽ�C�e���[�^��last�ɂǂ���
	CharIterator replaceText(CharIterator first, CharIterator last, const String& s);
	void scroll(int delta);
	CharIterator next(CharIterator citr, bool overLine = false, int cnt = 1) const;
	CharIterator prev(CharIterator citr, bool overLine = false, int cnt = 1) const;
	void next(SP<CharIterator> citr); //�Ǘ����ꂽ�C�e���[�^�𓮂���
	void prev(SP<CharIterator> citr); //�Ǘ����ꂽ�C�e���[�^�𓮂���
	LineIterator origin() const;
	Point originPos() const;
	SP<CharIterator> cursor() const;
	LineIterator begin();
	LineIterator end();
	CharIterator lineBegin(LineIterator litr) const;
	CharIterator lineEnd(LineIterator litr) const;
	LineIterator bucket(LineIterator litr) const;
	LineIterator nextBucket(LineIterator litr) const;
	double minimapLineInterval() const;
	double minimapScale() const;

	//NULL������}������Ɣԕ��ȂǂɎg����B������������܂ޔ͈͂�erase���Ȃ��悤����
	//NULL�����͎Q�Ƃ��؂�Ă�����initLine���Ɏ����ō폜�����
	//�����������폜����邩������Ȃ��̂�deleteNull�����ق�������
	SP<CharIterator> makeNull(CharIterator citr);
	void deleteNull(SP<CharIterator> nullItr); //�C�e���[�^�͉���
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
	SP<GA::CharIterator> _floatingBegin; //�Ǘ����ꂽ�C�e���[�^
	AnimationProgress _inOutAP;
	AnimationProgress _updateAP;
	std::vector<Vec2> _oldHeadPos;
	std::vector<Vec2> _newHeadPos;
	double _oldAdvance; //head��advance
	double _newAdvance;

	Vec2 easeOverLine(Vec2 source, Vec2 target, double t, int i);
public:
	FloatingAnimation(int lineInterval, int maxLineLength);
	Step step() const;
	bool isInactive() const;
	GA::CharIterator floatingBegin() const;
	bool isStable() const; //�������������W�ɐÎ~���Ă��邩
	void startFloating(GA& ga, GA::CharIterator floatingBegin);
	void stopFloating();
	void updateTime();
	void updatePos(const GA& ga); //�ύX���N�����Ƃ��̂݌Ăяo��

	//citr��floatingStart�ȍ~�ł��邱��
	//Inactive�̂Ƃ��ɌĂяo���Ȃ�
	//�e�s�̍s���̐Î~���W����̑��Έʒu��Ԃ��i�`�悷�鑤�ōs���̐Î~���W�𑫂��j
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


//���͏�Ԃ̂Ƃ��ɓ����@�\�����܂Ƃ߂�
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
	void inputText(SP<GA> ga, const String& addend, const String& editing);
	void stopInputing();
	void startInputing(SP<GA> ga);
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
		//y = log(x + a) + log(1 / a)�̐ϕ�
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
	SP<GlyphArrangement2> _ga;
	JudgeUnsettled _ju;
	ScrollDelta _scrollDelta;
	SP<FloatingAnimation> _floating;
	InputManager _inputManager;
	RenderTexture _masker;
	RenderTexture _maskee;
	MSRenderTexture _foreground;
	const PixelShader _maskPS;
	TemporaryData::Manager _tmpData;

public:

	//font: vertical�łȂ���΂Ȃ�Ȃ�(����݌v�����Ȃ��H)
	WholeView(Rect area, SP<Font::FixedFont> font);

	void setText(const String &text);
	void draw();
	void minimapTest();
	SP<GlyphArrangement2> GlyphArrangement() const;
};


class MinimapView {
	using GA = GlyphArrangement2;
private:
	RectF _area;
	Rect _body;
	SP<GA> _ga;
	TemporaryData::Manager _tmpManager;
public:
	MinimapView(RectF area, SP<GA> ga);
	void draw();
};


}
}