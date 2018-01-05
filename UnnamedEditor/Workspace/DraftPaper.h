#pragma once
#include "Font/Glyph.h"

namespace UnnamedEditor {
namespace Workspace {


using DevicePos = Vec2;


class DraftPaper {
private:

	RectF _paper;

	std::vector<SP<const Font::Glyph>> _glyphs;

	std::vector<DevicePos> _glyphPositions;

	//glyphs��boundingBox�̒��S�̈ʒu���A���̃I�u�W�F�N�g�̈ʒu
	DevicePos _pos;

	//��ʉ���0�Ƃ���rad
	double _angle;

	Vec2 _desirableMargin;

public:

	DraftPaper(const std::vector<SP<const Font::Glyph>> &glyphs, double angle);

	void setPos(const DevicePos &pos);

	DevicePos getPos() const;

	void draw() const;
	
	//���������܂邽�߂ɕK�v�ȃ}�[�W��
	DevicePos desirableMargin();

	
};


}
}