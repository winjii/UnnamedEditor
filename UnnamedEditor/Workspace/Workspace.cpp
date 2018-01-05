#include "stdafx.h"
#include "Workspace.h"

namespace UnnamedEditor {
namespace Workspace {


Workspace::Workspace(DevicePos pos, DevicePos size, FT_Library lib)
: _pos(pos)
, _size(size)
, _fontSize(20)
, _font(lib, "C:/Windows/Fonts/msmincho.ttc", _fontSize, _fontSize, true) {
	_text = L"�@��y�͔L�ł���B���O�͂܂��Ȃ��B�@�ǂ��Ő��ꂽ���ڂƌ��������ʁB���ł����Â����߂��߂������Ńj���[�j���[�����Ă����������͋L�����Ă���B��y�͂����Ŏn�߂Đl�ԂƂ������̂������B";
	std::vector<Font::Glyph> res = _font.renderString(_text.toUTF16());
	_glyphs.insert(_glyphs.end(), res.begin(), res.end());
}

void Workspace::update() {
	//TODO: ������X�V

	RectF(_pos, _size).draw(Palette::White);
	DevicePos charPos(_pos.x + _size.x - _fontSize*2, _pos.y + _size.y/2);
	for (int i = _glyphs.size() - 1; i >= 0; i--) {
		charPos -= _glyphs[i].getAdvance();
		_glyphs[i].draw(charPos);
		if (charPos.y < _pos.y) break;
	}
}


}
}