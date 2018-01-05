#pragma once
#include "Font\Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H

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