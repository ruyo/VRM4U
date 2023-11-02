#pragma once

#include <map>
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <stdexcept>

#pragma pack(1)

#ifdef __cplusplus
extern "C" {
#endif

    namespace VRM {
        typedef float(vec3)[3];
        typedef float(vec4)[4];

        struct ASSIMP_API VRMBlendShapeBind {
            aiString blendShapeName;
            aiString nodeName;
            aiString meshName;
            int meshID=0;
            int shapeIndex=0;
            int weight=100;
        };

        struct ASSIMP_API VRMBlendShapeGroup {
            aiString groupName;

            int bindNum=0;
            VRMBlendShapeBind *bind = nullptr;

            ~VRMBlendShapeGroup() {
                if (bind) {
                    delete[] bind;
                    bind = nullptr;
                }
            }

            void CopyFrom(const VRMBlendShapeGroup &src){
				groupName = src.groupName;

                bindNum = src.bindNum;
				bind = new VRMBlendShapeBind[bindNum];
                for (int i=0; i<bindNum; ++i){
					bind[i] = src.bind[i];
                }
            }
        };

        struct ASSIMP_API VRMHumanoid {
            aiString humanBoneName;
            aiString nodeName;
        };

        // physics
        struct ASSIMP_API VRMSpring {
            float stiffness = 0.f;
            float gravityPower = 0.f;
            vec3 gravityDir = { 0,0,0 };
            float dragForce = 0.f;
            float hitRadius = 0.f;

            int boneNum = 0;
            int *bones = nullptr;
            aiString *bones_name = nullptr;

            int colliderGourpNum = 0;
            int* colliderGroups = nullptr;

            ~VRMSpring() {
                if (bones_name) {
                    delete[] bones_name;
                    bones_name = nullptr;
                }
                if (colliderGroups) {
                    delete[] colliderGroups;
                    colliderGroups = nullptr;
                }
            }

            void CopyFrom(const VRMSpring &src){
                stiffness = src.stiffness;
				gravityPower = src.gravityPower;
				gravityDir[0] = src.gravityDir[0];
				gravityDir[1] = src.gravityDir[1];
				gravityDir[2] = src.gravityDir[2];
				dragForce = src.dragForce;
				hitRadius = src.hitRadius;

                boneNum = src.boneNum;
				bones = new int[boneNum];
				bones_name = new aiString[boneNum];
                for (int i=0; i<boneNum; ++i){
					bones[i] = src.bones[i];
					bones_name[i] = src.bones_name[i];
                }

                colliderGourpNum = src.colliderGourpNum;
				colliderGroups = new int[colliderGourpNum];
                for (int i=0; i<colliderGourpNum; ++i){
					colliderGroups[i] = src.colliderGroups[i];
                }
            }
        };
        struct ASSIMP_API VRMCollider {
            vec3 offset = { 0,0,0 };
            float radius = 0.f;
        };
        struct ASSIMP_API VRMColliderGroup {
            int node = 0;
            aiString node_name;

            int colliderNum = 0;
            VRMCollider *colliders = nullptr;

            ~VRMColliderGroup() {
                if (colliders) {
                    delete[] colliders;
                    colliders = nullptr;
                }
            }

            void CopyFrom(const VRMColliderGroup &src){
				node = src.node;
				node_name = src.node_name;

                colliderNum = src.colliderNum;
				colliders = new VRMCollider[colliderNum];
                for (int i=0; i<colliderNum; ++i){
					colliders[i] = src.colliders[i];
                }
            }
        };
        // physics

        // material
        struct ASSIMP_API VRMMaterialFloatProperties {
            float _Cutoff;
            float _BumpScale;
            float _ReceiveShadowRate;
            float _ShadeShift;
            float _ShadeToony;
            float _LightColorAttenuation;
            float _IndirectLightIntensity;
            float _RimLightingMix;
            float _RimFresnelPower;
            float _RimLift;
            float _OutlineWidth;
            float _OutlineScaledMaxDistance;
            float _OutlineLightingMix;
            float _UvAnimScrollX;
            float _UvAnimScrollY;
            float _UvAnimRotation;
            float _MToonVersion;
            float _DebugMode;
            float _BlendMode;
            float _OutlineWidthMode;
            float _OutlineColorMode;
            float _CullMode;
            float _OutlineCullMode;
            float _SrcBlend;
            float _DstBlend;
            float _ZWrite;
        };
        struct ASSIMP_API VRMMaterialVectorProperties {
            vec4 _Color = {1,1,1,1};
            vec4 _ShadeColor = {1,1,1,1};
            vec4 _MainTex;
            vec4 _ShadeTexture;
            vec4 _BumpMap;
            vec4 _ReceiveShadowTexture;
            vec4 _ShadingGradeTexture;
            vec4 _RimColor;
            vec4 _RimTexture;
            vec4 _SphereAdd;
            vec4 _EmissionColor;
            vec4 _EmissionMap;
            vec4 _OutlineWidthTexture;
            vec4 _OutlineColor;
            vec4 _UvAnimMaskTexture;
        };
        struct ASSIMP_API VRMMaterialTextureProperties {
            int _MainTex;
            int _ShadeTexture;
            int _BumpMap;
            int _ReceiveShadowTexture;
            int _ShadingGradeTexture;
            int _RimTexture;
            int _SphereAdd;
            int _EmissionMap;
            int _OutlineWidthTexture;
            int _UvAnimMaskTexture;
        };
        struct ASSIMP_API VRMMaterial {
            aiString name;
            aiString shaderName;
            VRMMaterialFloatProperties floatProperties;
            VRMMaterialVectorProperties vectorProperties;
            VRMMaterialTextureProperties textureProperties;

            void CopyFrom(const VRMMaterial &src){
				name = src.name;
				shaderName = src.shaderName;
				floatProperties = src.floatProperties;
				vectorProperties = src.vectorProperties;
				textureProperties = src.textureProperties;
            }
        };
        // material

        // license
        enum VRMLicenseList{
            LIC_version,
            LIC_author,
            LIC_contactInformation,
            LIC_reference,
            LIC_title,
            LIC_texture,
            LIC_allowedUserName,
            LIC_violentUssageName,
            LIC_sexualUssageName,
            LIC_commercialUssageName,
            LIC_otherPermissionUrl,
            LIC_licenseName,
            LIC_otherLicenseUrl,

            LIC_futter,
            LIC_max,
        };

        struct ASSIMP_API VRMLicensePair {
            aiString Key;
            aiString Value;
        };

        struct ASSIMP_API VRMLicense
        {
            int licensePairNum = 0;
            VRMLicensePair *licensePair = nullptr;

            ~VRMLicense() {
                if (licensePair) {
                    delete[] licensePair;
                    licensePair = nullptr;
                }
            }

            void CopyFrom(const VRMLicense &src) {
				licensePairNum = src.licensePairNum;
				licensePair = new VRMLicensePair[licensePairNum];
                for (int i=0; i<licensePairNum; ++i){
					licensePair[i] = src.licensePair[i];
                }
            }
        };
        // license end

        struct ASSIMP_API VRMMetadata
        {
            VRMLicense license;

            int springNum = 0;
            VRMSpring *springs = nullptr;

            int colliderGroupNum = 0;
            VRMColliderGroup *colliderGroups = nullptr;

            VRMHumanoid humanoidBone[55];

            int blendShapeGroupNum = 0;
            VRMBlendShapeGroup *blendShapeGroup = nullptr;

            int materialNum = 0;
            VRMMaterial *material = nullptr;

            VRMMetadata() {
            }

            ~VRMMetadata() {
                if (springs) {
                    delete[] springs;
                    springs = nullptr;
                }
                if (colliderGroups) {
                    delete[] colliderGroups;
                    colliderGroups = nullptr;
                }
                if (blendShapeGroup) {
                    delete[] blendShapeGroup;
                    blendShapeGroup = nullptr;
                }
                if (material) {
                    delete[] material;
                    material = nullptr;
                }
            }

            VRMMetadata *CreateClone(){
				auto *p = new VRMMetadata();

                p->license.CopyFrom(license);

                p->springNum = springNum;
				p->springs = new VRMSpring[springNum];
                for (int i=0;i<springNum; ++i){
					p->springs[i].CopyFrom(springs[i]);
                }

                p->colliderGroupNum = colliderGroupNum;
				p->colliderGroups = new VRMColliderGroup[colliderGroupNum];
                for (int i=0; i<colliderGroupNum; ++i){
					p->colliderGroups[i].CopyFrom(colliderGroups[i]);
				}

                for (int i=0; i<sizeof(humanoidBone)/sizeof(humanoidBone[0]); ++i){
					p->humanoidBone[i] = humanoidBone[i];
                }

				p->blendShapeGroupNum = blendShapeGroupNum;
                p->blendShapeGroup = new VRMBlendShapeGroup[blendShapeGroupNum];
                for (int i = 0; i < blendShapeGroupNum; ++i) {
                    p->blendShapeGroup[i].CopyFrom(blendShapeGroup[i]);
				}


				p->materialNum = materialNum;
				p->material = new VRMMaterial[materialNum];
				for (int i = 0; i < materialNum; ++i) {
					p->material[i].CopyFrom(material[i]);
				}


                return p;
            }

        };

        void ReleaseVRMMeta(VRMMetadata *&meta);
    }

#ifdef __cplusplus
}
#endif

#pragma pack()

