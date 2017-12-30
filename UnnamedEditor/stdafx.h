#pragma once
#include <Siv3D.hpp>
#include <memory>
#include <vector>

namespace UnnamedEditor {


template<class T> using SP = std::shared_ptr<T>;
template<class T> using UP = std::unique_ptr<T>;
template<class T> using WP = std::weak_ptr<T>;

template<class T>
class OpaqueAlias : T {
public:
	OpaqueAlias(const T &t) : T(t) {}
	explicit operator T() const { return this; }
};


}