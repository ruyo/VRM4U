# coding: utf-8
import unreal
import argparse

print("VRM4U python begin")
print (__file__)

parser = argparse.ArgumentParser()
parser.add_argument("-rigSrc")
parser.add_argument("-rigDst")
args = parser.parse_args()
#print(args.vrm)

######

## rig 取得
reg = unreal.AssetRegistryHelpers.get_asset_registry();
a = reg.get_all_assets();
for aa in a:
    if (aa.get_editor_property("object_path") == args.rigSrc):
        rigSrc = aa.get_asset()
    if (aa.get_editor_property("object_path") == args.rigDst):
        rigDst = aa.get_asset()
print(rigSrc)
print(rigDst)

modSrc = rigSrc.get_hierarchy_modifier() 
modDst = rigDst.get_hierarchy_modifier() 

conSrc = rigSrc.controller
conDst = rigDst.controller

graSrc = conSrc.get_graph()
graDst = conDst.get_graph()

modDst.initialize()


print(rig[3])

meta = vv



rigs = unreal.ControlRigBlueprint.get_currently_open_rig_blueprints()
rig = rigs[0]

h_mod = rig.get_hierarchy_modifier()

elements = h_mod.get_selection()


c = rig.controller
g = c.get_graph()
n = g.get_nodes()
print(n)

#c.add_branch_node()
#c.add_array_pin()

a:unreal.RigUnit_CollectionItems = unreal.RigUnit_CollectionItems()

print(a)

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

        if 'Type=Bone' in pin.get_default_value():
            collectionItem_forBone= node
        if 'Type=Control' in pin.get_default_value():
            collectionItem_forControl = node


#nn = unreal.EditorFilterLibrary.by_class(n,unreal.RigUnit_CollectionItems.static_class())


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
h_mod = rig.get_hierarchy_modifier()

## meta 取得
reg = unreal.AssetRegistryHelpers.get_asset_registry();
a = reg.get_all_assets();
for aa in a:
    if (aa.get_editor_property("object_path") == args.vrm):
        v:unreal.VrmAssetListObject = aa
        vv = v.get_asset().vrm_meta_object
print(vv)
meta = vv


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
        #print(bone_m)

        tmp = '(Type=Control,Name='
        tmp += "{}".format(bone_h).lower() + '_c'
        tmp += ')'
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
