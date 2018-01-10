#pragma once
#include "Font/Glyph.h"

namespace UnnamedEditor {
namespace Workspace {


using DevicePos = Vec2;


class DraftPaper {
private:

	//(0, 0)�ƃI�u�W�F�N�g�̈ʒu����v����悤��
	RectF _paper;

	std::vector<SP<const Font::Glyph>> _glyphs;

	std::vector<DevicePos> _glyphPositions;

	//glyphs��boundingBox�̒��S�̈ʒu���A���̃I�u�W�F�N�g�̈ʒu
	DevicePos _pos;

	//�����̃f�t�H���g�̊p�x��0�Ƃ���rad
	double _angle;

	Vec2 _desirableMargin;

public:

	DraftPaper(const std::vector<SP<const Font::Glyph>> &glyphs, double angle);

	void setPos(const DevicePos &pos);

	DevicePos getPos() const;

	void draw() const;
	
	//���������܂邽�߂ɏ㉺�ƍ��E�ɕK�v�ȃ}�[�W��
	Vec2 desirableMargin();

	RectF boundingBox(const Vec2 &pos = Vec2(0, 0));
	
};


}
}