# coding: utf-8
import unreal

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-vrm")
parser.add_argument("-rig")
parser.add_argument("-debugeachsave")
args = parser.parse_args()
#print(args.vrm)


######

rigs = unreal.ControlRigBlueprint.get_currently_open_rig_blueprints()
print(rigs)

for r in rigs:
    s:str = r.get_path_name()
    ss:str = args.rig
    if (s.find(ss) < 0):
        print("no rig")
    else:
        rig = r


h_mod = rig.get_hierarchy_modifier()

elements = h_mod.get_selection()

print(unreal.SystemLibrary.get_engine_version())
if (unreal.SystemLibrary.get_engine_version()[0] == '5'):
    c = rig.get_controller()
else:
    c = rig.controller

g = c.get_graph()
n = g.get_nodes()

mesh = rig.get_preview_mesh()
morphList = mesh.get_all_morph_target_names()
morphListWithNo = morphList[:]

morphListRenamed = []
morphListRenamed.clear()


for i in range(len(morphList)):
    morphListWithNo[i] = '{}'.format(morphList[i])

    
print(morphListWithNo)

###### root
key = unreal.RigElementKey(unreal.RigElementType.SPACE, 'MorphControlRoot_s')
space = h_mod.get_space(key)
if (space.get_editor_property('index') < 0):
    space = h_mod.add_space('MorphControlRoot_s', space_type=unreal.RigSpaceType.SPACE)
else:
    space = key


a:unreal.RigUnit_CollectionItems = unreal.RigUnit_CollectionItems()

print(a)

# 配列ノード追加
values_forCurve:unreal.RigVMStructNode = []
items_forControl:unreal.RigVMStructNode = []
items_forCurve:unreal.RigVMStructNode = []


for node in n:
    print(node)
    print(node.get_node_title())

    # set curve num
    if (node.get_node_title() == 'For Loop'):
        print(node)
        pin = node.find_pin('Count')
        print(pin)
        c.set_pin_default_value(pin.get_pin_path(), str(len(morphList)))

    # curve name array pin
    if (node.get_node_title() == 'Select'):
        print(node)
        pin = node.find_pin('Values')
        #print(pin)
        #print(pin.get_array_size())
        #print(pin.get_default_value())
        values_forCurve.append(pin)

    # items
    if (node.get_node_title() == 'Items'):
        if ("Type=Curve," in c.get_pin_default_value(node.find_pin('Items').get_pin_path())):
            items_forCurve.append(node.find_pin('Items'))
        else:
            items_forControl.append(node.find_pin('Items'))

print(values_forCurve)

# reset controller
for e in reversed(h_mod.get_elements()):
    if (e.type!= unreal.RigElementType.CONTROL):
        continue
    tmp = h_mod.get_control(e)
    if (tmp.space_name == 'MorphControlRoot_s'):
        #if (str(e.name).rstrip('_c') in morphList):
        #    continue
        print('delete')
        #print(str(e.name))
        h_mod.remove_element(e)


# curve array
for  v in values_forCurve:
    c.clear_array_pin(v.get_pin_path())
    for morph in morphList:
        tmp = "{}".format(morph)
        c.add_array_pin(v.get_pin_path(), default_value=tmp)

# curve controller
for morph in morphListWithNo:
    name_c = "{}_c".format(morph)
    key = unreal.RigElementKey(unreal.RigElementType.CONTROL, name_c)
    try:
        control = h_mod.get_control(key)
        if (control.get_editor_property('index') < 0):
            k = h_mod.add_control(name_c,
                control_type=unreal.RigControlType.FLOAT,
                space_name=space.name,
                gizmo_color=[1.0, 0.0, 0.0, 1.0],
                )
            control = h_mod.get_control(k)
    except:
        k = h_mod.add_control(name_c,
            control_type=unreal.RigControlType.FLOAT,
            space_name=space.name,
            gizmo_color=[1.0, 0.0, 0.0, 1.0],
            )
        control = h_mod.get_control(k)
    control.set_editor_property('gizmo_visible', False)
    control.set_editor_property('gizmo_enabled', False)
    h_mod.set_control(control)

    morphListRenamed.append(control.get_editor_property('name'))
    if (args.debugeachsave == '1'):
        try:
            unreal.EditorAssetLibrary.save_loaded_asset(rig)
        except:
            print('save error')
        #unreal.SystemLibrary.collect_garbage()

# curve Control array
for  v in items_forControl:
    c.clear_array_pin(v.get_pin_path())
    for morph in morphListRenamed:
        tmp = '(Type=Control,Name='
        tmp += "{}".format(morph)
        tmp += ')'
        c.add_array_pin(v.get_pin_path(), default_value=tmp)

# curve Float array
for  v in items_forCurve:
    c.clear_array_pin(v.get_pin_path())
    for morph in morphList:
        tmp = '(Type=Curve,Name='
        tmp += "{}".format(morph)
        tmp += ')'
        c.add_array_pin(v.get_pin_path(), default_value=tmp)


