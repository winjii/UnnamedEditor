#pragma once
#include "Geometry.h"

namespace UnnamedEditor::TextGeometry {

namespace G = Geometry;


// 上方向は考慮しない。上方向を扱い始めるとascenderとdescenderの扱いが面倒
enum Direction {
	RightDown,
	LeftDown,
	DownRight,
	DownLeft,
	DirectionCount // 列挙体の要素数
};
inline const static Point Dir1[(int)DirectionCount] = {
	{  1,  0 },
	{ -1,  0 },
	{  0,  1 },
	{  0,  1 },
};
inline const static Point Dir2[(int)DirectionCount] = {
	{  0,  1 },
	{  0,  1 },
	{  1,  0 },
	{ -1,  0 },
};
static Direction SwapAxis(Direction d) {
	return (Direction)((d + DirectionCount / 2) % DirectionCount);
}

static bool IsVertical(Direction d) {
	return d == Direction::DownLeft || d == Direction::DownRight;
}


template <class T>
struct PosOnText;

template class PosOnText<Point>;
template class PosOnText<Vec2>;
using PointOnText = PosOnText<Point>;
using Vec2OnText = PosOnText<Vec2>;

template <class T>
struct PosOnText : protected T {
	friend Geometry::Impl;
public:
	using Element = decltype(T::x);
	Element& parallel = T::x;
	Element& perpendicular = T::y;
	Element& prl = T::x;
	Element& prp = T::y;
private:
	PosOnText(const T& p) : T(p.x, p.y) {}
public:
	PosOnText() : T() {}
	PosOnText(const PosOnText& p) : T(p.x, p.y) {}
	PosOnText(const T& p, Direction d) {
		parallel = G::Dot<T>(Dir1[d], p);
		perpendicular = G::Dot<T>(Dir2[d], p);
	}
	PosOnText(Element parallel, Element perpendicular)
		: T(parallel, perpendicular) {
	}
	T toRealPos(Direction d) {
		return parallel * Dir1[d] + perpendicular * Dir2[d];
	}
	PosOnText& operator=(const PosOnText& p) { T::operator=(p); return *this; }
	Vec2OnText toTextVec2() { return { (double)parallel, (double)perpendicular }; }
	PointOnText toTextPoint() { return { (int)(parallel + 0.5), (int)(perpendicular + 0.5) }; }
	PosOnText operator+(const PosOnText& p) const { return T::operator+(p); }
	PosOnText operator-(const PosOnText& p) const { return T::operator-(p); }
	PosOnText& operator+=(const PosOnText& p) { T::operator+=(p); return *this; }
	PosOnText& operator-=(const PosOnText& p) { T::operator-=(p); return *this; }
	PosOnText operator*(Element e) const { return T::operator*(e); }
	PosOnText operator/(Element e) const { return T::operator/(e); }
	PosOnText& operator*=(Element e) { T::operator*=(e); return *this; }
	PosOnText& operator/=(Element e) { T::operator/=(e); return *this; }
};

// decltype(T::x)& は decltype(PosOnText<T>::Element)& とは書けない
template<class T> decltype(T::x)& X(PosOnText<T>& p) { return p.prl; }
template<class T> decltype(T::y)& Y(PosOnText<T>& p) { return p.prp; }
template<class T> decltype(T::x) X(const PosOnText<T>& p) { return p.prl; }
template<class T> decltype(T::y) Y(const PosOnText<T>& p) { return p.prp; }


template <class T>
struct RectangleOnText;

template class RectangleOnText<Point>;
template class RectangleOnText<Vec2>;
using RectOnText = RectangleOnText<Point>;
using RectFOnText = RectangleOnText<Vec2>;

template <class T>
struct RectangleOnText {
	PosOnText<T> pos;
	PosOnText<T> size;
	RectangleOnText() {}
	RectangleOnText(const PosOnText<T>& pos, const PosOnText<T>& size) : pos(pos), size(size) {}
	Rectangle<T> toRealRect(Direction d) {
		auto [rpos, rsize] = G::ToPositiveRect(pos.toRealPos(d), size.toRealPos(d));
		return Rectangle<T>(rpos, rsize);
	}
};


struct TextArea {
	Point origin;
	PointOnText size;
	TextArea() {}
	TextArea(const Rect& r, Direction d) {
		PointOnText tpos_(r.pos, d);
		PointOnText tsize_(r.size, d);
		auto [tpos, tsize] = G::ToPositiveRect(tpos_, tsize_);
		size = tsize;
		origin = tpos.toRealPos(d);
	}
	TextArea(const Point& pos, const PointOnText& size) : origin(pos), size(size) {}
	Size realSize(Direction d) {
		return Abs(size.toRealPos(d));
	}
	Point realPos(Direction d) {
		return toRect(d).pos;
	}
	Rect toRect(Direction d) {
		auto [rpos, rsize] = G::ToPositiveRect(origin, size.toRealPos(d));
		return Rect(rpos, rsize);
	}
};


}