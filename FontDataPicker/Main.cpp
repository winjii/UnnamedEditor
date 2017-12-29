#include <iostream>
#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OPENTYPE_VALIDATE_H
#include "GsubReader.h"

int main() {
	using namespace FontDataPicker;

	FT_Library lib;
	int error = FT_Init_FreeType(&lib);
	std::cout << error << std::endl;

	FT_Face face;
	error = FT_New_Face(lib, "SourceHanSerif-Regular.otf", 0, &face);
	std::cout << error << std::endl;

	FT_Bytes baseTable, gdefTable, gposTable, gsubTable, jstfTable;
	error = FT_OpenType_Validate(face, FT_VALIDATE_GSUB, &baseTable, &gdefTable, &gposTable, &gsubTable, &jstfTable);
	std::cout << error << std::endl;

	using uint16 = unsigned short;
	auto toUint16 = [](FT_Bytes p) {
		return ((uint16)p[0] << 8) + (uint16)p[1];
	};
	uint16 featureOffset = toUint16(gsubTable + 6);
	FT_Bytes featureList = gsubTable + featureOffset;
	uint16 featureCount = toUint16(featureList + 0);
	FT_Bytes featureRecords = featureList + 2;

	uint16 lookupOffset = toUint16(gsubTable + 8);
	FT_Bytes lookupList = gsubTable + lookupOffset;
	uint16 lookupCount = toUint16(lookupList + 0);
	FT_Bytes lookups = lookupList + 2;

	std::cout << std::endl << "Feature List" << std::endl;
	for (int i = 0; i < featureCount; i++) {
		FT_Bytes featureRecord = featureRecords + 6*i;
		const std::string tag = {
			(char)featureRecord[0],
			(char)featureRecord[1],
			(char)featureRecord[2],
			(char)featureRecord[3] };
		std::cout << tag << std::endl;
	}

	FT_Select_Charmap(face, FT_Encoding::FT_ENCODING_UNICODE);
	GlyphIndex input = (GlyphIndex)FT_Get_Char_Index(face, L'B');

	GsubReader gsubReader(face);
	GlyphIndex output = gsubReader.vertSubstitute(input);
	std::cout << std::endl << input << " -> " << output << std::endl;
	return 0;
}