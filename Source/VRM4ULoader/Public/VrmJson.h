#pragma once

#define RAPIDJSON_NAMESPACE vrm4u::local::rapid_json
#define RAPIDJSON_NAMESPACE_BEGIN namespace vrm4u { namespace local { namespace rapid_json {
#define RAPIDJSON_NAMESPACE_END } } }

#include <vector>
#include "rapidjson/document.h"

class VrmJson {

	bool bEnable = false;
public:
	RAPIDJSON_NAMESPACE::Document doc;

	bool validateSchema();
	bool init(const uint8_t* pData, size_t size);
	
	bool IsEnable() const{
		return bEnable;
	}
};
