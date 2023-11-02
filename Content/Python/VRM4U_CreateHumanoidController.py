# coding: utf-8
import unreal

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-vrm")
parser.add_argument("-rig")
parser.add_argument("-meta")
args = parser.parse_args()
print(args.vrm)

#print(dummy[3])

humanoidBoneList = [
	"hips",
	"leftUpperLeg",
	"rightUpperLeg",
	"leftLowerLeg",
	"rightLowerLeg",
	"leftFoot",
	"rightFoot",
	"spine",
	"chest",
	"upperChest", # 9 optional
	"neck",
	"head",
	"leftShoulder",
	"rightShoulder",
	"leftUpperArm",
	"rightUpperArm",
	"leftLowerArm",
	"rightLowerArm",
	"leftHand",
	"rightHand",
	"leftToes",
	"rightToes",
	"leftEye",
	"rightEye",
	"jaw",
	"leftThumbProximal",	# 24
	"leftThumbIntermediate",
	"leftThumbDistal",
	"leftIndexProximal",
	"leftIndexIntermediate",
	"leftIndexDistal",
	"leftMiddleProximal",
	"leftMiddleIntermediate",
	"leftMiddleDistal",
	"leftRingProximal",
	"leftRingIntermediate",
	"leftRingDistal",
	"leftLittleProximal",
	"leftLittleIntermediate",
	"leftLittleDistal",
	"rightThumbProximal",
	"rightThumbIntermediate",
	"rightThumbDistal",
	"rightIndexProximal",
	"rightIndexIntermediate",
	"rightIndexDistal",
	"rightMiddleProximal",
	"rightMiddleIntermediate",
	"rightMiddleDistal",
	"rightRingProximal",
	"rightRingIntermediate",
	"rightRingDistal",
	"rightLittleProximal",
	"rightLittleIntermediate",
	"rightLittleDistal",	#54
]

humanoidBoneParentList = [
	"", #"hips",
	"hips",#"leftUpperLeg",
	"hips",#"rightUpperLeg",
	"leftUpperLeg",#"leftLowerLeg",
	"rightUpperLeg",#"rightLowerLeg",
	"leftLowerLeg",#"leftFoot",
	"rightLowerLeg",#"rightFoot",
	"hips",#"spine",
	"spine",#"chest",
	"chest",#"upperChest"	9 optional
	"chest",#"neck",
	"neck",#"head",
	"chest",#"leftShoulder",			# <-- upper..
	"chest",#"rightShoulder",
	"leftShoulder",#"leftUpperArm",
	"rightShoulder",#"rightUpperArm",
	"leftUpperArm",#"leftLowerArm",
	"rightUpperArm",#"rightLowerArm",
	"leftLowerArm",#"leftHand",
	"rightLowerArm",#"rightHand",
	"leftFoot",#"leftToes",
	"rightFoot",#"rightToes",
	"head",#"leftEye",
	"head",#"rightEye",
	"head",#"jaw",
	"leftHand",#"leftThumbProximal",
	"leftThumbProximal",#"leftThumbIntermediate",
	"leftThumbIntermediate",#"leftThumbDistal",
	"leftHand",#"leftIndexProximal",
	"leftIndexProximal",#"leftIndexIntermediate",
	"leftIndexIntermediate",#"leftIndexDistal",
	"leftHand",#"leftMiddleProximal",
	"leftMiddleProximal",#"leftMiddleIntermediate",
	"leftMiddleIntermediate",#"leftMiddleDistal",
	"leftHand",#"leftRingProximal",
	"leftRingProximal",#"leftRingIntermediate",
	"leftRingIntermediate",#"leftRingDistal",
	"leftHand",#"leftLittleProximal",
	"leftLittleProximal",#"leftLittleIntermediate",
	"leftLittleIntermediate",#"leftLittleDistal",
	"rightHand",#"rightThumbProximal",
	"rightThumbProximal",#"rightThumbIntermediate",
	"rightThumbIntermediate",#"rightThumbDistal",
	"rightHand",#"rightIndexProximal",
	"rightIndexProximal",#"rightIndexIntermediate",
	"rightIndexIntermediate",#"rightIndexDistal",
	"rightHand",#"rightMiddleProximal",
	"rightMiddleProximal",#"rightMiddleIntermediate",
	"rightMiddleIntermediate",#"rightMiddleDistal",
	"rightHand",#"rightRingProximal",
	"rightRingProximal",#"rightRingIntermediate",
	"rightRingIntermediate",#"rightRingDistal",
	"rightHand",#"rightLittleProximal",
	"rightLittleProximal",#"rightLittleIntermediate",
	"rightLittleIntermediate",#"rightLittleDistal",
]

