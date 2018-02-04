# include <Siv3D.hpp> // OpenSiv3D v0.1.7
#include <fstream>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <FontDataPicker\GsubReader.h>
#include "Font\FixedFont.h"
#include "FairCopyField\FairCopyField.h"
#include "Workspace\Workspace.h"
#include "Workspace\DraftPaper.h"
#include "WholeView\WholeView.h"
#include "Font\ChangeableFont.h"

using namespace FontDataPicker;

namespace UnnamedEditor {

void GsubReaderTest() {
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

void FontTest() {
	using namespace UnnamedEditor::Font;
	FT_Library lib;
	FT_Init_FreeType(&lib);
	UnnamedEditor::Font::FixedFont font(lib, "C:/Windows/Fonts/msmincho.ttc", 30, true);
	String s = U"「山村、（mucho）。」";
	auto v = font.renderString(s.toUTF16());

	Graphics::SetBackground(Palette::Gray);
	while (System::Update()) {
		Vec2 pen(100, 100);
		for (int i = 0; i < v.size(); i++) {
			pen = v[i]->draw(pen);
		}
	}
}

void FairCopyFieldTest() {
	FT_Library lib;
	FT_Init_FreeType(&lib);
	FairCopyField::FairCopyField fc(0, 0, Window::Width(), Window::Height(), lib, 20);
	fc.setText(U"　吾輩は猫である。名前はまだない。　どこで生れたか頓と見当がつかぬ。何でも薄暗いじめじめした所でニャーニャー泣いていた事だけは記憶している。吾輩はここで始めて人間というものを見た。");
	while (System::Update()) {
		fc.update();
	}
}

void WorkspaceTest() {
	Window::Resize(Size(1280, 720));
	FT_Library lib;
	FT_Init_FreeType(&lib);
	Workspace::Workspace w(Vec2(0, 0), Vec2(Window::Width(), Window::Height()), lib);
	while (System::Update()) {
		w.update();
	}
}

void GlyphTest() {
	using namespace UnnamedEditor::Font;
	using namespace UnnamedEditor;
	FT_Library lib;
	FT_Init_FreeType(&lib);
	UnnamedEditor::Font::FixedFont fontV(lib, "C:/Windows/Fonts/msmincho.ttc", 30, true);
	UnnamedEditor::Font::FixedFont fontH(lib, "C:/Windows/Fonts/msmincho.ttc", 30, false);
	auto glyphsV = fontV.renderString(u"むーちょろくいちさん");
	auto glyphsH = fontH.renderString(u"mucho613");
	auto draw = [](const std::vector<SP<const UnnamedEditor::Font::Glyph>> &glyphs) {
		Vec2 pen(100, 100);
		for (int i = 0; i < (int)glyphs.size(); i++) {
			glyphs[i]->boundingBox(pen, -Math::Pi/6.0).drawFrame(0.5, Palette::White);
			pen = glyphs[i]->draw(pen, Palette::Red, -Math::Pi/6.0);
		}
	};
	while (System::Update()) {
		draw(glyphsV);
		draw(glyphsH);
	}
}

void DraftPaperTest() {
	using namespace UnnamedEditor::Font;
	using namespace UnnamedEditor;
	using namespace UnnamedEditor::Workspace;
	FT_Library lib;
	FT_Init_FreeType(&lib);
	UnnamedEditor::Font::FixedFont fontV(lib, "C:/Windows/Fonts/msmincho.ttc", 30, true);
	UnnamedEditor::Font::FixedFont fontH(lib, "C:/Windows/Fonts/msmincho.ttc", 30, false);
	auto glyphsV = fontV.renderString(u"むーーーーーーーーーーーーーーちょろくいちさん");
	auto glyphsH = fontH.renderString(u"mucho613");
	DraftPaper dpV(glyphsV, -Math::Pi/6.0);
	DraftPaper dpH(glyphsH, Math::Pi/6.0);
	dpV.setPos(Window::Size()/2.0);
	dpH.setPos(Window::Size()/2.0);
	Vec2 marginV = dpV.desirableMargin();
	Vec2 marginH = dpH.desirableMargin();
	RectF rectV(dpV.getPos() - marginV, marginV*2);
	RectF rectH(dpH.getPos() - marginH, marginH*2);
	Graphics::SetBackground(Palette::White);
	while (System::Update()) {
		dpV.draw();
		rectV.drawFrame(1, Palette::Blue);
		dpH.draw();
		rectH.drawFrame(1, Palette::Blue);
	}
}

void Glyph_scaleTest() {
	FT_Library lib;
	FT_Init_FreeType(&lib);
	UnnamedEditor::Font::FixedFont font0(lib, "C:/Windows/Fonts/msmincho.ttc", 32, true);
	UnnamedEditor::Font::FixedFont font1(lib, "C:/Windows/Fonts/msmincho.ttc", 31, true);
	String str = U"三人寄ればソレイユ！"; //OpenSiv3Dのバグにより変な線が入る
	//描画時に座標をasPoint()すると直るが...
	auto g0 = font0.renderString(str.toUTF16());
	auto g1 = font1.renderString(str.toUTF16());
	bool mode = false;

	Graphics::SetBackground(Palette::White);
	while (System::Update()) {
		if (MouseL.down()) mode = !mode;
		auto &glyphs = mode ? g1 : g0;
		Vec2 pen(200, 100);
		for each (auto g in glyphs) {
			pen = g->draw(pen, Palette::Black, 0, 0.9);
		}
	}
}

void NoBug() {
	//テクスチャアドレスモードなるものをclampにすれば直るよ
	//デフォルトではrepeatになっていて、テクスチャが循環する
	Graphics2D::SetSamplerState(SamplerState::ClampLinear);

	Image img(10, 10);
	for (int i = 0; i < img.height(); i++) {
		for (int j = 0; j < img.width(); j++) {
			if (i >= 5) img[i][j] = Color(Palette::White, 255);
			else img[i][j] = Color(Palette::White, 0);
		}
	}
	Texture texture(img);

	Graphics::SetBackground(Palette::White);
	while (System::Update()) {
		texture.draw(Vec2(100, 100.5), Palette::Black); //変な線が入る
	}
}

void WholeViewTest() {
	String IamACat = TextReader(U"IamACat.txt").readAll();
	
	FT_Library lib;
	FT_Init_FreeType(&lib);
	SP<Font::FixedFont> font(new Font::FixedFont(lib, "C:/Windows/Fonts/msmincho.ttc", 24, true));
	WholeView::WholeView wholeView(Vec2(0, 0), Vec2(Window::Width(), Window::Height()), font);
	wholeView.setText(IamACat);

	Graphics2D::SetSamplerState(SamplerState::ClampLinear);
	while (System::Update()) {
		wholeView.update();
	}
}

void ChangeableFontTest() {
	FT_Library lib;
	FT_Init_FreeType(&lib);
	Font::ChangeableFont font(lib, "C:/Windows/Fonts/msmincho.ttc", true);
	
	Graphics2D::SetSamplerState(SamplerState::ClampLinear);
	Graphics::SetBackground(Palette::White);
	s3d::Font sfont(30);
	std::u16string str = String(U"星宮いちご").toUTF16();
	while (System::Update()) {
		Vec2 pen(Window::ClientRect().topCenter());
		for (int i = 0; i < str.length(); i++) {
			double scale;
			SP<const Font::Glyph> g = font.renderChar(str[i], 4 + 20*(1 + Math::Sin(System::FrameCount()/(60.0*5)*2*Math::Pi)), scale);
			sfont(scale).draw(Vec2(100, 100), Palette::Black);
			pen = g->draw(pen, Palette::Black, 0, scale);
		}
	}
}

void FloatingTextTest() {
	using namespace WholeView;
	FT_Library lib;
	FT_Init_FreeType(&lib);
	Font::FixedFont font(lib, "C:/Windows/Fonts/msmincho.ttc", 20, true);
	std::u16string s = String(U"星宮いちごは、ごくごくフツーの中学1年生の女の子。ところが、親友のあおいに誘われてアイドル養成の名門校「スターライト学園」に編入したことで、いちごをとりまく世界がガラリと変わってしまう。様々なライバルたちと出会い、アイドルとしての心得を学びながら、いちごはアイカツ！カードを使って数々のオーディションに挑戦していくことに。新人アイドルいちごの、明るく元気なアイドル活動が幕を開ける…！").toUTF16(); 
	auto glyphs = font.renderString(s);
	Vec2 origin = Window::Center() + Vec2(50, 0);
	FloatingText ft(Window::ClientRect(), glyphs, 25, origin);

	Graphics2D::SetSamplerState(SamplerState::ClampLinear);
	Graphics::SetBackground(Palette::White);
	while (System::Update()) {
		if (ft.getState() == FloatingText::Inactive) {
			ft.transitIn(-50);
		}
		else if (ft.getState() == FloatingText::Stable) {
			ft.transitOut(origin);
		}
		ft.update();
	}
}

}

void Main()
{
	enum RunMode {
		GsubReaderTest,
		FontTest,
		FairCopyFieldTest,
		WorkspaceTest,
		GlyphTest,
		DraftPaperTest,
		Glyph_scaleTest,
		NoBug,
		WholeViewTest,
		ChangeableFontTest,
		FloatingTextTest
	} runMode = RunMode::FloatingTextTest;

	if (runMode == RunMode::GsubReaderTest) {
		UnnamedEditor::GsubReaderTest();
	}
	else if (runMode == RunMode::FontTest) {
		UnnamedEditor::FontTest();
	}
	else if (runMode == RunMode::FairCopyFieldTest) {
		UnnamedEditor::FairCopyFieldTest();
	}
	else if (runMode == RunMode::WorkspaceTest) {
		UnnamedEditor::WorkspaceTest();
	}
	else if (runMode == RunMode::GlyphTest) {
		UnnamedEditor::GlyphTest();
	}
	else if (runMode == RunMode::DraftPaperTest) {
		UnnamedEditor::DraftPaperTest();
	}
	else if (runMode == RunMode::Glyph_scaleTest) {
		UnnamedEditor::Glyph_scaleTest();
	}
	else if (runMode == RunMode::NoBug) {
		UnnamedEditor::NoBug();
	}
	else if (runMode == RunMode::WholeViewTest) {
		UnnamedEditor::WholeViewTest();
	}
	else if (runMode == RunMode::ChangeableFontTest) {
		UnnamedEditor::ChangeableFontTest();
	}
	else if (runMode == RunMode::FloatingTextTest) {
		UnnamedEditor::FloatingTextTest();
	}
}
