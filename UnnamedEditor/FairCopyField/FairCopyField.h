#pragma once

namespace UnnamedEditor {
namespace FairCopyField {

using DevicePos = OpaqueAlias<Vec2>;
using LogicPos = OpaqueAlias<Vec2>;

class CharPos {
private:

	LLInt _lineIndex;
	
	double _posInLine;

public:

	CharPos(LLInt lineIndex, double posInLine) : _lineIndex(lineIndex), _posInLine(posInLine) {}

	CharPos Add(double addend, double lineHeight) const {
		CharPos ret(*this);
		ret._posInLine += lineHeight;
		while (ret._posInLine >= lineHeight) {
			ret._posInLine -= lineHeight;
			ret._lineIndex++;
		}
		return ret;
	}
	
};

class FairCopyField {
private:

	DevicePos _pos, _size;

	

public:

	FairCopyField(double x, double y, double w, double h);

	~FairCopyField();
};

}
}