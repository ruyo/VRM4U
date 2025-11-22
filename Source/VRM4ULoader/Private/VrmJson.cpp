// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "VrmJson.h"
#include "VrmConvert.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#include "rapidjson/schema.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

bool VRMIsValid(const uint8_t* pData, size_t size) {
	VRMConverter j;
	if (j.Init(pData, size, nullptr)) {
		return j.ValidateSchema();
	}
	return false;
}


bool VRMIsVRM10(RAPIDJSON_NAMESPACE::Document &doc) {
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

bool VRMIsVRM10(const uint8_t* pData, size_t size) {
	VRMConverter j;
	if (j.Init(pData, size, nullptr) == false) {
		return false;
	}
	RAPIDJSON_NAMESPACE::Document& doc = j.jsonData.doc;
	//doc.Parse(&v[0]);

	return VRMIsVRM10(doc);
}

FString GetPluginThirdpartyPath() {
	std::string ret;
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("VRM4U");
	if (!Plugin.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Plugin not found!"));
		return FString();
	}
	return Plugin->GetBaseDir() / TEXT("Thirdparty");
}

std::string ReadTextFileFromThirdparty(const FString& RelativePath)
{
	std::string ret;

	FString TextFilePath = GetPluginThirdpartyPath() / RelativePath;

	// ファイルの存在確認
	if (!FPaths::FileExists(TextFilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("VRM4U_Schema: no file: %s"), *TextFilePath);
		return ret;
	}

	// テキストファイルを読み込む
	FString FileContent;
	TArray<uint8> data;
	//if (FFileHelper::LoadFileToString(FileContent, *ThirdpartyPath))
	if (FFileHelper::LoadFileToArray(data, *TextFilePath))
	{
		UE_LOG(LogTemp, Log, TEXT("VRM4U_Schema: read: %s"), *TextFilePath);
		return std::string(reinterpret_cast<const char*>(data.GetData()), data.Num());
	} else
	{
		UE_LOG(LogTemp, Error, TEXT("VRM4U_Schema: failed: %s"), *TextFilePath);
		return ret;
	}
}

// Schema Provider ($ref をファイルパスで解決)
class FileSchemaProvider : public RAPIDJSON_NAMESPACE::IRemoteSchemaDocumentProvider {
public:
	FileSchemaProvider(const std::string& baseDir){
		BaseDir_ = FString(UTF8_TO_TCHAR(baseDir.c_str()));
	}

	const RAPIDJSON_NAMESPACE::SchemaDocument* GetRemoteDocument(const char* uri, RAPIDJSON_NAMESPACE::SizeType length) override {
		// file name (ex: "VRM.meta.schema.json")
		FString UriStr(UTF8_TO_TCHAR(uri));
		FString SchemaFileName = FPaths::GetCleanFilename(UriStr);

		// path (ex: /Game/Schemas/vrm0/VRM.meta.schema.json)
		FString FilePath = FPaths::Combine(BaseDir_, SchemaFileName);

		auto* CachedSchema = SchemaCache_.Find(FilePath);
		if (CachedSchema) {
			return *CachedSchema;
		}

		TArray<uint8> FileContent;
		if (FPaths::FileExists(FilePath)) {
			FFileHelper::LoadFileToArray(FileContent, *FilePath);
		}
		if (FileContent.Num() == 0){
			if (FilePath.Contains(TEXT("/glTF"))) {
				UE_LOG(LogTemp, Log, TEXT("VRM4U_Schema: Failed to load schema: %s"), *FilePath);
			} else {
				UE_LOG(LogTemp, Warning, TEXT("VRM4U_Schema: Failed to load schema: %s"), *FilePath);
			}
			return nullptr;
		}

		// to string
		std::string SchemaStr(reinterpret_cast<const char*>(FileContent.GetData()), FileContent.Num());


		// parse
		auto Doc = std::make_unique<RAPIDJSON_NAMESPACE::Document>();
		if (Doc->Parse(SchemaStr.c_str()).HasParseError()) {
			UE_LOG(LogTemp, Error, TEXT("VRM4U_Schema: parse error in %s: %s"), *FilePath,
				UTF8_TO_TCHAR(RAPIDJSON_NAMESPACE::GetParseError_En(Doc->GetParseError())));
			return nullptr;
		}

		// SchemaDocument
		auto Schema = std::make_unique<RAPIDJSON_NAMESPACE::SchemaDocument>(*Doc, uri, length, this);
		auto* Ptr = Schema.get();
		SchemaCache_.Add(FilePath, Ptr);
		DocCache_.Add(FilePath, std::move(Doc));
		SchemaDocCache_.Add(FilePath, std::move(Schema));
		return Ptr;
	}

private:
	FString BaseDir_;
	TMap<FString, RAPIDJSON_NAMESPACE::SchemaDocument*> SchemaCache_;
	TMap<FString, std::unique_ptr<RAPIDJSON_NAMESPACE::Document>> DocCache_;
	TMap<FString, std::unique_ptr<RAPIDJSON_NAMESPACE::SchemaDocument>> SchemaDocCache_;
};


// VRM1
bool validateSchemaVRM1_internal(RAPIDJSON_NAMESPACE::Document& jsonDoc, const std::string& path, const std::string& fileExt) {

	// ext check
	// extなしなら通過
	if (!jsonDoc.HasMember("extensions") || !jsonDoc["extensions"].HasMember(fileExt.c_str())) {
		UE_LOG(LogTemp, Log, TEXT("VRM4U_Schema: no extensions/%s"), UTF8_TO_TCHAR(fileExt.c_str()));
		// ここは成功扱い
		return true;
	}

	std::string filename = fileExt + ".schema.json";
	std::string schemaStr = ReadTextFileFromThirdparty(FString(UTF8_TO_TCHAR((path + "/" + filename).c_str())));

	// parse
	RAPIDJSON_NAMESPACE::Document schemaDoc;
	if (schemaDoc.Parse(schemaStr.c_str()).HasParseError()) {
		UE_LOG(LogTemp, Error, TEXT("VRM4U_Schema: validation failed at: %s"), UTF8_TO_TCHAR(RAPIDJSON_NAMESPACE::GetParseError_En(jsonDoc.GetParseError())));
		//std::cerr << "Schema parse error: " << RAPIDJSON_NAMESPACE::GetParseError_En(schemaDoc.GetParseError())
		//	<< " at offset " << schemaDoc.GetErrorOffset() << std::endl;
		return false;
	}

	FileSchemaProvider provider(std::string(TCHAR_TO_UTF8(*(GetPluginThirdpartyPath() / UTF8_TO_TCHAR(path.c_str())))));
	RAPIDJSON_NAMESPACE::SchemaDocument schema(schemaDoc, nullptr, 0, &provider);
	RAPIDJSON_NAMESPACE::SchemaValidator validator(schema);
	RAPIDJSON_NAMESPACE::Value& vrmExtension = jsonDoc["extensions"][fileExt.c_str()];
	if (!vrmExtension.Accept(validator)) {
		RAPIDJSON_NAMESPACE::StringBuffer sb;
		validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
		UE_LOG(LogTemp, Error, TEXT("VRM4U_Schema: Schema validation failed at: %s"), UTF8_TO_TCHAR(sb.GetString()));
		UE_LOG(LogTemp, Error, TEXT("VRM4U_Schema: nvalid keyword: %s"), UTF8_TO_TCHAR(validator.GetInvalidSchemaKeyword()));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("VRM4U_Schema: valid %s"), UTF8_TO_TCHAR(fileExt.c_str()));
	return true;
}
bool validateSchemaVRM1(RAPIDJSON_NAMESPACE::Document& jsonDoc) {

	bool ret = true;
	ret &= validateSchemaVRM1_internal(jsonDoc, "vrm_specification/vrm1/VRMC_vrm-1.0/schema", "VRMC_vrm");
	ret &= validateSchemaVRM1_internal(jsonDoc, "vrm_specification/vrm1/VRMC_springBone-1.0/schema", "VRMC_springBone");
	ret &= validateSchemaVRM1_internal(jsonDoc, "vrm_specification/vrm1/VRMC_node_constraint-1.0/schema", "VRMC_node_constraint");
	ret &= validateSchemaVRM1_internal(jsonDoc, "vrm_specification/vrm1/VRMC_vrm_animation-1.0/schema", "VRMC_vrm_animation");

	return ret;
}


// VRM0
bool validateSchemaVRM0_internal(RAPIDJSON_NAMESPACE::Document &jsonDoc, const std::string& path, const std::string& filename) {

	std::string schemaStr = ReadTextFileFromThirdparty(FString(UTF8_TO_TCHAR((path + "/" + filename).c_str())));

	// parse
	RAPIDJSON_NAMESPACE::Document schemaDoc;
	if (schemaDoc.Parse(schemaStr.c_str()).HasParseError()) {
		UE_LOG(LogTemp, Error, TEXT("VRM4U_Schema: validation failed at: %s"), UTF8_TO_TCHAR(RAPIDJSON_NAMESPACE::GetParseError_En(jsonDoc.GetParseError())));
		//std::cerr << "Schema parse error: " << RAPIDJSON_NAMESPACE::GetParseError_En(schemaDoc.GetParseError())
		//	<< " at offset " << schemaDoc.GetErrorOffset() << std::endl;
		return false;
	}

	// ext
	if (!jsonDoc.HasMember("extensions") || !jsonDoc["extensions"].HasMember("VRM")) {
		UE_LOG(LogTemp, Error, TEXT("VRM4U_Schema: no extensions/vrm"));
		//std::cerr << "Missing VRMC_vrm extension in JSON" << std::endl;
		return false;
	}

	FileSchemaProvider provider(std::string( TCHAR_TO_UTF8(*(GetPluginThirdpartyPath() / UTF8_TO_TCHAR(path.c_str())))) );
	RAPIDJSON_NAMESPACE::SchemaDocument schema(schemaDoc, nullptr, 0, &provider);
	RAPIDJSON_NAMESPACE::SchemaValidator validator(schema);
	RAPIDJSON_NAMESPACE::Value& vrmExtension = jsonDoc["extensions"]["VRM"];
	if (!vrmExtension.Accept(validator)) {
		// エラー詳細を出力
		RAPIDJSON_NAMESPACE::StringBuffer sb;
		validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
		UE_LOG(LogTemp, Error, TEXT("VRM4U_Schema: Schema validation failed at: %s"), UTF8_TO_TCHAR(sb.GetString()));
		UE_LOG(LogTemp, Error, TEXT("VRM4U_Schema: nvalid keyword: %s"), UTF8_TO_TCHAR(validator.GetInvalidSchemaKeyword()));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("VRM4U_Schema: valid /%s"), UTF8_TO_TCHAR(filename.c_str()));
	return true;
}
bool validateSchemaVRM0(RAPIDJSON_NAMESPACE::Document& jsonDoc) {

	return validateSchemaVRM0_internal(jsonDoc, "vrm_specification/vrm0/schema", "vrm.schema.json");
}


bool VrmJson::validateSchema() {
	bEnable = false;

	std::vector<char> s;
	//std::string ss = ReadTextFileFromThirdparty(TEXT("vrm_specification/vrm0/schema/vrm.schema.json"));
	//std::string ss = ReadTextFileFromThirdparty(TEXT("vrm_specification/vrm1/VRMC_vrm-1.0/schema/VRMC_vrm.schema.json"));

	if (VRMIsVRM10(doc)) {
		if (validateSchemaVRM1(doc)) {
			// vrm1 ok
			bEnable = true;
		} else {
			// error
			UE_LOG(LogTemp, Warning, TEXT("VRM4U_Schema: VRM1 validation failed"));
		}
	} else {
		if (validateSchemaVRM0(doc)) {
			bEnable = true;
			// vrm0 ok
		} else {
			// error
			UE_LOG(LogTemp, Warning, TEXT("VRM4U_Schema: VRM0 validation failed"));
		}
	}

	if (bEnable == false) {
		doc.Clear();
	}
	return bEnable;
}

bool VrmJson::init(const uint8_t* pData, size_t size) {
	bEnable = false;

	std::vector<char> v;
	v.resize(size);
	memcpy(&v[0], pData, size);
	v.push_back(0);


	if (doc.Parse(&v[0]).HasParseError()) {
		UE_LOG(LogTemp, Warning, TEXT("VRM4U_Schema: parse error: %s"), UTF8_TO_TCHAR(RAPIDJSON_NAMESPACE::GetParseError_En(doc.GetParseError())));
		//std::cerr << "Schema parse error: " << RAPIDJSON_NAMESPACE::GetParseError_En(schemaDoc.GetParseError())
		//	<< " at offset " << schemaDoc.GetErrorOffset() << std::endl;
		return false;
	}

	bEnable = true;


	return true;
}

