# include <Siv3D.hpp> // OpenSiv3D v0.1.7
#include <fstream>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_IMAGE_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_GLYPH_H
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
				image[r][c] = Color(Palette::White, 255 * gray.v);
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
		Rect(0, 0, Window::ClientSize()).draw(Palette::White);
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

	Scene::SetBackground(Palette::Gray);
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
	FairCopyField::FairCopyField fc(0, 0, Window::ClientWidth(), Window::ClientHeight(), lib, 20);
	fc.setText(U"　吾輩は猫である。名前はまだない。　どこで生れたか頓と見当がつかぬ。何でも薄暗いじめじめした所でニャーニャー泣いていた事だけは記憶している。吾輩はここで始めて人間というものを見た。");
	while (System::Update()) {
		fc.update();
	}
}

void WorkspaceTest() {
	Window::Resize(Size(1280, 720));
	FT_Library lib;
	FT_Init_FreeType(&lib);
	Workspace::Workspace w(Vec2(0, 0), Vec2(Window::ClientWidth(), Window::ClientHeight()), lib);
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
	dpV.setPos(Window::ClientSize()/2.0);
	dpH.setPos(Window::ClientSize()/2.0);
	Vec2 marginV = dpV.desirableMargin();
	Vec2 marginH = dpH.desirableMargin();
	RectF rectV(dpV.getPos() - marginV, marginV*2);
	RectF rectH(dpH.getPos() - marginH, marginH*2);
	Scene::SetBackground(Palette::White);
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

	Scene::SetBackground(Palette::White);
	while (System::Update()) {
		if (MouseL.down()) mode = !mode;
		auto &glyphs = mode ? g1 : g0;
		Vec2 pen(200, 100);
		for (auto g : glyphs) {
			pen = g->draw(pen, Palette::Black, 0, 0.9);
		}
	}
}

void NoBug() {
	//テクスチャアドレスモードなるものをclampにすれば直るよ
	//デフォルトではrepeatになっていて、テクスチャが循環する
	const ScopedRenderStates2D state(SamplerState::ClampLinear);

	Image img(10, 10);
	for (int i = 0; i < img.height(); i++) {
		for (int j = 0; j < img.width(); j++) {
			if (i >= 5) img[i][j] = Color(Palette::White, 255);
			else img[i][j] = Color(Palette::White, 0);
		}
	}
	Texture texture(img);

	Scene::SetBackground(Palette::White);
	while (System::Update()) {
		texture.draw(Vec2(100, 100.5), Palette::Black); //変な線が入る
	}
}

void WholeViewTest() {
	
	FT_Library lib;
	FT_Init_FreeType(&lib);
	SP<Font::FixedFont> font(new Font::FixedFont(lib, "C:/Windows/Fonts/msmincho.ttc", 20, true));
	WholeView::WholeView wholeView(Vec2(0, 0), Vec2(Window::ClientWidth(), Window::ClientHeight()), font);
	String IamACat = TextReader(U"IamACat.txt").readAll();
	wholeView.setText(IamACat);

	MSRenderTexture msrt(Window::ClientSize());
	const ScopedRenderStates2D state(SamplerState::ClampLinear);
	while (System::Update()) {
		msrt.clear(Palette::White);
		{
			ScopedRenderTarget2D target(msrt);
			wholeView.minimapTest();
		}
		Graphics2D::Flush();
		msrt.resolve();
		msrt.scaled(1).draw(Arg::center(Window::ClientCenter()));
		//wholeView.draw();
	}
}

