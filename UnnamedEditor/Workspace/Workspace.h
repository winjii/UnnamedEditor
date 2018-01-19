#pragma once
#include "Font\FixedFont.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include "DraftPaper.h"

namespace UnnamedEditor {
namespace Workspace {


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


class PaperManager {
private:

	int animationMS = 1000;

	DraftPaper _paper;

	DevicePos _start, _end;

	Stopwatch _sw;

	bool _isDisabled;

public:

	PaperManager(const DraftPaper &paper, const DevicePos &start, const DevicePos &end, bool isEnabled = true)
	: _paper(paper)
	, _start(start)
	, _end(end)
	, _isDisabled(!isEnabled) {


		_sw.start();
	}

	void update() {
		int ms = _sw.ms();
		if (ms > animationMS) _sw.pause();
		double t = std::min(1.0, ms/(double)animationMS);
		if (_isDisabled)
			_paper.setPos(EaseOut(Easing::Quint, _end, _start, t));
		else
			_paper.setPos(EaseOut(Easing::Quint, _start, _end, t));
		_paper.draw();
	}

	bool isDisabled() { return _isDisabled; }

	void disable() {
		if (_isDisabled) return;
		_isDisabled = true;
		_sw.restart();
	}

	void enable() {
		if (!_isDisabled) return;
		_isDisabled = false;
		_sw.restart();
	}

	bool isInAnimation() {
		return _sw.ms() <= animationMS;
	}
};


class Workspace {
private:

	DevicePos _pos, _size;

	int _fontSize;

	Font::FixedFont _font;

	String _text;

	std::vector<SP<const Font::Glyph>> _glyphs;

	JudgeUnsettled _ju;

	int _draftCharCount;

	RectF _draftField;

	int _draftFontSize;

	Font::FixedFont _draftFont;

	std::list<PaperManager> _papers;

	std::list<PaperManager> _garbagePapers;

public:

	Workspace(DevicePos pos, DevicePos size, FT_Library lib);

	void addText(const String &text);

	void deleteText(int count = 1);

	void update();

};


}
}