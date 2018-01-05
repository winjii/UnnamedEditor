#pragma once
#include "Font\Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H

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


class Workspace {
private:

	DevicePos _pos, _size;

	int _fontSize;

	Font::Font _font;

	String _text;

	std::vector<SP<const Font::Glyph>> _glyphs;

	JudgeUnsettled _ju;

	Stopwatch _sw;

	std::vector<DevicePos> _targets;

public:

	Workspace(DevicePos pos, DevicePos size, FT_Library lib);

	void addText(const String &text);

	void deleteText();

	void update();

};


}
}