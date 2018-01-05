#pragma once

namespace UnnamedEditor {
namespace Font {


class Glyph {
private:

	bool _isVertical;

	double _fontSize;
	
	Vec2 _bearing;

	double _advance;
	
	Texture _texture;

public:

	Glyph(bool isVertical, double fontSize, double bearingX, double bearingY, double advance, const Image &image);

	~Glyph();

	//angle: rad
	Vec2 draw(const Vec2 &pen, const Color &color = Palette::Black, double angle = 0.0) const;

	Vec2 getAdvance(double angle = 0.0) const;

	double getFontSize() const;

	bool isVertical() const;

	RectF boundingBox(const Vec2 &pen = Vec2(0, 0), double angle = 0.0) const;
};


}
}