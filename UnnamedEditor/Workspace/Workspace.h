#pragma once
#include "Font\Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include "DraftPaper.h"

namespace UnnamedEditor {
namespace Workspace {


using DevicePos = Vec2;


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


class PaperManager {
private:

	int animationMS = 1000;

	DraftPaper _paper;

	DevicePos _start, _end;

	Stopwatch _sw;

public:

	PaperManager(const DraftPaper &paper, const DevicePos &start, const DevicePos &end)
	: _paper(paper)
	, _start(start)
	, _end(end) {


		_sw.start();
	}

	void update() {
		int ms = _sw.ms();
		if (ms > animationMS) _sw.pause();
		double t = std::min(1.0, ms/(double)animationMS);	
		_paper.setPos(EaseOut(Easing::Quint, _start, _end, t));
		_paper.draw();
	}
};


class Workspace {
private:

	DevicePos _pos, _size;

	int _fontSize;

	Font::Font _font;

	String _text;

	std::vector<SP<const Font::Glyph>> _glyphs;

	JudgeUnsettled _ju;

	int _draftCharCount;

	RectF _draftField;

	int _draftFontSize;

	Font::Font _draftFont;

	std::vector<PaperManager> _papers;

public:

	Workspace(DevicePos pos, DevicePos size, FT_Library lib);

	void addText(const String &text);

	void deleteText(int count = 1);

	void update();

};


}
}