void MiniRenderTest() {
	FT_Library lib;
	FT_Init_FreeType(&lib);
	SP<Font::FixedFont> font(new Font::FixedFont(lib, "C:/Windows/Fonts/msmincho.ttc", 20, true));
	WholeView::WholeView wholeView(Vec2(0, 0), Vec2(Window::ClientWidth(), Window::ClientHeight()), font);
	String IamACat = TextReader(U"IamACat.txt").readAll();
	wholeView.setText(IamACat);

	MSRenderTexture msrt(Window::ClientSize());
	RenderTexture buf0(Window::ClientSize()), buf1(Window::ClientSize());
	const ScopedRenderStates2D state(SamplerState::ClampLinear);
	double la = 0;
	double ls = 0;
	double g = 0;
	ColorF mul();
	while (System::Update()) {
		double a = exp(la);
		double s = exp(ls);
		msrt.clear(Palette::White);
		buf0.clear(Palette::White);
		{
			ScopedRenderTarget2D target(msrt);
			wholeView.draw();
		}
		Graphics2D::Flush();
		msrt.resolve();
		Shader::GaussianBlur(msrt, buf0, Vec2(0, g));
		{
			//グレースケールにおいて、白(1)以外の色を黒(0)に近づけるための色演算
			//1 - a*(1 - x) = ax + (1 - a)
			ScopedColorMul2D mul(a);
			ScopedColorAdd2D add(1 - a);
			buf0.scaled(s).draw(Arg::center(Window::ClientCenter()));
		}
		//buf1.draw();
		SimpleGUI::Slider(U"a {:3.3}"_fmt(a), la, -3, 3, Vec2(0, 20), 100);
		SimpleGUI::Slider(U"s {:3.3}"_fmt(s), ls, -3, 3, Vec2(0, 60), 100);
		SimpleGUI::Slider(U"g {:3.3}"_fmt(g), g, -10, 10, Vec2(0, 100), 100);
	}
}

void ChangeableFontTest() {
	FT_Library lib;
	FT_Init_FreeType(&lib);
	Font::ChangeableFont font(lib, "C:/Windows/Fonts/msmincho.ttc", true);

	const ScopedRenderStates2D state(SamplerState::ClampLinear);
	Scene::SetBackground(Palette::White);
	s3d::Font sfont(30);
	std::u16string str = String(U"星宮いちご").toUTF16();
	while (System::Update()) {
		Vec2 pen(RectF(Window::ClientSize()).topCenter());
		for (int i = 0; i < str.length(); i++) {
			double scale;
			SP<const Font::Glyph> g = font.renderChar(str[i], 4 + 20*(1 + Math::Sin(Scene::FrameCount()/(60.0*5)*2*Math::Pi)), scale);
			sfont(scale).draw(Vec2(100, 100), Palette::Black);
			pen = g->draw(pen, Palette::Black, 0, scale);
		}
	}
}

void FontScaleTest() {
	using namespace UnnamedEditor::Font;
	FT_Library lib;
	FT_Init_FreeType(&lib);
	std::string path = "C:/Windows/Fonts/msmincho.ttc";
	std::vector<int> s{ 32, 37, 41, 43, 47, 53, 49, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107 };
	std::vector<FixedFont> fonts;
	//const ScopedRenderStates2D state(SamplerState::ClampLinear);
	Scene::SetBackground(Palette::White);
	for (int i = 0; i < s.size(); i++) {
		fonts.push_back(FixedFont(lib, path, s[i], true));
	}
	s3d::Font sf(32, U"C:/Windows/Fonts/msmincho.ttc");
	while (System::Update()) {
		
		for (int i = 0; i < fonts.size(); i++) {
			Vec2 p = Window::ClientCenter() + Vec2(-32*(int)fonts.size()/2 + 32*i, -16);
			fonts[i].renderChar(U'鬱')->draw(p, Palette::Black, 0, 32.0/s[i]);
		}
		DrawableText t = sf(U"鬱");
		t.draw(Window::ClientCenter() - Vec2(0, 32 + 16), Palette::Black);
	}
}

void FontShiftTest() {
	using namespace UnnamedEditor::Font;
	FT_Library lib;
	FT_Init_FreeType(&lib);
	FixedFont f(lib, "D:/Data/Source/Repos/UnnamedEditor/UnnamedEditor/App/SourceHanSerif-Regular.otf", 32, false);
	const ScopedRenderStates2D state(SamplerState::ClampNearest);
	Scene::SetBackground(Palette::White);
	s3d::Font sf(32, U"D:/Data/Source/Repos/UnnamedEditor/UnnamedEditor/App/SourceHanSerif-Regular.otf");
	while (System::Update()) {

		for (int i = 0; i < 16; i++) {
			Vec2 p = Window::ClientCenter() + Vec2(-32*16/2 + 32*i, 0);
			auto g = f.renderChar(U'鬱');
			g->draw(p + Vec2(i / 16.0, -32), Palette::Black);
			sf(U'鬱').draw(p + Vec2(i/16.0, 0), Palette::Black);
			Circle(p + Vec2(i / 16.0, -32), 3).draw(Palette::Red);
		}
	}
}

