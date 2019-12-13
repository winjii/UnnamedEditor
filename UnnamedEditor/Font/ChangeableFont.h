#pragma once
#include "FontBase.h"


namespace UnnamedEditor {
namespace Font {


//�K�X2^n�Ԋu�̃T�C�Y�ŃO���t�𐶐����āA�����L�k���邱�ƂŔC�ӃT�C�Y�̕`����\�ɂ���
//std::vector�̃������m�ۂ݂����Ȃ̂Ɠ�������
//�T�C�Y�������Ȃ߂炩�ɕω������邱�Ƃ��l�������A�O���t�ǂݍ��݂̂��߂̋�Ԍv�Z�ʂƎ��Ԍv�Z�ʋ���log�ŗ}������
//(�`�掞�̃R�X�g�͓��R�ʂł����邪)
class ChangeableFont : FontBase {
private:

	//�L�[(a, b): �O���t�T�C�Y��2^a, �O���tID��b
	std::map<std::pair<int, FontDataPicker::GlyphIndex>, SP<Glyph>> _glyphMemo;

public:

	ChangeableFont(FTLibraryWrapper lib, std::string fontPath, bool isVertical);

	~ChangeableFont();

	SP<const Glyph> renderChar(char16_t charCode, double size, double &o_scale);

};


}
}