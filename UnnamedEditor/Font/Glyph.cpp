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

Vec2 Glyph::draw(const Vec2 &pen) {
	Vec2 res = pen;
	_texture.draw(pen + _bearing);
	if (_isVertical) res.y += _advance;
	else res.x += _advance;
	return res;
}


}
}