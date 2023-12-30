# coding: utf-8
import unreal
import time

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-vrm")
parser.add_argument("-rig")
parser.add_argument("-debugeachsave")
args = parser.parse_args()
#print(args.vrm)


######

with unreal.ScopedSlowTask(1, "Convert MorphTarget") as slow_task_root:
    slow_task_root.make_dialog()

    rigs = unreal.ControlRigBlueprint.get_currently_open_rig_blueprints()
    print(rigs)

    for r in rigs:
        s:str = r.get_path_name()
        ss:str = args.rig
        if (s.find(ss) < 0):
            print("no rig")
        else:
            rig = r

    hierarchy = unreal.ControlRigBlueprintLibrary.get_hierarchy(rig)

    h_con = hierarchy.get_controller()

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

    bRemoveElement = False
    if ("5." in unreal.SystemLibrary.get_engine_version()):
        if ("5.0." in unreal.SystemLibrary.get_engine_version()):
            bRemoveElement = True
    else:
        bRemoveElement = True

    if (bRemoveElement):
        while(len(hierarchy.get_bones()) > 0):
            e = hierarchy.get_bones()[-1]
            h_con.remove_all_parents(e)
            h_con.remove_element(e)

    h_con.import_bones(unreal.ControlRigBlueprintLibrary.get_preview_mesh(rig).skeleton)
    h_con.import_curves(unreal.ControlRigBlueprintLibrary.get_preview_mesh(rig).skeleton)

    dset = rig.get_editor_property('rig_graph_display_settings')
    dset.set_editor_property('node_run_limit', 0)
    rig.set_editor_property('rig_graph_display_settings', dset)



    ###### root
    key = unreal.RigElementKey(unreal.RigElementType.NULL, 'MorphControlRoot_s')
    space = hierarchy.find_null(key)
    if (space.get_editor_property('index') < 0):
        space = h_con.add_null('MorphControlRoot_s', space_type=unreal.RigSpaceType.SPACE)
    else:
        space = key


    #a:unreal.RigUnit_CollectionItems = unreal.RigUnit_CollectionItems()
    #print(a)

    # 配列ノード追加
    values_forCurve:unreal.RigVMStructNode = []
    items_forControl:unreal.RigVMStructNode = []
    items_forCurve:unreal.RigVMStructNode = []


    for node in n:
        #print(node)
        #print(node.get_node_title())

        # set curve num
        if (node.get_node_title() == 'For Loop'):
            #print(node)
            pin = node.find_pin('Count')
            #print(pin)
            c.set_pin_default_value(pin.get_pin_path(), str(len(morphList)), True, False)

        # curve name array pin
        if (node.get_node_title() == 'Select'):
            #print(node)
            pin = node.find_pin('Values')
            #print(pin)
            #print(pin.get_array_size())
            #print(pin.get_default_value())
            values_forCurve.append(pin)
            
        # items
        if (node.get_node_title() == 'Collection from Items'):

            if ("Type=Curve," in c.get_pin_default_value(node.find_pin('Items').get_pin_path())):
                items_forCurve.append(node.find_pin('Items'))
            elif (node.find_pin('Items').get_array_size() == 40):
                items_forControl.append(node.find_pin('Items'))
        
    print(items_forControl)
    print(values_forCurve)

    # reset controller
    for e in reversed(hierarchy.get_controls()):
        if (len(hierarchy.get_parents(e)) == 0):
            continue
        if (hierarchy.get_parents(e)[0].name == 'MorphControlRoot_s'):
            #if (str(e.name).rstrip('_c') in morphList):
            #    continue
            print('delete')

            #print(str(e.name))
            if (bRemoveElement):
                h_con.remove_element(e)


    # curve array
    for  v in values_forCurve:
        c.clear_array_pin(v.get_pin_path(), False)
        for morph in morphList:
            tmp = "{}".format(morph)
            c.add_array_pin(v.get_pin_path(), default_value=tmp, setup_undo_redo=False)

    # curve controller
    for morph in morphListWithNo:
        name_c = "{}_c".format(morph)
        key = unreal.RigElementKey(unreal.RigElementType.CONTROL, name_c)
        
        settings = unreal.RigControlSettings()
        settings.shape_color = [1.0, 0.0, 0.0, 1.0]
        settings.control_type = unreal.RigControlType.FLOAT
        
        
        try:
            control = hierarchy.find_control(key)
            if (control.get_editor_property('index') < 0):
                k = h_con.add_control(name_c, space, settings, unreal.RigControlValue(), setup_undo=False)
                control = hierarchy.find_control(k)
        except:
            k = h_con.add_control(name_c, space, settings, unreal.RigControlValue(), setup_undo=False)
            control = hierarchy.find_control(k)

        shape_t = unreal.Transform(location=[0.0, 0.0, 0.0], rotation=[0.0, 0.0, 0.0], scale=[0.001, 0.001, 0.001])
        hierarchy.set_control_shape_transform(k, shape_t, True)


        morphListRenamed.append(control.key.name)
        if (args.debugeachsave == '1'):
            try:
                unreal.EditorAssetLibrary.save_loaded_asset(rig)
            except:
                print('save error')
            #unreal.SystemLibrary.collect_garbage()

    # eye controller
    eyeControllerTable = [
        "VRM4U_EyeUD_left",
        "VRM4U_EyeLR_left",

        "VRM4U_EyeUD_right",
        "VRM4U_EyeLR_right",
    ]

    # eye controller
    for eyeCon in eyeControllerTable:
        name_c = "{}_c".format(eyeCon)
        key = unreal.RigElementKey(unreal.RigElementType.CONTROL, name_c)
        
        settings = unreal.RigControlSettings()
        settings.shape_color = [1.0, 0.0, 0.0, 1.0]
        settings.control_type = unreal.RigControlType.FLOAT
        
        try:
            control = hierarchy.find_control(key)
            if (control.get_editor_property('index') < 0):
                k = h_con.add_control(name_c, space, settings, unreal.RigControlValue(), setup_undo=False)
                control = hierarchy.find_control(k)
        except:
            k = h_con.add_control(name_c, space, settings, unreal.RigControlValue(), setup_undo=False)
            control = hierarchy.find_control(k)

        shape_t = unreal.Transform(location=[0.0, 0.0, 0.0], rotation=[0.0, 0.0, 0.0], scale=[0.001, 0.001, 0.001])
        hierarchy.set_control_shape_transform(k, shape_t, True)

    # curve Control array
    with unreal.ScopedSlowTask(len(items_forControl)*len(morphListRenamed), "Add Control") as slow_task:
        slow_task.make_dialog()
        for  v in items_forControl:
            c.clear_array_pin(v.get_pin_path(), False)
            for morph in morphListRenamed:
                slow_task.enter_progress_frame(1)
                tmp = '(Type=Control,Name='
                tmp += "{}".format(morph)
                tmp += ')'
                c.add_array_pin(v.get_pin_path(), default_value=tmp, setup_undo_redo=False)

    # curve Float array
    with unreal.ScopedSlowTask(len(items_forCurve)*len(morphList), "Add Curve") as slow_task:
        slow_task.make_dialog()
        for  v in items_forCurve:
            c.clear_array_pin(v.get_pin_path(), False)
            for morph in morphList:
                slow_task.enter_progress_frame(1)
                tmp = '(Type=Curve,Name='
                tmp += "{}".format(morph)
                tmp += ')'
                c.add_array_pin(v.get_pin_path(), default_value=tmp, setup_undo_redo=False)





