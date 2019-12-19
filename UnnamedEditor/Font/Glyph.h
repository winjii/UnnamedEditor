#pragma once

namespace UnnamedEditor {
namespace Font {


class Glyph {
private:

	static SP<Glyph> empty;

	bool _isVertical;

	int _fontSize;
	
	Vec2 _bearing;

	double _advance;
	
	Texture _texture;

public:

	Glyph(bool isVertical, int fontSize, double bearingX, double bearingY, double advance, const Image &image);

	~Glyph();

	static SP<Glyph> EmptyGlyph();

	//angle: rad
	Vec2 draw(const Vec2 &pen, const Color &color = Palette::Black, double angle = 0.0, double scale = 1.0) const;

	double advance() const;

	Vec2 advance(double angle) const;

	int getFontSize() const;

	bool isVertical() const;

	RectF boundingBox(const Vec2 &pen = Vec2(0, 0), double angle = 0.0) const;
};


}
}