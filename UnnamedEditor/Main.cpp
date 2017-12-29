# include <Siv3D.hpp> // OpenSiv3D v0.1.7
#include <ft2build.h>
#include FT_FREETYPE_H

void Main()
{
	FT_Library lib;
	FT_Init_FreeType(&lib);
	FT_Face face;
	FT_New_Face(lib, "C:/Windows/Fonts/msmincho.ttc", 0, &face);
}
