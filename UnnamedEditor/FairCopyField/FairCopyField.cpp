#include "stdafx.h"
#include "FairCopyField.h"

namespace UnnamedEditor {
namespace FairCopyField {

FairCopyField::FairCopyField(double x, double y, double w, double h)
: _pos(Vec2(x, y)), _size(Vec2(w, h)) {
	
}

FairCopyField::~FairCopyField() {}

}
}