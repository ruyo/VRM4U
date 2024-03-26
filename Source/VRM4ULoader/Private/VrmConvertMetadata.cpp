// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmConvertMetadata.h"
#include "VrmConvert.h"


#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "VrmLicenseObject.h"

#if	UE_VERSION_OLDER_THAN(4,26,0)
#include "AssetRegistryModule.h"
#else
#include "AssetRegistry/AssetRegistryModule.h"
#endif
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

#include "VrmJson.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>
#include <assimp/vrm/vrmmeta.h>


bool VRMConverter::Init(const uint8* pFileData, size_t dataSize, const aiScene *pScene) {
	jsonData.init(pFileData, dataSize);
	aiData = pScene;
	return true;
}


static UVrmLicenseObject *tmpLicense = nullptr;
UVrmLicenseObject* VRMConverter::GetVRMMeta(const aiScene *mScenePtr) {
	tmpLicense = nullptr;
	VRMConverter::ConvertVrmMeta(nullptr, mScenePtr, nullptr, 0);

	return tmpLicense;
}

bool VRMConverter::ConvertVrmFirst(UVrmAssetListObject* vrmAssetList, const uint8* pData, size_t dataSize) {

	// material
	if (VRMConverter::Options::Get().IsVRM10Model()) {
		// mtoon params
		vrmAssetList->MaterialHasMToon.Empty();
		for (auto& mat : jsonData.doc["materials"].GetArray()) {
			bool b = false;
			if (mat.HasMember("extensions")) {
				if (mat["extensions"].HasMember("VRMC_materials_mtoon")) {
					b = true;
				}
			}
			vrmAssetList->MaterialHasMToon.Add(b);
		}
	} else {
		// alpha cutoff flag
		vrmAssetList->MaterialHasAlphaCutoff.Empty();
		for (auto& mat : jsonData.doc["materials"].GetArray()) {
			bool b = false;
			if (mat.HasMember("alphaCutoff")) {
				b = true;
			}
			vrmAssetList->MaterialHasAlphaCutoff.Add(b);
		}
	}

	return true;
}