void RasterizeTest() {
	FT_Library lib;
	FT_Init_FreeType(&lib);
	FT_Face face;
	FT_New_Face(lib, "C:/Windows/Fonts/msmincho.ttc", 0, &face);
	
	FT_GlyphSlot slot = face->glyph;

	unsigned int charCode = L'あ';
	FT_UInt gid = FT_Get_Char_Index(face, charCode);

	FT_Set_Pixel_Sizes(face, 64, 64);
	FT_Load_Glyph(face, gid, FT_LOAD_NO_BITMAP);
	/*FT_Bitmap bitmap;
	bitmap.buffer = new unsigned char[64 * 64];
	memset(bitmap.buffer, 0, 64*64);
	bitmap.rows = 64;
	bitmap.width = 64;
	bitmap.pitch = 64;
	bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
	FT_Raster_Params rparams;
	rparams.flags = FT_RASTER_FLAG_AA;
	rparams.target = &bitmap;
	int err = FT_Outline_Render(lib, &(slot->outline), &rparams);*/
	FT_Glyph g;
	FT_Get_Glyph(slot, &g);
	FT_Glyph_To_Bitmap(&g, FT_RENDER_MODE_NORMAL, nullptr, true);
	const FT_Bitmap &bitmap = ((FT_BitmapGlyph)g)->bitmap;
	Image image = Image(bitmap.width, bitmap.rows);
	for (int r = 0; r < bitmap.rows; r++) {
		for (int c = 0; c < bitmap.width; c++) {
			HSV gray(Color(bitmap.buffer[r * bitmap.width + c]));
			image[r][c] = Color(Palette::White, 255 * gray.v);
		}
	}
	auto texture = DynamicTexture(image);
	Scene::SetBackground(Palette::White);
	while (System::Update()) {
		texture.draw(Window::ClientCenter(), Palette::Black);
	}
}

void GlyphLoadTest() {
	using namespace UnnamedEditor::Font;
	FT_Library lib;
	FT_Init_FreeType(&lib);
	FixedFont f(lib, "C:/Windows/Fonts/msmincho.ttc", 32, false);
	String IamACat = TextReader(U"IamACat.txt").readAll();
	Console.open();
	Stopwatch sw;
	sw.start();
	int cnt = 0;
	for (char16_t c : IamACat) {
		if (cnt % 10000 == 0) {
			printf("%d%%\n", 100*cnt/(int)IamACat.size());
		}
		f.renderChar(c);
		cnt++;
	}
	sw.pause();
	printf("end: %f s", sw.sF());
	system("pause");
}

void TextInputTest() {
	String raw;
	String str;
	while (System::Update()) {
		raw += TextInput::GetRawInput();
		TextInput::UpdateText(str);
		if (MouseL.down()) {
			printf("set break point here");
		}
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
		MiniRenderTest,
		ChangeableFontTest,
		FontScaleTest,
		RasterizeTest,
		FontShiftTest,
		GlyphLoadTest,
		TextInputTest,
	} runMode = RunMode::WholeViewTest;

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
	else if (runMode == RunMode::MiniRenderTest) {
		UnnamedEditor::MiniRenderTest();
	}
	else if (runMode == RunMode::ChangeableFontTest) {
		UnnamedEditor::ChangeableFontTest();
	}
	else if (runMode == RunMode::FontScaleTest) {
		UnnamedEditor::FontScaleTest();
	}
	else if (runMode == RunMode::RasterizeTest) {
		UnnamedEditor::RasterizeTest();
	}
	else if (runMode == RunMode::FontShiftTest) {
		UnnamedEditor::FontShiftTest();
	}
	else if (runMode == RunMode::GlyphLoadTest) {
		UnnamedEditor::GlyphLoadTest();
	}
	else if (runMode == RunMode::TextInputTest) {
		UnnamedEditor::TextInputTest();
	}
}
