#pragma once
#include "FontBase.h"


namespace UnnamedEditor {
namespace Font {


//適宜2^n間隔のサイズでグリフを生成して、それを伸縮することで任意サイズの描画を可能にする
//std::vectorのメモリ確保みたいなのと同じ原理
//サイズだけをなめらかに変化させることを考えた時、グリフ読み込みのための空間計算量と時間計算量共にlogで抑えられる
//(描画時のコストは当然別でかかるが)
class ChangeableFont : FontBase {
private:

	//キー(a, b): グリフサイズが2^a, グリフIDがb
	std::map<std::pair<int, FontDataPicker::GlyphIndex>, SP<Glyph>> _glyphMemo;

public:

	ChangeableFont(FTLibraryWrapper lib, std::string fontPath, bool isVertical);

	~ChangeableFont();

	SP<const Glyph> renderChar(char16_t charCode, double size, double &o_scale);

};


}
}