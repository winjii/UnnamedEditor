#pragma once
#include <Siv3D.hpp>
#include <memory>

namespace UnnamedEditor {


template<class T> using SP = std::shared_ptr<T>;
template<class T> using UP = std::unique_ptr<T>;
template<class T> using WP = std::weak_ptr<T>;


}