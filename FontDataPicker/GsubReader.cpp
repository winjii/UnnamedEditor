#include "GsubReader.h"
#include <string>
#include <vector>

namespace FontDataPicker {


GsubReader::GsubReader(FT_Face face) : _face(face) {
	auto toUint16 = [](FT_Bytes p) {
		return ((uint16)p[0] << 8) + (uint16)p[1];
	};

	FT_Bytes baseTable, gdefTable, gposTable, gsubTable, jstfTable;
	int error = FT_OpenType_Validate(face, FT_VALIDATE_GSUB, &baseTable, &gdefTable, &gposTable, &gsubTable, &jstfTable);
	uint16 featureOffset = toUint16(gsubTable + 6);
	FT_Bytes featureList = gsubTable + featureOffset;
	uint16 featureCount = toUint16(featureList + 0);
	FT_Bytes featureRecords = featureList + 2;

	uint16 lookupOffset = toUint16(gsubTable + 8);
	FT_Bytes lookupList = gsubTable + lookupOffset;
	uint16 lookupCount = toUint16(lookupList + 0);
	FT_Bytes lookups = lookupList + 2;

	//Coverage�e�[�u�����w�肵�Ă���O���t�C���f�b�N�X��ǂݎ��
	auto coverageToIndices = [toUint16](FT_Bytes coverage) {
		std::vector<GlyphIndex> ret;
		uint16 format = toUint16(coverage + 0);
		if (format == 1) {
			uint16 count = toUint16(coverage + 2);
			FT_Bytes indices = coverage + 4;
			for (int i = 0; i < count; i++) {
				ret.push_back(toUint16(indices + 2*i));
			}
		}
		else if (format == 2) {
			uint16 count = toUint16(coverage + 2);
			FT_Bytes rangeRecords = coverage + 4;
			for (int i = 0; i < count; i++) {
				FT_Bytes rangeRecord = rangeRecords + 6*i;
				GlyphIndex startIndex = toUint16(rangeRecord + 0);
				GlyphIndex endIndex = toUint16(rangeRecord + 2);
				for (GlyphIndex j = startIndex; j <= endIndex; j++) {
					ret.push_back(j);
				}
			}
		}
		return ret;
	};

	//Lookup�e�[�u����ǂ�ŁA_vertSubstitution���\�z
	auto readLookup = [&](FT_Bytes lookup) {
		uint16 lookupType = toUint16(lookup + 0);
		if (lookupType != 1) return; //�P�ƒu��(type == 1)�̂ݏ���
		uint16 subTableCount = toUint16(lookup + 4);
		FT_Bytes subTables = lookup + 6;
		for (int i = 0; i < subTableCount; i++) {
			FT_Bytes subTable = lookup + toUint16(subTables + 2*i);
			uint16 sbstFormat = toUint16(subTable + 0);
			if (sbstFormat == 1) {
				FT_Bytes coverage = subTable + toUint16(subTable + 2);
				int16 deltaGlyphID = (int16)toUint16(subTable + 4);
				std::vector<uint16> indices = coverageToIndices(coverage);
				for (int i = 0; i < indices.size(); i++) {
					int outputIndex = (int)indices[i] + deltaGlyphID;
					if (outputIndex < 0) outputIndex += 65536;
					else if (outputIndex >= 65536) outputIndex -= 65536;
					_vertSubstitution[indices[i]] = (uint16)outputIndex;
				}
			}
			else if (sbstFormat == 2) {
				FT_Bytes coverage = subTable + toUint16(subTable + 2);
				//uint16 count = toUint16(subTable + 4); //indices.size()�Ɠ����͂��Ȃ̂Ŏg��Ȃ�
				FT_Bytes substitutes = subTable + 6;
				std::vector<uint16> indices = coverageToIndices(coverage);
				for (int i = 0; i < indices.size(); i++) {
					uint16 outputIndex = toUint16(substitutes + 2*i);
					_vertSubstitution[indices[i]] = outputIndex;
				}
			}
		}
	};

	//vert�t�B�[�`���[(�c�����p�O���t�u���̋@�\)�������āA�����̒u������ǂ݂ɍs��
	//�����ɂ́A�g�����������̌n(Script)�ƌ���̌n(Language)��I��������őΉ�����feature������ǂނׂ��B���������ƌĂяo������Script��Language��I������K�v���o�Ă��ĕ��G�ɂȂ�̂ŁA�Ƃ肠����vert�Ƃ���feature��S���ǂ�ł���B�c�����ւ̃O���t�u���������p���Ȃ��Ȃ�傫�Ȗ��͂Ȃ��Ǝv����
	for (int i = 0; i < featureCount; i++) {
		FT_Bytes featureRecord = featureRecords + 6*i;
		const std::string tag = {
			(char)featureRecord[0],
			(char)featureRecord[1],
			(char)featureRecord[2],
			(char)featureRecord[3] };
		if (tag != "vert") continue;
		FT_Bytes feature = featureList + toUint16(featureRecord + 4);
		uint16 featureLookupCount = toUint16(feature + 2);
		FT_Bytes lookupIndices = feature + 4;
		for (int j = 0; j < featureLookupCount; j++) {
			uint16 lookupIndex = ((uint16)lookupIndices[2*j]) + lookupIndices[2*j + 1];
			uint16 lookupTableOffset = ((uint16)lookups[2*lookupIndex] << 8) + (uint16)lookups[2*lookupIndex + 1];
			FT_Bytes lookup = lookupList + lookupTableOffset;
			readLookup(lookup);
		}
	}
}

GlyphIndex GsubReader::vertSubstitute(GlyphIndex gid) {
	if (_vertSubstitution.find(gid) == _vertSubstitution.end())
		return gid;
	return _vertSubstitution[gid];
}


}