// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmJson.h"

bool VrmJson::init(const uint8_t* pData, size_t size) {
	bEnable = false;

	if (size < 4 || pData == nullptr) {
		return false;
	}

	int c = 0;
	size_t c_start = 0;
	{
		std::vector<char> str = { 'J','S','O','N' };

		for (; c_start < size - str.size(); ++c_start) {
			bool bFound = false;

			for (int i = 0; i < 4; ++i) {
				if (str[i] != pData[c_start + i]) {
					continue;
				}
				bFound = true;
			}
			if (bFound) {
				c_start += 4;
				break;
			}
		}
	}

	size_t c_end = c_start;
	for (; c_end < size; ++c_end) {
		if (pData[c_end] == '{') {
			c++;
		}
		if (pData[c_end] == '}') {
			c--;
		}
		if (c == 0) {
			++c_end;
			break;
		}
	}

	if (c_end - c_start <=1) {
		return false;
	}
	std::vector<char> v;
	v.resize(c_end - c_start);
	memcpy(&v[0], pData + c_start, c_end - c_start);
	v.push_back(0);

	doc.Parse(&v[0]);

	bEnable = true;
	return true;
}

bool VRMIsVRM10(const uint8_t* pData, size_t size) {

	if (size < 4 || pData == nullptr) {
		return false;
	}

	int c = 0;
	size_t c_start = 0;
	{
		std::vector<char> str = { 'J','S','O','N' };

		for (; c_start < size - str.size(); ++c_start) {
			bool bFound = false;

			for (int i = 0; i < 4; ++i) {
				if (str[i] != pData[c_start + i]) {
					continue;
				}
				bFound = true;
			}
			if (bFound) {
				c_start += 4;
				break;
			}
		}
	}

	size_t c_end = c_start;
	for (; c_end < size; ++c_end) {
		if (pData[c_end] == '{') {
			c++;
		}
		if (pData[c_end] == '}') {
			c--;
		}
		if (c == 0) {
			++c_end;
			break;
		}
	}

	std::vector<char> v;
	v.resize(c_end - c_start);
	memcpy(&v[0], pData + c_start, c_end - c_start);
	v.push_back(0);

	RAPIDJSON_NAMESPACE::Document doc;
	doc.Parse(&v[0]);

	if (doc.HasMember("extensions")) {
		if (doc["extensions"].HasMember("VRMC_vrm")) {
			return true;
		}
		if (doc["extensions"].HasMember("VRMC_vrm_animation")) {
			return true;
		}
	}
	return false;
}