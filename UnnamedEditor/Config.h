#pragma once

namespace UnnamedEditor {


struct Config {
	Size windowSize;
	bool isVertical;
	int fontSize;
	
	Config(const TOMLReader& t) {
		isVertical = t[U"isVertical"].get<bool>();
		fontSize = t[U"fontSize"].get<int>();
		windowSize.x = t[U"windowSize.x"].get<int>();
		windowSize.y = t[U"windowSize.y"].get<int>();
	}
};


}