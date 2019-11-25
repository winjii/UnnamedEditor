#pragma once
#include "Font\FixedFont.h"
#include <list>


namespace UnnamedEditor {
namespace WholeView {


using DevicePos = Vec2;
const char16_t ZERO_WIDTH_SPACE = U'\u200B';
const char16_t NEWLINE = U'\n';


//���m�蕶������̓��[�h���ǂ����𔻒肷��N���X
//���m�蕶������폜�������ɁA���ɂ��̍폜�̂��߂Ɏg��ꂽ���͂��A�m�蕶����̍폜������Ă��܂��i���O�̂����j���Ƃւ̑Ώ����ł���
//�i�m�蕶����ւ̕ҏW����莞�ԃu���b�N���邾���j
class JudgeUnsettled {
private:

	int _count;

public:

	JudgeUnsettled() : _count(0) {}

	//1�t���[�����
	//unsettledStrSize: ���݂̖��m�蕶����̒���
	void update(int unsettledStrLength) {
		if (unsettledStrLength == 0) _count = std::max(_count - 1, 0);
		else _count = 5;
	}

	bool isUnsettled() { return _count > 0; }

	bool isSettled() { return _count == 0; }
};

//���������̍폜�͔�����̂͂ǂ��̖�����

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
	std::pair<Iterator, int> lineHead(Iterator itr) const; //[�擪������Iterator, �擪�����܂ł̋���]
	std::pair<Iterator, int> nextLineHead(Iterator itr) const; //���s��������Ȃ����end()�܂ł̋����ɂȂ�
};


//�e�L�X�g��\�����邽�߂̃O���t�z�u���v�Z����
//�e�L�X�g�̍X�V�͂ł����A�e�L�X�g���폜���ꂽ������Text::Iterator�͉���i�X�V�ɑΉ�����ɂ�TextWindow���g���j
//�O���ŏ����iterator�𓮂�������Q�Ɛ�ɏ������񂾂肵���Ⴂ���Ȃ�
//TODO: ���{���͌^�Ő������ׂ������̃N���X����Ȃ��ƃA�N�Z�X�ł��Ȃ��Ǝ��C�e���[�^���`
//��������̑��i_begin����_end�܂Łj��XXXExtended�ɂ���Ă����L����Ȃ��i�����ŏ���ɐL�k���Ȃ��j
//���镶���̈ʒu�����߂�̂ɐ�΂ɂ��̑O�̉��s�܂ł͑k��Ȃ���΂����Ȃ�����܂�Ԃ������ʂɉ����Čv�Z�ʂ�������͎̂d���Ȃ�
class GlyphArrangement {
public:
	using Iterator = std::pair<Text::Iterator, std::list<Vec2>::iterator>;
private:
	SP<const Text> _text;
	RectF _area;
	double _lineInterval;

	Iterator nextUnsafe(Iterator itr);
	Iterator prevUnsafe(Iterator itr);
	Iterator advancedUnsafe(Iterator itr, int d);
protected:
	std::list<Vec2> _pos; //���s�ŋ�؂�ꂽ�u���b�N�P�ʂŃL���b�V������Ă���
	Iterator _begin;
	Text::Iterator _beginConstraints, _endConstraints;

	void arrange(Iterator first, Iterator last, Vec2 origin);
public:
	GlyphArrangement(SP<const Text> text, const RectF& area, double lineInterval, Vec2 originPos);
	GlyphArrangement(const GlyphArrangement& ga, Iterator beginConstraints);
	GlyphArrangement(GlyphArrangement&&) = default;
	GlyphArrangement& operator=(GlyphArrangement&&) = default;
	Text::Iterator beginConstraints() const;
	Text::Iterator endConstraints() const;
	RectF area() const;
	double lineInterval() const;
	bool onTextArea(Iterator itr) const; //�`��G���A���ɔ��\���������on
	bool upperTextArea(Iterator itr) const;
	bool lowerTextArea(Iterator itr) const;
	void scroll(double delta); //�����Ŏ����Ă�v�f�����Ԃ�����
	void disable();

	//XXXExtended:�e�L�X�g���͂ݏo���Ȃ�����͑����g�����ėL����Iterator��Ԃ����Ƃ�ۏ؂���
	//�iIterator::first��end()�łȂ��̂�Iterator::second��end()�ł���悤�ȃC�e���[�^��Ԃ����Ƃ͂Ȃ��j
	Iterator prevExtended(Iterator itr);
	Iterator nextExtended(Iterator itr);
	Iterator prevExtended(Iterator itr, int cnt);
	Iterator nextExtended(Iterator itr, int cnt);
	Iterator beginExtended();

	//�`��̊J�n�ʒu�܂�begin���ړ�������
	void fitBegin();
};


//�O���t�z�u�Ɠ��e�ҏW��S���A��������ۂ�
//�x���]���I�ɓ����Ȃ�ׂ��K�v�ŏ����̘J�͂Ŕz�u���v�Z����
//���ۂɃe�L�X�g��\������ɂ͕`��͈͂ɉ������`��R�X�g���ő喈�t���[�������邽�߁A����Ɣ�ׂăl�b�N�ɂȂ�Ȃ��R�X�g�œ����΂���
class TextWindow : public GlyphArrangement {
	SP<Text> _text;
public:
	TextWindow(SP<Text> text, const RectF& area, double lineInterval, Vec2 originPos);
	Iterator insertText(Iterator itr, String s);
	Iterator eraseText(Iterator first, Iterator last);
	SP<const Text> text();
	UP<GlyphArrangement> detachBack(Iterator partition);
	void undetatch(UP<GlyphArrangement> ga);
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
	TextWindow::Iterator _cursor;
	JudgeUnsettled _ju;
	FloatingStep _floatingStep;
	AnimationProgress _floatingProgress;
	SP<const Text> _text;

	Vec2 floatingTextIn(Vec2 source, Vec2 target, double t, int i);
	Vec2 floatingTextOut(Vec2 source, Vec2 target, double t, int i);

public:

	//font: vertical�łȂ���΂Ȃ�Ȃ�(����݌v�����Ȃ��H)
	WholeView(const DevicePos &pos, const DevicePos &size, SP<Font::FixedFont> font);

	void setText(const String &text);
	void update();
};


}
}