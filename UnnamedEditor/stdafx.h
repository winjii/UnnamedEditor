#pragma once
#include <Siv3D.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <stdexcept>

namespace UnnamedEditor {

#define VAIN_STATEMENT std::cout << ""

template<class T> using SP = std::shared_ptr<T>;
template<class T> using UP = std::unique_ptr<T>;
template<class T> using WP = std::weak_ptr<T>;

using LLInt = long long int;


}