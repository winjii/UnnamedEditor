# include <Siv3D.hpp> // OpenSiv3D v0.1.7
#include <ft2build.h>
#include FT_FREETYPE_H
#include <FontDataPicker\GsubReader.h>

using namespace FontDataPicker;

void Main()
{
	FT_Library lib;
	FT_Init_FreeType(&lib);
	FT_Face face;
	FT_New_Face(lib, "C:/Windows/Fonts/msmincho.ttc", 0, &face);

	FT_GlyphSlot slot = face->glyph;
	Image image;
	DynamicTexture texture;
	auto init = [&](int pw, int ph) {
		FT_Set_Pixel_Sizes(face, pw, ph);
		
		GsubReader gr(face);

		unsigned int charCode = L'。';
		FT_UInt gid = FT_Get_Char_Index(face, charCode);
		FT_Load_Glyph(face, gr.vertSubstitute(gid), FT_LOAD_NO_BITMAP); //MS明朝において、NO_BITMAPにしないと読み込めないケースあり
		FT_Render_Glyph(face->glyph, FT_Render_Mode::FT_RENDER_MODE_NORMAL);
		FT_Bitmap bitmap = face->glyph->bitmap;
		image = Image(bitmap.width, bitmap.rows);
		for (int r = 0; r < bitmap.rows; r++) {
			for (int c = 0; c < bitmap.width; c++) {
				HSV gray(Color(bitmap.buffer[r*bitmap.width + c]));
				image[r][c] = HSV(0, 0, 1 - gray.v);
			}
		}
		texture = DynamicTexture(image);

	};
	int pw = 16, ph = 16;
	init(pw, ph);
	while (System::Update()) {
		if (KeyUp.down()) {
			pw++; ph++;
			init(pw, ph);
		}
		else if (KeyDown.down()) {
			pw = Max(1, pw - 1); ph = Max(1, ph - 1);
			init(pw, ph);
		}
		Rect(0, 0, Window::Size()).draw(Palette::White);
		Point pos(100 - (slot->metrics.horiBearingX >> 6), 100 - (slot->metrics.horiBearingY >> 6));
		//Rect(pos, bitmap.width, bitmap.rows).drawFrame(0, 5, Palette::Blue);
		texture.fill(image);
		texture.draw(pos);
	}
}
