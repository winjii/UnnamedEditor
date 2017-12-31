#pragma once

namespace UnnamedEditor {
namespace Font {


class Glyph {
private:

	bool _isVertical;
	
	Vec2 _bearing;

	double _advance;
	
	Texture _texture;

public:

	Glyph(bool isVertical, double bearingX, double bearingY, double advance, const Image &image);

	~Glyph();

	Vec2 draw(const Vec2 &pen);

	double getAdvance();

};


}
}