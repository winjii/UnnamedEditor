#include "Glyph.h"

namespace UnnamedEditor {
namespace Font {


Glyph::Glyph(bool isVertical, double bearingX, double bearingY, double advance, const Image &image)
: _isVertical(isVertical)
, _bearing(bearingX, bearingY)
, _advance(advance)
, _texture(image) {
	
}

Glyph::~Glyph() {}

Vec2 Glyph::draw(const Vec2 &pen, const Color &color, double angle) const {
	Vec2 res = pen;
	if (!_texture.isEmpty()) {
		_texture.rotateAt(0, 0, angle).draw(pen + _bearing.rotated(angle), color);
	}
	res += getAdvance(angle);
	return res;
}

Vec2 Glyph::getAdvance(double angle) const {
	if (_isVertical) return Vec2(0, _advance).rotate(angle);
	return Vec2(_advance, 0).rotate(angle);
}


}
}