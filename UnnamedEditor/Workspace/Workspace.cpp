#include "stdafx.h"
#include "Workspace.h"

namespace UnnamedEditor {
namespace Workspace {


Workspace::Workspace(DevicePos pos, DevicePos size, FT_Library lib)
: _pos(pos)
, _size(size)
, _fontSize(20)
, _font(lib, "C:/Windows/Fonts/msmincho.ttc", _fontSize, _fontSize, true) {
	_targets.resize(15);
	for (int i = 0; i < 15; i++) {
		_targets[i] = RandomPoint(RectF(_pos, _size));
	}
}

void Workspace::addText(const String & text) {
	_text += text;
	std::vector<SP<const Font::Glyph>> res = _font.renderString(text.toUTF16());
	_glyphs.insert(_glyphs.end(), res.begin(), res.end());
}

void Workspace::deleteText() {
	if (_text.empty()) return;
	_text.pop_back();
	_glyphs.pop_back();
}

void Workspace::update() {
	String input;
	TextInput::UpdateText(input);
	addText(input);
	String unsettled = TextInput::GetMarkedText();
	_ju.update(unsettled.length());
	if (_ju.isSettled() && KeyBackspace.down())
		deleteText();

	if (KeyDown.down()) {
		_sw.start();
	}
	if (_sw.ms() > 2500) {
		_sw.pause();
	}

	RectF(_pos, _size).draw(Palette::Lightgrey);

	const DevicePos head(_pos.x + _size.x - _fontSize*2, _pos.y + _size.y/2);
	{
		auto unsettledGlyhps = _font.renderString(unsettled.toUTF16());
		DevicePos charPos = head;
		for each (auto g in unsettledGlyhps) {
			g->draw(charPos, Palette::Red);
			charPos += g->getAdvance();
		}
	}

	DevicePos charPos = head;
	for (int i = (int)_glyphs.size() - 1; i >= 0; i--) {
		charPos -= _glyphs[i]->getAdvance();
		DevicePos end = charPos;
		int j = (int)_glyphs.size() - 1 - i;
		if (j < _targets.size()) {
			end = _targets[j];
		}
		_glyphs[i]->draw(EaseIn(Easing::Bounce, charPos, end, std::min(1.0, _sw.ms()/2500.0)));
		if (charPos.y < _pos.y) break;
	}
}


}
}