for i in range(len(humanoidBoneList)):
    humanoidBoneList[i] = humanoidBoneList[i].lower()

for i in range(len(humanoidBoneParentList)):
    humanoidBoneParentList[i] = humanoidBoneParentList[i].lower()

######

reg = unreal.AssetRegistryHelpers.get_asset_registry();

##

rigs = unreal.ControlRigBlueprint.get_currently_open_rig_blueprints()
print(rigs)

for r in rigs:
    s:str = r.get_path_name()
    ss:str = args.rig
    if (s.find(ss) < 0):
        print("no rig")
    else:
        rig = r


#rig = rigs[10]
h_mod = rig.get_hierarchy_modifier()
#elements = h_mod.get_selection()
#sk = rig.get_preview_mesh()
#k = sk.skeleton
#print(k)

#print(sk.bone_tree)

#
#kk = unreal.RigElementKey(unreal.RigElementType.BONE, "hipssss");
#kk.name = ""
#kk.type = 1;

#print(h_mod.get_bone(kk))

#print(h_mod.get_elements())

### 全ての骨
modelBoneListAll = []
modelBoneNameList = []

#for e in reversed(h_mod.get_elements()):
#    if (e.type != unreal.RigElementType.BONE):
#        h_mod.remove_element(e)
    
for e in h_mod.get_elements():
    if (e.type == unreal.RigElementType.BONE):
        modelBoneListAll.append(e)
        modelBoneNameList.append("{}".format(e.name).lower())
#    else:
#        h_mod.remove_element(e)

print(modelBoneListAll[0])

#exit
#print(k.get_editor_property("bone_tree")[0].get_editor_property("translation_retargeting_mode"))
#print(k.get_editor_property("bone_tree")[0].get_editor_property("parent_index"))

#unreal.select

#vrmlist = unreal.VrmAssetListObject
#vrmmeta = vrmlist.vrm_meta_object
#print(vrmmeta.humanoid_bone_table)

#selected_actors = unreal.EditorLevelLibrary.get_selected_level_actors()
#selected_static_mesh_actors = unreal.EditorFilterLibrary.by_class(selected_actors,unreal.StaticMeshActor.static_class())

#static_meshes = np.array([])



## meta 取得
reg = unreal.AssetRegistryHelpers.get_asset_registry();
a = reg.get_all_assets();

if (args.meta):
    for aa in a:
        if (aa.get_editor_property("object_path") == args.meta):
            v:unreal.VrmMetaObject = aa
            vv = aa.get_asset()

if (vv == None):
    for aa in a:
        if (aa.get_editor_property("object_path") == args.vrm):
            v:unreal.VrmAssetListObject = aa
            vv = v.get_asset().vrm_meta_object
print(vv)
meta = vv



#v:unreal.VrmAssetListObject = None
#if (True):
#    a = reg.get_assets_by_path(args.vrm)
#    a = reg.get_all_assets();
#    for aa in a:
#        if (aa.get_editor_property("object_path") == args.vrm):
#            v:unreal.VrmAssetListObject = aa

#v = unreal.VrmAssetListObject.cast(v)
#print(v)

#unreal.VrmAssetListObject.vrm_meta_object

#meta = v.vrm_meta_object()
#meta = unreal.EditorFilterLibrary.by_class(asset,unreal.VrmMetaObject.static_class())


print (meta)
#print(meta[0].humanoid_bone_table)

### モデル骨のうち、ヒューマノイドと同じもの
### 上の変換テーブル
humanoidBoneToModel = {"" : ""}
humanoidBoneToModel.clear()

    
### modelBoneでループ
#for bone_h in meta.humanoid_bone_table:
for bone_h_base in humanoidBoneList:

    bone_h = None
    for e in meta.humanoid_bone_table:
        if ("{}".format(e).lower() == bone_h_base):
            bone_h = e;
            break;

    print("{}".format(bone_h))

    if (bone_h==None):
        continue

    bone_m = meta.humanoid_bone_table[bone_h]

    try:
        i = modelBoneNameList.index(bone_m.lower())
    except:
        i = -1
    if (i < 0):
        continue
    if ("{}".format(bone_h).lower() == "upperchest"):
        continue;

    humanoidBoneToModel["{}".format(bone_h).lower()] = "{}".format(bone_m).lower()

    if ("{}".format(bone_h).lower() == "chest"):
        #upperchestがあれば、これの次に追加
        bh = 'upperchest'
        print("upperchest: check begin")
        for e in meta.humanoid_bone_table:
            if ("{}".format(e).lower() != 'upperchest'):
                continue

            bm = "{}".format(meta.humanoid_bone_table[e]).lower()
            if (bm == ''):
                continue

            humanoidBoneToModel[bh] = bm
            humanoidBoneParentList[10] = "upperchest"
            humanoidBoneParentList[12] = "upperchest"
            humanoidBoneParentList[13] = "upperchest"

            print("upperchest: find and insert parent")
            break
        print("upperchest: check end")



