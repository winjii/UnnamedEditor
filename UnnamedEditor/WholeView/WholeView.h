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


//TODO: ����line����n�i1000�Ƃ�10000�Ƃ��j�����ȉ���line�܂ł̃~�j�}�b�v���L���b�V���i�o�P�b�g�@�j
//��line1�̃~�j�}�b�v���L���b�V��
//���~�j�}�b�v�`���shortcut�ł���C���[�W
//��������ҏW���ꂽ�Ƃ��͂ǂ�����H
//������o�P�b�g���X�V���ꂽ�炻�̈��O�̃o�P�b�g����Čv�Z�������ł���̂ɂ���Ă��Ȃ��אڃo�P�b�g�����܂�Ȃ����Ⴆ��n=1000���Ƃ�����A�ň��P�[�X�ł�500��501��1000��1�̃o�P�b�g�����݂ɕ��Ԃ������o�P�b�g�̍ő����2��n���炢�ŗ}������
//TODO: �����̉��s�����Ȃ��悤�ɂ���
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
		Point pos; //line���ł̑��ΓI�Ȉʒu
	};
	struct BucketHeader {
		int size;
	};
	struct LineData {
		std::vector<CharData> cd; //�e�L�X�g�����ɉ��s
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

	LineIterator initLine(LineIterator litr); //glyph, pos���v�Z cd.empty()��������폜
	void initBucket(LineIterator fisrt, LineIterator last);
	void registerItr(SP<CharIterator> itr);
public:
	GlyphArrangement2(SP<Font::FixedFont> font, int lineInterval, int maxLineLength);

	LineIterator tryNext(LineIterator litr, int cnt = 1);
	LineIterator tryPrev(LineIterator litr, int cnt = 1);
	void insertText(CharIterator citr, const String &s);
	void eraseText(CharIterator first, CharIterator last);
	void scroll(int delta);
	CharIterator next(CharIterator citr) const;
	CharIterator prev(CharIterator citr) const;
	void next(SP<CharIterator> citr);
	void prev(SP<CharIterator> citr);
	LineIterator origin() const;
	Point originPos() const;
	SP<CharIterator> cursor() const;
	LineIterator endLine();
};


class FloatingArrangement {
private:
	GlyphArrangement2::CharIterator _floatingStart;
	GlyphArrangement2::LineData _head;
	GlyphArrangement2::LineIterator _body;
public:

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

	enum class FloatingStep {
		Inactive, AnimatingIn, Stable, AnimatingOut
	};

	RectF _area;
	Rect _textArea;
	int _lineInterval;
	SP<Font::FixedFont> _font;
	GlyphArrangement2 _ga;
	JudgeUnsettled _ju;
	FloatingStep _floatingStep;
	AnimationProgress _floatingProgress;
	SP<const Text::Text> _text;
	ScrollDelta _scrollDelta;

	Vec2 floatingTextIn(Vec2 source, Vec2 target, double t, int i);
	Vec2 floatingTextOut(Vec2 source, Vec2 target, double t, int i);

public:

	//font: vertical�łȂ���΂Ȃ�Ȃ�(����݌v�����Ȃ��H)
	WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font);

	void setText(const String &text);
	void draw();
};


}
}