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

RectF Glyph::boundingBox(const Vec2 &pen, double angle) const {
	Vec2 v1 = _bearing;
	Vec2 v2 = v1 + Vec2(_texture.width(), 0);
	Vec2 v3 = v1 + Vec2(_texture.width(), _texture.height());
	Vec2 v4 = v1 + Vec2(0, _texture.height());
	v1.rotate(angle);
	v2.rotate(angle);
	v3.rotate(angle);
	v4.rotate(angle);
	Vec2 min(std::min(std::min(v1.x, v2.x), std::min(v3.x, v4.x)),
			 std::min(std::min(v1.y, v2.y), std::min(v3.y, v4.y)));
	Vec2 max(std::max(std::max(v1.x, v2.x), std::max(v3.x, v4.x)),
			 std::max(std::max(v1.y, v2.y), std::max(v3.y, v4.y)));
	return RectF(pen + min, max - min);
}


}
}