#pragma once

namespace UnnamedEditor {
namespace Geometry {


// friend指定できるようにクラスにしてある
// xとかyとかをこのクラスだけに公開できる
class Impl {
public:
	template <class T>
	static std::pair<T, T> ToPositiveRect(const T& pos, const T& size) {
		T rpos = pos, rsize = size;
		if (rsize.x < 0) { rpos.x += rsize.x; rsize.x *= -1; }
		if (rsize.y < 0) { rpos.y += rsize.y; rsize.y *= -1; }
		return { rpos, rsize };
	}

	template <class T>
	static decltype(T::x) Dot(const T& p, const T& q) { return p.x * q.x + p.y * q.y; }

	template <class T>
	static T Abs(const T& p) { return { std::abs(p.x), std::abs(p.y) }; }
};
template<class T>
inline std::pair<T, T> ToPositiveRect(const T& pos, const T& size) {
	return Impl::ToPositiveRect(pos, size);
}
template <class T>
inline decltype(T::x) Dot(const T& p, const T& q) {
	return Impl::Dot(p, q);
}
template<class T>
inline T Abs(const T& p) {
	return Impl::Abs(p);
}



}
}