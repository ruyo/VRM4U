// VRM4U Copyright (c) 2021-2023 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmJson.h"

bool VrmJson::init(const uint8_t* pData, size_t size) {

	int c = 0;
	size_t c_start = 0;
	for (; c_start < size; ++c_start) {
		if (pData[c_start] == '{') {
			break;
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

	doc.Parse(&v[0]);

	return true;
}


void VrmJsonTest(const uint8_t* pData, size_t size) {
}

bool VRMIsVRM10(const uint8_t* pData, size_t size) {

	int c = 0;
	size_t c_start = 0;
	for (; c_start < size; ++c_start) {
		if (pData[c_start] == '{') {
			break;
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
	}
	return false;





	/*
	std::string s = j["extensions"]["VRM"]["exporterVersion"];
	auto s2 = j["extensions"]["VRM"]["meta"]["title"].get <std::string>();
	auto s3 = UTF8_TO_TCHAR(s2.c_str());

  auto v1 = json["hoge"]["fuga"][1];
  auto v2 = json["/hoge/fuga/1"_json_pointer];
  auto key = nlohmann::json::json_pointer("/hoge/fuga/1");
  auto v3 = json[key];

	// add a number that is stored as double (note the implicit conversion of j to an object)
	j["pi"] = 3.141;

	// add a Boolean that is stored as bool
	j["happy"] = true;

	// add a string that is stored as std::string
	j["name"] = "Niels";

	// add another null object by passing nullptr
	j["nothing"] = nullptr;

	// add an object inside the object
	j["answer"]["everything"] = 42;
	*/
}