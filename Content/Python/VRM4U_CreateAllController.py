# coding: utf-8
import unreal
import argparse

print("VRM4U python begin")
print (__file__)

parser = argparse.ArgumentParser()
parser.add_argument("-vrm")
parser.add_argument("-rig")
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

## meta 取得
a = reg.get_all_assets();
for aa in a:
    if (aa.get_editor_property("object_path") == args.vrm):
        v:unreal.VrmAssetListObject = aa
        vv = v.get_asset().vrm_meta_object
print(vv)
meta = vv

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


### 全ての骨
modelBoneListAll = []
modelBoneNameList = []

for e in reversed(h_mod.get_elements()):
    if (e.type != unreal.RigElementType.BONE):
        h_mod.remove_element(e)

name_to_control = {"dummy_for_table" : None}

gizmo_trans = unreal.Transform([0.0, 0.0, 0.0], [0.0, 0.0, 0.0], [0.2, 0.2, 0.2])

for e in h_mod.get_elements():
    print(e.name)
    if (e.type != unreal.RigElementType.BONE):
        continue
    bone = h_mod.get_bone(e)
    parentName = "{}".format(bone.get_editor_property('parent_name'))
    
    #print("parent==")
    #print(parentName)
    cur_parentName = parentName + "_c"
    name_s = "{}".format(e.name) + "_s"
    name_c = "{}".format(e.name) + "_c"
    space = h_mod.add_space(name_s, parent_name=cur_parentName, space_type=unreal.RigSpaceType.CONTROL)
    control = h_mod.add_control(name_c,
        space_name=space.name,
        gizmo_color=[0.1, 0.1, 1.0, 1.0],
        )

    h_mod.set_initial_global_transform(space, h_mod.get_global_transform(e))

    cc = h_mod.get_control(control)
    cc.set_editor_property('gizmo_transform', gizmo_trans)
    #cc.set_editor_property('control_type', unreal.RigControlType.ROTATOR)
    h_mod.set_control(cc)

    name_to_control[name_c] = cc

    c = None
    try:
        i = list(name_to_control.keys()).index(cur_parentName)
    except:
        i = -1

    if (i >= 0):
        c = name_to_control[cur_parentName]

    if (cc != None):
        if ("{}".format(e.name) in meta.humanoid_bone_table.values() ):
            cc.set_editor_property('gizmo_color', unreal.LinearColor(0.1, 0.1, 1.0, 1.0))
            h_mod.set_control(cc)
        else:
            cc.set_editor_property('gizmo_color', unreal.LinearColor(1.0, 0.1, 0.1, 1.0))
            h_mod.set_control(cc)


c = rig.controller
g = c.get_graph()
n = g.get_nodes()

# 配列ノード追加
collectionItem_forControl:unreal.RigVMStructNode = None
collectionItem_forBone:unreal.RigVMStructNode = None

for node in n:
    if (node.get_node_title() == 'Items'):
        #print(node.get_node_title())
        #node = unreal.RigUnit_CollectionItems.cast(node)
        pin = node.find_pin('Items')
        print(pin.get_array_size())
        print(pin.get_default_value())
        if (pin.get_array_size() < 40):
            continue

        if 'Type=Control' in pin.get_default_value():
            collectionItem_forControl = node

        if 'Type=Bone' in pin.get_default_value():
            collectionItem_forBone = node


#nn = unreal.EditorFilterLibrary.by_class(n,unreal.RigUnit_CollectionItems.static_class())


# controller array
if (collectionItem_forControl == None):
	collectionItem_forControl = c.add_struct_node(unreal.RigUnit_CollectionItems.static_struct(), method_name='Execute')
items_forControl = collectionItem_forControl.find_pin('Items')
c.clear_array_pin(items_forControl.get_pin_path())

if (collectionItem_forBone != None):
    items_forBone = collectionItem_forBone.find_pin('Items')
    c.clear_array_pin(items_forBone.get_pin_path())
    for bone_h in meta.humanoid_bone_table:
        bone_m = meta.humanoid_bone_table[bone_h]
        try:
            i = humanoidBoneList.index(bone_h.lower())
        except:
            i = -1
        if (i >= 0):
            tmp = '(Type=Bone,Name='
            tmp += "{}".format(bone_m).lower()
            tmp += ')'
            c.add_array_pin(items_forBone.get_pin_path(), default_value=tmp)
    
    
    
for e in h_mod.get_elements():
    print(e.name)
    if (e.type == unreal.RigElementType.CONTROL):
        tmp = '(Type=Control,Name='
        tmp += "{}".format(e.name)
        tmp += ')'
        c.add_array_pin(items_forControl.get_pin_path(), default_value=tmp)


