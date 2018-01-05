# include <Siv3D.hpp> // OpenSiv3D v0.1.7
#include <ft2build.h>
#include FT_FREETYPE_H
#include <FontDataPicker\GsubReader.h>
#include "Font\Font.h"
#include "FairCopyField\FairCopyField.h"
#include "Workspace\Workspace.h"

using namespace FontDataPicker;

void Main()
{
	enum RunMode {
		GsubReaderTest,
		FontTest,
		FairCopyFieldTest,
		WorkspaceTest,
		GlyphTest
	} runMode = RunMode::GlyphTest;

	if (runMode == RunMode::GsubReaderTest) {
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
	else if (runMode == RunMode::FontTest) {
		using namespace UnnamedEditor::Font;
		FT_Library lib;
		FT_Init_FreeType(&lib);
		UnnamedEditor::Font::Font font(lib, "C:/Windows/Fonts/msmincho.ttc", 30, 30, true);
		String s = L"「山村、（mucho）。」";
		auto v = font.renderString(s.toUTF16());

		Graphics::SetBackground(Palette::Gray);
		while (System::Update()) {
			Vec2 pen(100, 100);
			for (int i = 0; i < v.size(); i++) {
				pen = v[i]->draw(pen);
			}
		}
	}
	else if (runMode == RunMode::FairCopyFieldTest) {
		using namespace UnnamedEditor::FairCopyField;
		FT_Library lib;
		FT_Init_FreeType(&lib);
		FairCopyField fc(0, 0, Window::Width(), Window::Height(), lib, 20);
		fc.setText(L"　吾輩は猫である。名前はまだない。　どこで生れたか頓と見当がつかぬ。何でも薄暗いじめじめした所でニャーニャー泣いていた事だけは記憶している。吾輩はここで始めて人間というものを見た。");
		while (System::Update()) {
			fc.update();
		}
	}
	else if (runMode == RunMode::WorkspaceTest) {
		using namespace UnnamedEditor::Workspace;
		FT_Library lib;
		FT_Init_FreeType(&lib);
		Workspace w(Vec2(0, 0), Vec2(Window::Width(), Window::Height()), lib);
		w.addText(L"　吾輩は猫である。名前はまだない。　どこで生れたか頓と見当がつかぬ。何でも薄暗いじめじめした所でニャーニャー泣いていた事だけは記憶している。吾輩はここで始めて人間というものを見た。");
		while (System::Update()) {
			w.update();
		}
	}
	else if (runMode == RunMode::GlyphTest) {
		using namespace UnnamedEditor::Font;
		using namespace UnnamedEditor;
		FT_Library lib;
		FT_Init_FreeType(&lib);
		UnnamedEditor::Font::Font fontV(lib, "C:/Windows/Fonts/msmincho.ttc", 30, 30, true);
		UnnamedEditor::Font::Font fontH(lib, "C:/Windows/Fonts/msmincho.ttc", 30, 30, false);
		auto glyphsV = fontV.renderString(u"むーちょろくいちさん");
		auto glyphsH = fontH.renderString(u"mucho613");
		auto draw = [](const std::vector<SP<const UnnamedEditor::Font::Glyph>> &glyphs) {
			Vec2 pen(100, 100);
			for (int i = 0; i < (int)glyphs.size(); i++) {
				pen = glyphs[i]->draw(pen, Palette::Red, -Math::Pi/6.0);
			}
		};
		while (System::Update()) {
			draw(glyphsV);
			draw(glyphsH);
		}
	}
}
