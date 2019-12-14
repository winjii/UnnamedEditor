#include "stdafx.h"
#include "Workspace.h"

namespace UnnamedEditor {
namespace Workspace {


Workspace::Workspace(DevicePos pos, DevicePos size, FT_Library lib)
: _pos(pos)
, _size(size)
, _fontSize(20)
, _font(lib, "C:/Windows/Fonts/msmincho.ttc", _fontSize, true)
, _draftCharCount(0)
, _draftField(pos, Vec2(size.x - _fontSize*2 - _fontSize, size.y))
, _draftFontSize(15)
, _draftFont(lib, "C:/Windows/Fonts/msmincho.ttc", _draftFontSize, true) {
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
	String unsettled = TextInput::GetEditingText();
	_ju.update(unsettled.length());

	if (_ju.isSettled() && KeyBackspace.down())
		deleteText();
	else if (_ju.isSettled() && KeyDown.down() && _draftCharCount > 0) {
		_draftCharCount = 0;
	}
	else if (_ju.isSettled() && KeyLeft.down() && _draftCharCount > 0) {
		String draftText = _text.substr(_text.length() - (size_t)_draftCharCount, _draftCharCount);
		deleteText(_draftCharCount);
		double angle = Random(-Math::Pi/4.0, Math::Pi/4.0);
		DraftPaper dp(_draftFont.renderString(draftText.toUTF16()), angle);
		Vec2 margin = dp.desirableMargin();
		margin = Vec2(std::min(margin.x, _draftField.size.x/2.0 - 1e-3),
					  std::min(margin.y, _draftField.size.y/2.0 - 1e-3));
		DevicePos end = RandomVec2(RectF(_draftField.pos + margin, _draftField.size - 2*margin));
		
		Vec2 boundingSize = dp.boundingBox().size;
		DevicePos start = [&](){
			DevicePos pos = _pos - boundingSize, size = _size + boundingSize*2;
			double rnd = Random()*(size.x + size.y)*2;
			
			//è„ï”
			if (rnd < size.x) return pos + Vec2(rnd, 0);
			rnd -= size.x;
			//â∫ï”
			if (rnd < size.x) return pos + Vec2(rnd, size.y);
			rnd -= size.x;
			//ç∂ï”
			if (rnd < size.y) return pos + Vec2(0, rnd);
			rnd -= size.y;
			//âEï”
			return pos + Vec2(size.x, rnd);
		}();

		_papers.push_back(PaperManager(dp, start, end));
	}
	else if (_ju.isSettled() && KeyEnter.down()) {
		//âΩÇ‡Ç≥ÇπÇÒÇ≈
	}
	else if (_ju.isSettled() && KeyControl.pressed() && KeyV.down()) {
		String input;
		Clipboard::GetText(input);
		addText(input);
	}
	else if (KeyDelete.down()) {
		for (auto itr = _papers.begin(); itr != _papers.end();) {
			itr->disable();
			_garbagePapers.push_back(*itr);
			itr++;
		}
		_papers.clear();
	}
	else {
		String input;
		TextInput::UpdateText(input);
		addText(input);
	}

	RectF(_pos, _size).draw(Palette::White);
	const DevicePos head(_pos.x + _size.x - _fontSize*2, _pos.y + _size.y/2);
	{
		auto unsettledGlyhps = _font.renderString(unsettled.toUTF16());
		DevicePos charPos = head;
		for (auto g : unsettledGlyhps) {
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

	for (auto itr = _papers.begin(); itr != _papers.end();) {
		itr->update();
		itr++;
	}
	for (auto itr = _garbagePapers.begin(); itr != _garbagePapers.end();) {
		if (!itr->isInAnimation()) {
			itr = _garbagePapers.erase(itr);
			continue;
		}
		itr->update();
		itr++;
	}
}


}
}