bool VRMConverter::ConvertVrmMeta(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr, const uint8* pData, size_t dataSize) {

	tmpLicense = nullptr;
	VRM::VRMMetadata *SceneMeta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

	UVrmMetaObject *MetaObject = nullptr;
	UVrmLicenseObject *lic = nullptr;

	{
		UPackage *package = GetTransientPackage();

		if (vrmAssetList) {
			package = vrmAssetList->Package;
		}

		if (package == GetTransientPackage() || vrmAssetList==nullptr) {
			MetaObject = VRM4U_NewObject<UVrmMetaObject>(package, NAME_None, EObjectFlags::RF_Public | RF_Transient, NULL);
			lic = VRM4U_NewObject<UVrmLicenseObject>(package, NAME_None, EObjectFlags::RF_Public | RF_Transient, NULL);
		} else {

			if (vrmAssetList->ReimportBase) {
				MetaObject = vrmAssetList->ReimportBase->VrmMetaObject;
				lic = vrmAssetList->ReimportBase->VrmLicenseObject;
			}
			if (MetaObject == nullptr) {
				MetaObject = VRM4U_NewObject<UVrmMetaObject>(package, *(FString(TEXT("VM_")) + vrmAssetList->BaseFileName + TEXT("_VrmMeta")), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
			}
			if (lic == nullptr){
				lic = VRM4U_NewObject<UVrmLicenseObject>(package, *(FString(TEXT("VL_")) + vrmAssetList->BaseFileName + TEXT("_VrmLicense")), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
			}
		}
		if (MetaObject) MetaObject->MarkPackageDirty();
		if (lic) lic->MarkPackageDirty();

		if (vrmAssetList) {
			vrmAssetList->VrmMetaObject = MetaObject;
			vrmAssetList->VrmLicenseObject = lic;
			MetaObject->VrmAssetListObject = vrmAssetList;
		} else {
			tmpLicense = lic;
		}
	}

	if (SceneMeta == nullptr) {
		return false;
	}

	if (VRMConverter::Options::Get().IsVRM10Model()) {
		MetaObject->Version = 1;
	}

	// bone
	if (VRMConverter::Options::Get().IsVRM10Model()){
		// VRM10
		if (pData && dataSize) {
			auto &humanBone = jsonData.doc["extensions"]["VRMC_vrm"]["humanoid"]["humanBones"];
			auto &origBone = jsonData.doc["nodes"];

			for (auto& g : humanBone.GetObject()) {
				int node = g.value["node"].GetInt();

				if (node >= 0 && node < (int)origBone.Size()) {
					FString str = UTF8_TO_TCHAR(origBone[node]["name"].GetString());

					if (VRMConverter::Options::Get().IsForceOriginalBoneName()) {
					}else{
						str = VRMUtil::MakeName(str, true);
					}

					MetaObject->humanoidBoneTable.Add(UTF8_TO_TCHAR(g.name.GetString())) = str;
				} else {
					MetaObject->humanoidBoneTable.Add(UTF8_TO_TCHAR(g.name.GetString())) = "";
				}
			}
		}
	} else {
		for (auto& a : SceneMeta->humanoidBone) {
			if (FString(a.humanBoneName.C_Str()) == "") {
				continue;
			}
			FString str = UTF8_TO_TCHAR(a.nodeName.C_Str());
			if (VRMConverter::Options::Get().IsForceOriginalBoneName()) {
			} else {
				str = VRMUtil::MakeName(str, true);
			}
			MetaObject->humanoidBoneTable.Add(UTF8_TO_TCHAR(a.humanBoneName.C_Str())) = str;
		}
	}

	//shape
	if (VRMConverter::Options::Get().IsVRM10Model()) {
		// VRM10
		auto &presets = jsonData.doc["extensions"]["VRMC_vrm"]["expressions"]["preset"];

		MetaObject->BlendShapeGroup.SetNum(presets.Size());
		int presetIndex = -1;
		for (auto& presetData : presets.GetObject()){
			++presetIndex;

			MetaObject->BlendShapeGroup[presetIndex].name = UTF8_TO_TCHAR(presetData.name.GetString()); // ex happy

			auto &bindData = presetData.value["morphTargetBinds"];

			MetaObject->BlendShapeGroup[presetIndex].BlendShape.SetNum(bindData.Size());

			MetaObject->BlendShapeGroup[presetIndex].isBinary = presetData.value["isBinary"].GetBool();
			MetaObject->BlendShapeGroup[presetIndex].overrideBlink = presetData.value["overrideBlink"].GetString();
			MetaObject->BlendShapeGroup[presetIndex].overrideLookAt = presetData.value["overrideLookAt"].GetString();
			MetaObject->BlendShapeGroup[presetIndex].overrideMouth = presetData.value["overrideMouth"].GetString();

			int bindIndex = -1;
			for (auto &bind : bindData.GetArray()) {
				++bindIndex;

				//m->BlendShapeGroup[presetIndex].BlendShape[bindIndex].morphTargetName = UTF8_TO_TCHAR(aiGroup.bind[b].blendShapeName.C_Str());
				auto &targetShape = MetaObject->BlendShapeGroup[presetIndex].BlendShape[bindIndex];
				//targetShape.morphTargetName = UTF8_TO_TCHAR(aiGroup.bind[b].blendShapeName.C_Str());
				//targetShape.meshName = UTF8_TO_TCHAR(aiGroup.bind[b].meshName.C_Str());
				//targetShape.nodeName = UTF8_TO_TCHAR(aiGroup.bind[b].nodeName.C_Str());
				//targetShape.weight = aiGroup.bind[b].weight;
				targetShape.shapeIndex = bind["index"].GetInt();

				{
					int tmpNodeID = bind["node"].GetInt(); // adjust offset
					int tmpMeshID = jsonData.doc["nodes"].GetArray()[tmpNodeID]["mesh"].GetInt();

					//meshID offset
					int offset = 0;
					for (int meshNo = 0; meshNo < tmpMeshID; ++meshNo) {
						if (jsonData.doc["meshes"].GetArray()[meshNo].HasMember("primitives") == false) continue;
						offset += jsonData.doc["meshes"].GetArray()[meshNo]["primitives"].Size() - 1;
					}
					targetShape.meshID = tmpMeshID + offset;
				}

				if (targetShape.meshID < (int)jsonData.doc["meshes"].Size()) {
					{
						auto& targetNames = jsonData.doc["meshes"].GetArray()[targetShape.meshID]["extras"]["targetNames"];
						if (targetShape.shapeIndex < (int)targetNames.Size()) {
							targetShape.morphTargetName = UTF8_TO_TCHAR(targetNames.GetArray()[targetShape.shapeIndex].GetString());
						}
					}

					{
						auto& tmp = jsonData.doc["meshes"][targetShape.meshID]["primitives"]["extras"]["targetNames"];
						if (targetShape.shapeIndex < (int)tmp.Size()) {
							targetShape.morphTargetName = tmp[targetShape.shapeIndex].GetString();

							if (VRMConverter::Options::Get().IsForceOriginalMorphTargetName()) {
							}else{
								targetShape.morphTargetName = VRMUtil::MakeName(targetShape.morphTargetName);
							}
						}
					}
					targetShape.meshName = UTF8_TO_TCHAR(jsonData.doc["meshes"].GetArray()[targetShape.meshID]["name"].GetString());
				}
			}
		}
	} else {
		MetaObject->BlendShapeGroup.SetNum(SceneMeta->blendShapeGroupNum);
		for (int i = 0; i < SceneMeta->blendShapeGroupNum; ++i) {
			auto& aiGroup = SceneMeta->blendShapeGroup[i];

			FString s = UTF8_TO_TCHAR(aiGroup.groupName.C_Str());
			if (VRMConverter::Options::Get().IsRemoveBlendShapeGroupPrefix()) {
				int32 ind = 0;
				if (s.FindLastChar('.', ind)) {
					if (ind < s.Len() - 1) {
						s = s.RightChop(ind+1);
					}
				}
			}
			MetaObject->BlendShapeGroup[i].name = s;

			MetaObject->BlendShapeGroup[i].BlendShape.SetNum(aiGroup.bindNum);
			for (int b = 0; b < aiGroup.bindNum; ++b) {
				auto& bind = MetaObject->BlendShapeGroup[i].BlendShape[b];
				bind.morphTargetName = UTF8_TO_TCHAR(aiGroup.bind[b].blendShapeName.C_Str());
				bind.meshName = UTF8_TO_TCHAR(aiGroup.bind[b].meshName.C_Str());
				bind.nodeName = UTF8_TO_TCHAR(aiGroup.bind[b].nodeName.C_Str());
				bind.weight = aiGroup.bind[b].weight;
				bind.meshID = aiGroup.bind[b].meshID;
				bind.shapeIndex = aiGroup.bind[b].shapeIndex;

				if (VRMConverter::Options::Get().IsForceOriginalMorphTargetName()) {
				}else{
					bind.morphTargetName = VRMUtil::MakeName(bind.morphTargetName);
				}
			}
		}
	}

	// tmp shape...
	if (pData && vrmAssetList) {
		if (VRMConverter::Options::Get().IsVRM10Model()) {
		} else {
			TMap<FString, FString> ParamTable;

			ParamTable.Add("_Color", "mtoon_Color");
			ParamTable.Add("_RimColor", "mtoon_RimColor");
			ParamTable.Add("_EmisionColor", "mtoon_EmissionColor");
			ParamTable.Add("_OutlineColor", "mtoon_OutColor");

			auto& group = jsonData.doc["extensions"]["VRM"]["blendShapeMaster"]["blendShapeGroups"];
			for (int i = 0; i < (int)group.Size(); ++i) {
				auto& bind = MetaObject->BlendShapeGroup[i];

				auto& shape = group[i];
				if (shape.HasMember("materialValues") == false) {
					continue;
				}
				for (auto& mat : shape["materialValues"].GetArray()) {
					FVrmBlendShapeMaterialList mlist;
					mlist.materialName = mat["materialName"].GetString();
					mlist.propertyName = mat["propertyName"].GetString();

					FString *tmp = vrmAssetList->MaterialNameOrigToAsset.Find(NormalizeFileName(mlist.materialName));
					if (tmp == nullptr) {
						continue;
					}
					mlist.materialName = *tmp;
					if (ParamTable.Find(mlist.propertyName)) {
						mlist.propertyName = ParamTable[mlist.propertyName];
					}
					mlist.color = FLinearColor(
						mat["targetValue"].GetArray()[0].GetFloat(),
						mat["targetValue"].GetArray()[1].GetFloat(),
						mat["targetValue"].GetArray()[2].GetFloat(),
						mat["targetValue"].GetArray()[3].GetFloat());
					bind.MaterialList.Add(mlist);
				}
			}
		}
	}

	if (VRMConverter::Options::Get().IsVRM10Model()) {
		// vrm1 collider
		{
			{
				auto& collider = jsonData.doc["extensions"]["VRMC_springBone"]["colliders"];

				MetaObject->VRMColliderMeta.SetNum(collider.Size());
				for (int colliderNo = 0; colliderNo < (int)collider.Size(); ++colliderNo) {
					auto& dstCollider = MetaObject->VRMColliderMeta[colliderNo];
					dstCollider.bone = collider.GetArray()[colliderNo]["node"].GetInt();
					//dstCollider.boneName

					if (collider.GetArray()[colliderNo]["shape"].HasMember("sphere")) {
						dstCollider.collider.SetNum(1);
						dstCollider.collider[0].offset.Set(
							collider.GetArray()[colliderNo]["shape"]["sphere"]["offset"][0].GetFloat(),
							collider.GetArray()[colliderNo]["shape"]["sphere"]["offset"][1].GetFloat(),
							collider.GetArray()[colliderNo]["shape"]["sphere"]["offset"][2].GetFloat());
						dstCollider.collider[0].radius = collider.GetArray()[colliderNo]["shape"]["sphere"]["radius"].GetFloat();
						dstCollider.collider[0].shapeType = TEXT("sphere");

					}

					if (collider.GetArray()[colliderNo]["shape"].HasMember("capsule")) {
						dstCollider.collider.SetNum(1);
						dstCollider.collider[0].offset.Set(
							collider.GetArray()[colliderNo]["shape"]["capsule"]["offset"][0].GetFloat(),
							collider.GetArray()[colliderNo]["shape"]["capsule"]["offset"][1].GetFloat(),
							collider.GetArray()[colliderNo]["shape"]["capsule"]["offset"][2].GetFloat());
						dstCollider.collider[0].radius = collider.GetArray()[colliderNo]["shape"]["capsule"]["radius"].GetFloat();
						dstCollider.collider[0].tail.Set(
							collider.GetArray()[colliderNo]["shape"]["capsule"]["tail"][0].GetFloat(),
							collider.GetArray()[colliderNo]["shape"]["capsule"]["tail"][1].GetFloat(),
							collider.GetArray()[colliderNo]["shape"]["capsule"]["tail"][2].GetFloat());
						dstCollider.collider[0].shapeType = TEXT("capsule");
					}
				}
			}
			{
				auto& colliderGroup = jsonData.doc["extensions"]["VRMC_springBone"]["colliderGroups"]["colliderGroups"];
				auto& dstCollider = MetaObject->VRMColliderGroupMeta;

				dstCollider.SetNum(colliderGroup.Size());
				for (int i = 0; i < dstCollider.Num(); ++i) {
					auto c = colliderGroup.GetArray();
					dstCollider[i].groupName = c[i]["name"].GetString();

					for (uint32_t g = 0; g < c[i]["colliders"].Size(); ++g) {
						dstCollider[i].colliderGroup.Add(c[i]["colliders"].GetArray()[g].GetInt());
					}
				
				}


			}
		}

		// vrm1 spring
		{
			auto& jsonSpring = jsonData.doc["extensions"]["VRMC_springBone"]["springs"];

			auto& sMeta = MetaObject->VRM1SpringBoneMeta.Springs;
			sMeta.SetNum(jsonSpring.Size());
			for (uint32 springNo = 0; springNo < jsonSpring.Size(); ++springNo) {
				auto& jsonJoints = jsonSpring.GetArray()[springNo]["joints"];

				auto& dstSpring = sMeta[springNo];
				dstSpring.joints.SetNum(jsonJoints.Size());
				for (uint32 jointNo = 0; jointNo < jsonJoints.Size(); ++jointNo) {
					auto& jj = jsonJoints.GetArray()[jointNo];

					auto& s = dstSpring.joints[jointNo];

					s.dragForce = jj["dragForce"].GetFloat();
					s.gravityPower = jj["gravityPower"].GetFloat();
					if (jj.HasMember("gravityDir")) {
						if (jj["gravityDir"].GetArray().Size() == 3) {
							s.gravityDir.X = jj["gravityDir"][0].GetFloat();
							s.gravityDir.Y = jj["gravityDir"][1].GetFloat();
							s.gravityDir.Z = jj["gravityDir"][2].GetFloat();
						}
					}
					int node = jj["node"].GetInt();
					s.boneNo = node;

					{
						auto& jsonNode = jsonData.doc["nodes"];
						if (node >= 0 && node < (int)jsonNode.Size()) {
							s.boneName = VRMUtil::GetSafeNewName(UTF8_TO_TCHAR(jsonNode[node]["name"].GetString()));
						}
					}

					s.hitRadius = jj["hitRadius"].GetFloat();
					s.stiffness = jj["stiffness"].GetFloat();

					/*
							"node": 0,
							"hitRadius": 0.1,
							"stiffness": 0.5,
							"dragForce": 0.5,

					s.stiffness = vrms.stiffness;
					s.gravityPower = vrms.gravityPower;
					s.gravityDir.Set(vrms.gravityDir[0], vrms.gravityDir[1], vrms.gravityDir[2]);
					s.dragForce = vrms.dragForce;
					s.hitRadius = vrms.hitRadius;

					s.bones.SetNum(vrms.boneNum);
					s.boneNames.SetNum(vrms.boneNum);
					for (int b = 0; b < vrms.boneNum; ++b) {
						s.bones[b] = vrms.bones[b];
						s.boneNames[b] = UTF8_TO_TCHAR(vrms.bones_name[b].C_Str());
					}


					s.ColliderIndexArray.SetNum(vrms.colliderGourpNum);
					for (int c = 0; c < vrms.colliderGourpNum; ++c) {
						s.ColliderIndexArray[c] = vrms.colliderGroups[c];
					}
					*/

				}
			}
		}

	} else {
		// spring
		MetaObject->VRMSpringMeta.SetNum(SceneMeta->springNum);
		for (int i = 0; i < SceneMeta->springNum; ++i) {
			const auto& vrms = SceneMeta->springs[i];

			auto& s = MetaObject->VRMSpringMeta[i];
			s.stiffness = vrms.stiffness;
			s.gravityPower = vrms.gravityPower;
			s.gravityDir.Set(vrms.gravityDir[0], vrms.gravityDir[1], vrms.gravityDir[2]);
			s.dragForce = vrms.dragForce;
			s.hitRadius = vrms.hitRadius;

			s.bones.SetNum(vrms.boneNum);
			s.boneNames.SetNum(vrms.boneNum);
			for (int b = 0; b < vrms.boneNum; ++b) {
				s.bones[b] = vrms.bones[b];
				s.boneNames[b] = UTF8_TO_TCHAR(vrms.bones_name[b].C_Str());
			}


			s.ColliderIndexArray.SetNum(vrms.colliderGourpNum);
			for (int c = 0; c < vrms.colliderGourpNum; ++c) {
				s.ColliderIndexArray[c] = vrms.colliderGroups[c];
			}
		}

		//collider
		MetaObject->VRMColliderMeta.SetNum(SceneMeta->colliderGroupNum);
		for (int i = 0; i < SceneMeta->colliderGroupNum; ++i) {
			const auto& vrmc = SceneMeta->colliderGroups[i];

			auto& c = MetaObject->VRMColliderMeta[i];
			c.bone = vrmc.node;
			c.boneName = UTF8_TO_TCHAR(vrmc.node_name.C_Str());

			c.collider.SetNum(vrmc.colliderNum);
			for (int b = 0; b < vrmc.colliderNum; ++b) {
				c.collider[b].offset = FVector(vrmc.colliders[b].offset[0], vrmc.colliders[b].offset[1], vrmc.colliders[b].offset[2]);
				c.collider[b].radius = vrmc.colliders[b].radius;
			}
		}
	}

	//constraint
	if (VRMConverter::Options::Get().IsVRM10Model()) {
		// VRM10
		auto& nodes = jsonData.doc["nodes"];
		for (auto& node : nodes.GetArray()) {
			if (node.HasMember("extensions") == false) continue;
			if (node["extensions"].HasMember("VRMC_node_constraint") == false) continue;
			if (node["extensions"]["VRMC_node_constraint"].HasMember("constraint") == false) continue;


			auto& constraint = node["extensions"]["VRMC_node_constraint"]["constraint"];
			if (constraint.HasMember("roll")) {
				FVRMConstraintRoll c;

				c.source = constraint["roll"]["source"].GetInt();
				if (c.source < (int)nodes.Size()) {
					c.sourceName = nodes.GetArray()[c.source]["name"].GetString();
				}
				c.rollAxis = constraint["roll"]["rollAxis"].GetString();
				c.weight = constraint["roll"]["weight"].GetFloat();

				FVRMConstraint cc;
				cc.constraintRoll = c;
				cc.type = EVRMConstraintType::Roll;

				MetaObject->VRMConstraintMeta.Add(node["name"].GetString(), cc);
			}
			if (constraint.HasMember("aim")) {
				FVRMConstraintAim c;

				c.source = constraint["aim"]["source"].GetInt();
				if (c.source < (int)nodes.Size()) {
					c.sourceName = nodes.GetArray()[c.source]["name"].GetString();
				}
				c.aimAxis = constraint["aim"]["aimAxis"].GetString();
				c.weight = constraint["aim"]["weight"].GetFloat();

				FVRMConstraint cc;
				cc.constraintAim = c;
				cc.type = EVRMConstraintType::Aim;

				MetaObject->VRMConstraintMeta.Add(node["name"].GetString(), cc);
			}
			if (constraint.HasMember("rotation")) {
				FVRMConstraintRotation c;

				c.source = constraint["rotation"]["source"].GetInt();
				if (c.source < (int)nodes.Size()) {
					c.sourceName = nodes.GetArray()[c.source]["name"].GetString();
				}
				c.weight = constraint["rotation"]["weight"].GetFloat();

				FVRMConstraint cc;
				cc.constraintRotation = c;
				cc.type = EVRMConstraintType::Rotation;

				MetaObject->VRMConstraintMeta.Add(node["name"].GetString(), cc);
			}
		}
	}

	// vrma
	if (VRMConverter::Options::Get().IsVRMAModel()) {
		if (pData && dataSize) {
			auto& humanBone = jsonData.doc["extensions"]["VRMC_vrm_animation"]["humanoid"]["humanBones"];
			auto& origBone = jsonData.doc["nodes"];

			for (auto& g : humanBone.GetObject()) {
				int nodeNo = g.value["node"].GetInt();

				if (FString(g.name.GetString()) == "") {
					continue;
				}

				if (nodeNo >= 0 && nodeNo < (int)origBone.Size()) {
					MetaObject->humanoidBoneTable.Add(UTF8_TO_TCHAR(g.name.GetString())) = UTF8_TO_TCHAR(origBone[nodeNo]["name"].GetString());
				}
				else {
					MetaObject->humanoidBoneTable.Add(UTF8_TO_TCHAR(g.name.GetString())) = "";
				}
			}
		}
	}


	// license
	{
		struct TT {
			FString key;
			FString &dst;
		};
		const TT table[] = {
			{TEXT("version"),		lic->version},
			{TEXT("author"),			lic->author},
			{TEXT("contactInformation"),	lic->contactInformation},
			{TEXT("reference"),		lic->reference},
				// texture skip
			{TEXT("title"),			lic->title},
			{TEXT("allowedUserName"),	lic->allowedUserName},
			{TEXT("violentUsageName"),	lic->violentUsageName},
			{TEXT("sexualUsageName"),	lic->sexualUsageName},
			{TEXT("commercialUsageName"),	lic->commercialUsageName},
			{TEXT("otherPermissionUrl"),		lic->otherPermissionUrl},
			{TEXT("licenseName"),			lic->licenseName},
			{TEXT("otherLicenseUrl"),		lic->otherLicenseUrl},

			{TEXT("violentUssageName"),	lic->violentUsageName},
			{TEXT("sexualUssageName"),	lic->sexualUsageName},
			{TEXT("commercialUssageName"),	lic->commercialUsageName},
		};
		for (int i = 0; i < SceneMeta->license.licensePairNum; ++i) {

			auto &p = SceneMeta->license.licensePair[i];

			for (auto &t : table) {
				if (t.key == p.Key.C_Str()) {
					t.dst = UTF8_TO_TCHAR(p.Value.C_Str());
				}
			}
			if (vrmAssetList) {
				if (FString(TEXT("texture")) == p.Key.C_Str()) {
					int t = FCString::Atoi(*FString(p.Value.C_Str()));
					if (t >= 0 && t < vrmAssetList->Textures.Num()) {
						lic->thumbnail = vrmAssetList->Textures[t];
					}
				}
			}
		}
	}

	return true;
}

bool VRMConverter::ConvertVrmMetaRenamed(UVrmAssetListObject* vrmAssetList, const aiScene* mScenePtr, const uint8* pData, size_t dataSize) {
	if (VRMConverter::Options::Get().IsGenerateHumanoidRenamedMesh()) {
		UPackage* package = GetTransientPackage();

		if (vrmAssetList) {
			package = vrmAssetList->Package;
		}

		UVrmMetaObject* m = vrmAssetList->VrmMetaObject;

		//m2 = VRM4U_NewObject<UVrmMetaObject>(package, *(FString(TEXT("VM_")) + vrmAssetList->BaseFileName + TEXT("_VrmMeta")), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		{
			TMap<FString, FString> BoneTable;

			UVrmMetaObject* m2 = VRM4U_DuplicateObject<UVrmMetaObject>(m, package, *(FString(TEXT("VM_")) + vrmAssetList->BaseFileName + TEXT("_ue4mannequin_VrmMeta")));
			vrmAssetList->VrmMannequinMetaObject = m2;
			for (auto& a : m2->humanoidBoneTable) {
				auto c = VRMUtil::table_ue4_vrm.FindByPredicate([a](const VRMUtil::VRMBoneTable& data) {
					// meta humanoid key == utiltable vrmbone
					return a.Key.ToLower() == data.BoneVRM.ToLower();
				});
				if (c) {
					if (a.Value != "") {
						BoneTable.Add(a.Value, c->BoneUE4);
						a.Value = c->BoneUE4;
					}
				}
			}
			for (auto& a : m2->VRMColliderMeta) {
				auto c = BoneTable.Find(a.boneName);
				if (c) {
					if (a.boneName != "") {
						a.boneName = *c;
					}
				}
			}
			for (auto& a : m2->VRMSpringMeta) {
				for (auto& b : a.boneNames) {
					auto c = BoneTable.Find(b);
					//auto c = VRMUtil::table_ue4_vrm.FindByPredicate([b](const VRMUtil::VRMBoneTable& data) { return b.ToLower() == data.BoneVRM.ToLower(); });
					if (c) {
						if (b != "") {
							b = *c;
						}
					}
				}
			}
		}

		{
			TMap<FString, FString> BoneTable;

			UVrmMetaObject* m3 = VRM4U_DuplicateObject<UVrmMetaObject>(m, package, *(FString(TEXT("VM_")) + vrmAssetList->BaseFileName + TEXT("_humanoid_VrmMeta")));
			vrmAssetList->VrmHumanoidMetaObject = m3;
			for (auto& a : m3->humanoidBoneTable) {
				if (a.Value != "") {
					BoneTable.Add(a.Value, a.Key);
					a.Value = a.Key;
				}
			}
			for (auto& a : m3->VRMColliderMeta) {
				auto c = BoneTable.Find(a.boneName);
				if (c) {
					if (a.boneName != "") {
						a.boneName = *c;
					}
				}
			}
			for (auto& a : m3->VRMSpringMeta) {
				for (auto& b : a.boneNames) {
					auto c = BoneTable.Find(b);
					//auto c = VRMUtil::table_ue4_vrm.FindByPredicate([b](const VRMUtil::VRMBoneTable& data) { return b.ToLower() == data.BoneVRM.ToLower(); });
					if (c) {
						if (b != "") {
							b = *c;
						}
					}
				}
			}
		}
	}
	return true;
}


VrmConvertMetadata::VrmConvertMetadata()
{
}

VrmConvertMetadata::~VrmConvertMetadata()
{
}
