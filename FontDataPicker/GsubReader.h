#pragma once
#include <ft2build.h>
#include <map>
#include FT_FREETYPE_H
#include FT_OPENTYPE_VALIDATE_H


namespace FontDataPicker {


using int16 = short;
using uint16 = unsigned short;
using GlyphIndex = uint16;

class GsubReader {
private:

	//Š—LŒ ‚Í‚½‚È‚¢
	FT_Face _face;

	std::map<GlyphIndex, GlyphIndex> _vertSubstitution;

public:

	GsubReader(FT_Face face);

	GlyphIndex vertSubstitute(GlyphIndex gid);

};


}