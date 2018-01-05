#include "stdafx.h"
#include "Workspace.h"

namespace UnnamedEditor {
namespace Workspace {


Workspace::Workspace(DevicePos pos, DevicePos size, FT_Library lib)
: _pos(pos)
, _size(size)
, _fontSize(20)
, _font(lib, "C:/Windows/Fonts/msmincho.ttc", _fontSize, _fontSize, true) {
	_text = L"　吾輩は猫である。名前はまだない。　どこで生れたか頓と見当がつかぬ。何でも薄暗いじめじめした所でニャーニャー泣いていた事だけは記憶している。吾輩はここで始めて人間というものを見た。";
	std::vector<Font::Glyph> res = _font.renderString(_text.toUTF16());
	_glyphs.insert(_glyphs.end(), res.begin(), res.end());
}

void Workspace::update() {
	//TODO: 文字列更新

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