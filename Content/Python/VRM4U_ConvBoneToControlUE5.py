# coding: utf-8
from asyncio.windows_events import NULL
import unreal

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-vrm")
parser.add_argument("-rig")
parser.add_argument("-meta")
args = parser.parse_args()
print(args.vrm)

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
hierarchy = unreal.ControlRigBlueprintLibrary.get_hierarchy(rig)

h_con = hierarchy.get_controller()

r_con = rig.get_controller()

graph = r_con.get_graph()
node = graph.get_nodes()

for e in hierarchy.get_controls(True):
    h_con.remove_element(e)

for e in hierarchy.get_nulls(True):
    h_con.remove_element(e)

boneCount = 0

for e in hierarchy.get_bones():
    p = hierarchy.get_first_parent(e)
    print('===')
    print(e)
    print(p)
    t = unreal.Transform()
    s = unreal.RigControlSettings()
    s.shape_visible = False
    v = unreal.RigControlValue()
    shape_t = unreal.Transform(location=[0.0, 0.0, 0.0], rotation=[0.0, 0.0, 0.0], scale=[0.001, 0.001, 0.001])

    if (boneCount == 0):
        n = h_con.add_null("{}_s".format(e.name), unreal.RigElementKey(), t)
        c = h_con.add_control(e.name, n, s, v)
        t = hierarchy.get_global_transform(n)
        hierarchy.set_global_transform(c, t, True)
        hierarchy.set_control_shape_transform(c, shape_t, True)

    else:
        p.type = unreal.RigElementType.CONTROL
        n = h_con.add_null("{}_s".format(e.name), p, t)
        c = h_con.add_control(e.name, n, s, v)
        t = hierarchy.get_global_transform(n)
        hierarchy.set_global_transform(c, t, True)
        hierarchy.set_control_shape_transform(c, shape_t, True)

    if ("{}".format(e.name)  == "head"):
        parent = c
        n = h_con.add_null("eye_l_s", parent, t)
        c = h_con.add_control("eye_l", n, s, v)
        t = hierarchy.get_global_transform(n)
        hierarchy.set_global_transform(c, t, True)
        hierarchy.set_control_shape_transform(c, shape_t, True)

        n = h_con.add_null("eye_r_s", parent, t)
        c = h_con.add_control("eye_r", n, s, v)
        t = hierarchy.get_global_transform(n)
        hierarchy.set_global_transform(c, t, True)
        hierarchy.set_control_shape_transform(c, shape_t, True)


    boneCount += 1

