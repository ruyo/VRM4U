# coding: utf-8
# ノードの骨指定部分を _c のControllerに置き換える
from asyncio.windows_events import NULL
from platform import java_ver
import unreal
import time
import argparse

print("VRM4U python begin")
print (__file__)

parser = argparse.ArgumentParser()
parser.add_argument("-vrm")
parser.add_argument("-rig")
parser.add_argument("-meta")
args = parser.parse_args()
print(args.vrm)

with unreal.ScopedSlowTask(1, "Convert Bone") as slow_task_root:
    slow_task_root.make_dialog()

    reg = unreal.AssetRegistryHelpers.get_asset_registry()

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

    def checkAndSwapPinBoneToContorl(pin):
        subpins = pin.get_sub_pins()
        #print(subpins)
        if (len(subpins) != 2):
            return;

        typePin = pin.find_sub_pin('Type')
        namePin = pin.find_sub_pin('Name')

        #if (typePin==None or namePin==None):
        if (typePin==None):
            return;

        #if (typePin.get_default_value() == '' or namePin.get_default_value() == ''):
        if (typePin.get_default_value() == ''):
            return;

        if (typePin.get_default_value() != 'Bone'):
            return;

        #print(typePin.get_default_value() + ' : ' + namePin.get_default_value())


        r_con.set_pin_default_value(typePin.get_pin_path(), 'Control', True, False)
        r_con.set_pin_default_value(namePin.get_pin_path(), namePin.get_default_value() + '_c', True, False)
        #print('swap end')

    #end check pin

    bonePin = None
    controlPin = None

    with unreal.ScopedSlowTask(1, "Replace Bone to Controller") as slow_task:
        slow_task.make_dialog()
        for n in node:
            pins = n.get_pins()
            for pin in pins:
                if (pin.is_array()):
                    for item in pin.get_sub_pins():
                        checkAndSwapPinBoneToContorl(item)
                else:
                    checkAndSwapPinBoneToContorl(pin)

