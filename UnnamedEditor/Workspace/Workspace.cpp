#include "stdafx.h"
#include "Workspace.h"

namespace UnnamedEditor {
namespace Workspace {


Workspace::Workspace(DevicePos pos, DevicePos size, FT_Library lib)
: _pos(pos)
, _size(size)
, _fontSize(30)
, _font(lib, "C:/Windows/Fonts/msmincho.ttc", _fontSize, true)
, _draftCharCount(0)
, _draftField(pos, Vec2(size.y, size.x - _fontSize*2 - _fontSize))
, _draftFontSize(20)
, _draftFont(lib, "C:/Windows/Fonts/msmincho.ttc", _draftFontSize, false) {
}

void Workspace::addText(const String &text) {
	_text += text;
	_draftCharCount += text.length();
	std::vector<SP<const Font::Glyph>> res = _font.renderString(text.toUTF16());
	_glyphs.insert(_glyphs.end(), res.begin(), res.end());
}

void Workspace::deleteText(int count) {
	for (int i = 0; i < count && !_text.empty(); i++) {
		_draftCharCount = std::max(0, _draftCharCount - 1);
		_text.pop_back();
		_glyphs.pop_back();
	}
}

void Workspace::update() {
	String input;
	TextInput::UpdateText(input);
	addText(input);
	String unsettled = TextInput::GetMarkedText();
	_ju.update(unsettled.length());

	if (_ju.isSettled() && KeyBackspace.down())
		deleteText();
	if (KeyEnter.down()) {
		String draftText = _text.substr(_text.length() - (size_t)_draftCharCount, _draftCharCount);
		deleteText(_draftCharCount);
		double angle = Random(-Math::Pi, Math::Pi);
		DraftPaper dp(_draftFont.renderString(draftText.toUTF16()), angle);
		Vec2 margin = dp.desirableMargin();
		DevicePos end = RandomPoint(RectF(_draftField.pos + margin, _draftField.size - 2*margin));
		
		DevicePos start = [&](){
			DevicePos pos = _draftField.pos - margin, size = _draftField.size + margin*2;
			double rnd = Random()*(size.x + size.y)*2;
			
			//ã•Ó
			if (rnd < size.x) return pos + Vec2(rnd, 0);
			rnd -= size.x;
			//‰º•Ó
			if (rnd < size.x) return pos + Vec2(rnd, size.y);
			rnd -= size.x;
			//¶•Ó
			if (rnd < size.y) return pos + Vec2(0, rnd);
			rnd -= size.y;
			//‰E•Ó
			return pos + Vec2(size.x, rnd);
		}();

		_papers.push_back(PaperManager(dp, start, end));
	}

	RectF(_pos, _size).draw(Palette::White);

	const DevicePos head(_pos.x + _size.x - _fontSize*2, _pos.y + _size.y/2);
	{
		auto unsettledGlyhps = _font.renderString(unsettled.toUTF16());
		DevicePos charPos = head;
		for each (auto g in unsettledGlyhps) {
			g->draw(charPos, Palette::Gray);
			charPos += g->getAdvance();
		}
	}

	DevicePos charPos = head;
	for (int cnt = 0; cnt < (int)_glyphs.size(); cnt++) {
		int i = (int)_glyphs.size() - 1 - cnt;
		charPos -= _glyphs[i]->getAdvance();
		Color color = (cnt < _draftCharCount ? Palette::Red : Palette::Black);
		_glyphs[i]->draw(charPos, color);
		if (charPos.y < _pos.y) break;
	}

	for (int i = 0; i < (int)_papers.size(); i++) {
		_papers[i].update();
	}
}


}
}