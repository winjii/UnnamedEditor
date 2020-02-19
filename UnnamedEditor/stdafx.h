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

namespace Util {


template<class Pointer> class PointerWrapperBase {
protected:
	virtual void destroy(Pointer p) const = 0;
private:
	struct Inner {
		Pointer _p;
		const PointerWrapperBase<Pointer>& _main;
		Inner(Pointer p, const PointerWrapperBase<Pointer>& main) : _p(p), _main(main) {}
		~Inner() { _main.destroy(_p); }
	};
	SP<Inner> _inner;
public:
	PointerWrapperBase<Pointer>(Pointer p) : _inner(new Inner(p, *this)) {}
	Pointer operator->() const { return _inner->_p; }
	Pointer raw() { return _inner->_p; }
};


}


}