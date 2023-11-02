# coding: utf-8
import unreal

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-vrm")
parser.add_argument("-rig")
parser.add_argument("-meta")
args = parser.parse_args()
#print(args.vrm)

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
	"upperChest",
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
	"leftThumbProximal",
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
	"rightLittleDistal",
]

for i in range(len(humanoidBoneList)):
    humanoidBoneList[i] = humanoidBoneList[i].lower()


######

rigs = unreal.ControlRigBlueprint.get_currently_open_rig_blueprints()
#print(rigs)

for r in rigs:
    s:str = r.get_path_name()
    ss:str = args.rig
    if (s.find(ss) < 0):
        print("no rig")
    else:
        rig = r




print(unreal.SystemLibrary.get_engine_version())
if (unreal.SystemLibrary.get_engine_version()[0] == '5'):
    c = rig.get_controller()#rig.controller
else:
    c = rig.controller

g = c.get_graph()
n = g.get_nodes()
print(n)

#c.add_branch_node()
#c.add_array_pin()

a:unreal.RigUnit_CollectionItems = unreal.RigUnit_CollectionItems()

# print(a)

# 配列ノード追加
collectionItem_forControl:unreal.RigVMStructNode = None
collectionItem_forBone:unreal.RigVMStructNode = None

for node in n:
    if (node.get_node_title() == 'Items' or node.get_node_title() == 'Collection from Items'):

        #print(node.get_node_title())
        #node = unreal.RigUnit_CollectionItems.cast(node)
        pin = node.find_pin('Items')
        print(pin.get_array_size())
        print(pin.get_default_value())
        if (pin.get_array_size() < 40):
            continue

        if 'Type=Bone' in pin.get_default_value():
            collectionItem_forBone= node
        if 'Type=Control' in pin.get_default_value():
            collectionItem_forControl = node


#nn = unreal.EditorFilterLibrary.by_class(n,unreal.RigUnit_CollectionItems.static_class())

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
#print(vv)
meta = vv


# controller array
if (collectionItem_forControl == None):
	collectionItem_forControl = c.add_struct_node(unreal.RigUnit_CollectionItems.static_struct(), method_name='Execute')
items_forControl = collectionItem_forControl.find_pin('Items')
c.clear_array_pin(items_forControl.get_pin_path())

# bone array
if (collectionItem_forBone == None):
	collectionItem_forBone = c.add_struct_node(unreal.RigUnit_CollectionItems.static_struct(), method_name='Execute')
items_forBone = collectionItem_forBone.find_pin('Items')
c.clear_array_pin(items_forBone.get_pin_path())

## h_mod
rigs = unreal.ControlRigBlueprint.get_currently_open_rig_blueprints()
rig = rigs[0]

print(items_forControl)
print(items_forBone)

humanoidBoneTable = {"dummy" : "dummy"}
humanoidBoneTable.clear()

for h in meta.humanoid_bone_table:
    bone_h = "{}".format(h).lower()
    bone_m = "{}".format(meta.humanoid_bone_table[h]).lower()
    try:
        i = list(humanoidBoneTable.values()).index(bone_m)
    except:
        i = -1
    if (bone_h!="" and bone_m!="" and i==-1):
        humanoidBoneTable[bone_h] = bone_m

for bone_h in humanoidBoneList:
    bone_m = humanoidBoneTable.get(bone_h, None)
    if bone_m == None:
        continue
    
#for bone_h in meta.humanoid_bone_table:
#    bone_m = meta.humanoid_bone_table[bone_h]
#    try:
#        i = humanoidBoneList.index(bone_h.lower())
#    except:
#        i = -1
#    if (i >= 0):
    if (True):
        tmp = '(Type=Bone,Name='
        #tmp += "{}".format(bone_m).lower()
        tmp += bone_m
        tmp += ')'
        c.add_array_pin(items_forBone.get_pin_path(), default_value=tmp)
        #print(bone_m)

        tmp = '(Type=Control,Name='
        #tmp += "{}".format(bone_h).lower() + '_c'
        tmp += bone_h + '_c'
        tmp += ')'
        #print(c)
        c.add_array_pin(items_forControl.get_pin_path(), default_value=tmp)
        #print(bone_h)



#for e in h_mod.get_elements():
#    if (e.type == unreal.RigElementType.CONTROL):
#        tmp = '(Type=Control,Name='
#        tmp += "{}".format(e.name)
#        tmp += ')'
#        c.add_array_pin(items_forControl.get_pin_path(), default_value=tmp)
#        print(e.name)
#    if (e.type == unreal.RigElementType.BONE):
#        tmp = '(Type=Bone,Name='
#        tmp += "{}".format(e.name)
#        tmp += ')'
#        c.add_array_pin(items_forBone.get_pin_path(), default_value=tmp)
#        print(e.name)
    

#print(i.get_all_pins_recursively())

#ii:unreal.RigUnit_CollectionItems = n[1]

#pp = ii.get_editor_property('Items')

#print(pp)


#print(collectionItem.get_all_pins_recursively()[0])

#i.get_editor_property("Items")

#c.add_array_pin("Execute")

# arrayを伸ばす
#i.get_all_pins_recursively()[0].get_pin_path()
#c.add_array_pin(i.get_all_pins_recursively()[0].get_pin_path(), default_value='(Type=Bone,Name=Global)')


#rig = rigs[10]