parent=None
control_to_mat={None:None}

count = 0

### 骨名からControlへのテーブル
name_to_control = {"dummy_for_table" : None}

print("loop begin")

###### root
key = unreal.RigElementKey(unreal.RigElementType.SPACE, 'root_s')
space = h_mod.get_space(key)
if (space.get_editor_property('index') < 0):
    space = h_mod.add_space('root_s', space_type=unreal.RigSpaceType.SPACE)
else:
    space = key

key = unreal.RigElementKey(unreal.RigElementType.CONTROL, 'root_c')
control = h_mod.get_control(key)
if (control.get_editor_property('index') < 0):
    control = h_mod.add_control('root_c',
        space_name=space.name,
        gizmo_color=[1.0, 0.0, 0.0, 1.0],
        )
else:
    control = key
    h_mod.reparent_element(control, space)
parent = control

cc = h_mod.get_control(control)
cc.set_editor_property('gizmo_visible', False)
h_mod.set_control(cc)

for ee in humanoidBoneToModel:

    element = humanoidBoneToModel[ee]
    humanoidBone = ee
    modelBoneNameSmall = element

	# 対象の骨
    #modelBoneNameSmall = "{}".format(element.name).lower()
    #humanoidBone = modelBoneToHumanoid[modelBoneNameSmall];
    boneNo = humanoidBoneList.index(humanoidBone)
    print("{}_{}_{}__parent={}".format(modelBoneNameSmall, humanoidBone, boneNo,humanoidBoneParentList[boneNo]))

	# 親
    if count != 0:
        parent = name_to_control[humanoidBoneParentList[boneNo]]

	# 階層作成
    bIsNew = False
    name_s = "{}_s".format(humanoidBone)
    key = unreal.RigElementKey(unreal.RigElementType.SPACE, name_s)
    space = h_mod.get_space(key)
    if (space.get_editor_property('index') < 0):
        space = h_mod.add_space(name_s, space_type=unreal.RigSpaceType.SPACE)
        bIsNew = True
    else:
        space = key

    name_c = "{}_c".format(humanoidBone)
    key = unreal.RigElementKey(unreal.RigElementType.CONTROL, name_c)
    control = h_mod.get_control(key)
    if (control.get_editor_property('index') < 0):
        control = h_mod.add_control(name_c,
            space_name=space.name,
            gizmo_color=[1.0, 0.0, 0.0, 1.0],
            )
        #h_mod.get_control(control).gizmo_transform = gizmo_trans
        if (24<=boneNo & boneNo<=53):
            gizmo_trans = unreal.Transform([0.0, 0.0, 0.0], [0.0, 0.0, 0.0], [0.1, 0.1, 0.1])
        else:
            gizmo_trans = unreal.Transform([0.0, 0.0, 0.0], [0.0, 0.0, 0.0], [1, 1, 1])

        if (17<=boneNo & boneNo<=18):
            gizmo_trans = unreal.Transform([0.0, 0.0, 0.0], [0.0, 0.0, 0.0], [1, 1, 1])
        cc = h_mod.get_control(control)
        cc.set_editor_property('gizmo_transform', gizmo_trans)
        cc.set_editor_property('control_type', unreal.RigControlType.ROTATOR)
        h_mod.set_control(cc)
        #h_mod.set_control_value_transform(control,gizmo_trans)
        bIsNew = True
    else:
        control = key
        if (bIsNew == True):
            h_mod.reparent_element(control, space)


    # テーブル登録
    name_to_control[humanoidBone] = control
    print(humanoidBone)


    # ロケータ 座標更新
    # 不要な上層階層を考慮
	
    gTransform = h_mod.get_global_transform(modelBoneListAll[modelBoneNameList.index(modelBoneNameSmall)])
    if count == 0:
        bone_initial_transform = gTransform
    else:
        #bone_initial_transform = h_mod.get_initial_transform(element)
        bone_initial_transform = gTransform.multiply(control_to_mat[parent].inverse())

    h_mod.set_initial_transform(space, bone_initial_transform)
    control_to_mat[control] = gTransform

    # 階層修正
    h_mod.reparent_element(space, parent)
    
    count += 1
    if (count >= 500):
        break
