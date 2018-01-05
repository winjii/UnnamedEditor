#pragma once
#include "Font\Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace UnnamedEditor {
namespace Workspace {


using DevicePos = Vec2;


class Workspace {
private:

	DevicePos _pos, _size;

	int _fontSize;

	Font::Font _font;

	String _text;

	std::vector<SP<const Font::Glyph>> _glyphs;

public:

	Workspace(DevicePos pos, DevicePos size, FT_Library lib);

	void addText(const String &text);

	void deleteText();

	void update();

};


}
}