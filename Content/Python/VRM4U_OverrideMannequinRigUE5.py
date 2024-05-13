# coding: utf-8
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
        #print('swap end')

    #end check pin

    bonePin = None
    controlPin = None

    with unreal.ScopedSlowTask(1, "Replace Bone") as slow_task:
        slow_task.make_dialog()
        while(len(hierarchy.get_bones()) > 0):
            e = hierarchy.get_bones()[-1]
            h_con.remove_all_parents(e)
        h_con.import_bones(unreal.ControlRigBlueprintLibrary.get_preview_mesh(rig).skeleton)

    ##
    with unreal.ScopedSlowTask(1, "Replace Bone to Controller") as slow_task:
        slow_task.make_dialog()
        for n in node:
            pins = n.get_pins()
            for pin in pins:
                if (pin.is_array()):
                    if (pin.get_array_size() > 40):
                        # long bone list
                        typePin = pin.get_sub_pins()[0].find_sub_pin('Type')
                        if (typePin != None):
                            if (typePin.get_default_value() == 'Bone'):
                                bonePin = pin
                                continue
                            if (typePin.get_default_value() == 'Control'):
                                if ('Name="pelvis"' in r_con.get_pin_default_value(n.find_pin('Items').get_pin_path())):
                                    controlPin = pin
                                    continue
                    for item in pin.get_sub_pins():
                        checkAndSwapPinBoneToContorl(item)
                else:
                    checkAndSwapPinBoneToContorl(pin)


    for e in hierarchy.get_controls():
        tmp = "{}".format(e.name)
        if (tmp.endswith('_ctrl') == False):
            continue
        if (tmp.startswith('thumb_0') or tmp.startswith('index_0') or tmp.startswith('middle_0') or tmp.startswith('ring_0') or tmp.startswith('pinky_0')):
            print('')
        else:
            continue
        c = hierarchy.find_control(e)

        #print(e.name)


        #ttt = unreal.RigComputedTransform(transform=[[0.0, 0.0, 2.0], [90.0, 0.0, 0.0], [0.03, 0.03, 0.25]])
        #ttt = unreal.RigComputedTransform(transform=[[0.0, 0.0, 2.0], [90.0, 0.0, 0.0], [0.03, 0.03, 0.25]])
        ttt = unreal.Transform(location=[0.0, 0.0, 2.0], rotation=[0.0, 0.0, 90.0], scale=[0.03, 0.03, 0.25])
        hierarchy.set_control_shape_transform(e, ttt, True)
        '''
        shape = c.get_editor_property('shape')

        init = shape.get_editor_property('initial')
        #init = shape.get_editor_property('current')

        local = init.get_editor_property('local')
        local = ttt
        init.set_editor_property('local', local)

        gl = init.get_editor_property('global_')
        gl = ttt
        init.set_editor_property('global_', gl)

        shape.set_editor_property('initial', init)
        shape.set_editor_property('current', init)
        c.set_editor_property('shape', shape)
        '''


    ###
    swapBoneTable = [
        ["root",""],
        ["pelvis","hips"],
        ["spine_01","spine"],
        ["spine_02","chest"],
        ["spine_03","upperChest"],
        ["clavicle_l","leftShoulder"],
        ["upperArm_L","leftUpperArm"],
        ["lowerarm_l","leftLowerArm"],
        ["hand_L","leftHand"],
        ["index_01_l","leftIndexProximal"],
        ["index_02_l","leftIndexIntermediate"],
        ["index_03_l","leftIndexDistal"],
        ["middle_01_l","leftMiddleProximal"],
        ["middle_02_l","leftMiddleIntermediate"],
        ["middle_03_l","leftMiddleDistal"],
        ["pinky_01_l","leftLittleProximal"],
        ["pinky_02_l","leftLittleIntermediate"],
        ["pinky_03_l","leftLittleDistal"],
        ["ring_01_l","leftRingProximal"],
        ["ring_02_l","leftRingIntermediate"],
        ["ring_03_l","leftRingDistal"],
        ["thumb_01_l","leftThumbProximal"],
        ["thumb_02_l","leftThumbIntermediate"],
        ["thumb_03_l","leftThumbDistal"],
        ["lowerarm_twist_01_l",""],
        ["upperarm_twist_01_l",""],
        ["clavicle_r","rightShoulder"],
        ["upperArm_R","rightUpperArm"],
        ["lowerarm_r","rightLowerArm"],
        ["hand_R","rightHand"],
        ["index_01_r","rightIndexProximal"],
        ["index_02_r","rightIndexIntermediate"],
        ["index_03_r","rightIndexDistal"],
        ["middle_01_r","rightMiddleProximal"],
        ["middle_02_r","rightMiddleIntermediate"],
        ["middle_03_r","rightMiddleDistal"],
        ["pinky_01_r","rightLittleProximal"],
        ["pinky_02_r","rightLittleIntermediate"],
        ["pinky_03_r","rightLittleDistal"],
        ["ring_01_r","rightRingProximal"],
        ["ring_02_r","rightRingIntermediate"],
        ["ring_03_r","rightRingDistal"],
        ["thumb_01_r","rightThumbProximal"],
        ["thumb_02_r","rightThumbIntermediate"],
        ["thumb_03_r","rightThumbDistal"],
        ["lowerarm_twist_01_r",""],
        ["upperarm_twist_01_r",""],
        ["neck_01","neck"],
        ["head","head"],
        ["thigh_L","leftUpperLeg"],
        ["calf_l","leftLowerLeg"],
        ["calf_twist_01_l",""],
        ["foot_L","leftFoot"],
        ["ball_l","leftToes"],
        ["thigh_twist_01_l",""],
        ["thigh_R","rightUpperLeg"],
        ["calf_r","rightLowerLeg"],
        ["calf_twist_01_r",""],
        ["foot_R","rightFoot"],
        ["ball_r","rightToes"],
        ["thigh_twist_01_r",""],

        ["index_metacarpal_l",""],
        ["index_metacarpal_r",""],
        ["middle_metacarpal_l",""],
        ["middle_metacarpal_r",""],
        ["ring_metacarpal_l",""],
        ["ring_metacarpal_r",""],
        ["pinky_metacarpal_l",""],
        ["pinky_metacarpal_r",""],

        #custom
        ["eye_l", "leftEye"],
        ["eye_r", "rightEye"],

    ]

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

    for i in range(len(humanoidBoneList)):
        humanoidBoneList[i] = humanoidBoneList[i].lower()


    for i in range(len(swapBoneTable)):
        swapBoneTable[i][0] = swapBoneTable[i][0].lower()
        swapBoneTable[i][1] = swapBoneTable[i][1].lower()


    ### 全ての骨
    modelBoneElementList = []
    modelBoneNameList = []

    for e in hierarchy.get_bones():
        if (e.type == unreal.RigElementType.BONE):
            modelBoneElementList.append(e)
            modelBoneNameList.append("{}".format(e.name))

    ## meta 取得
    reg = unreal.AssetRegistryHelpers.get_asset_registry();
    a = reg.get_all_assets();

    if (args.meta):
        for aa in a:
            #print(aa.get_editor_property("package_name"))
            #print(aa.get_editor_property("asset_name"))
            if ((unreal.StringLibrary.conv_name_to_string(aa.get_editor_property("package_name"))+"."+unreal.StringLibrary.conv_name_to_string(aa.get_editor_property("asset_name"))) == unreal.StringLibrary.conv_name_to_string(args.meta)):
                #if (aa.get_editor_property("object_path") == args.meta):
                v:unreal.VrmMetaObject = aa
                vv = aa.get_asset()
                break

    if (vv == None):
        for aa in a:
            if ((unreal.StringLibrary.conv_name_to_string(aa.get_editor_property("package_name"))+"."+unreal.StringLibrary.conv_name_to_string(aa.get_editor_property("asset_name"))) == unreal.StringLibrary.conv_name_to_string(args.meta)):
                #if (aa.get_editor_property("object_path") == args.vrm):
                v:unreal.VrmAssetListObject = aa
                vv = v.get_asset().vrm_meta_object
                break

    meta = vv

    # Backwards Bone Names
    allBackwardsNode = []

    def r_nodes(node):
        b = False
        try:
            allBackwardsNode.index(node)
        except:
            b = True
        if (b == True):
            allBackwardsNode.append(node)

            linknode = node.get_linked_source_nodes()
            for n in linknode:
                r_nodes(n)

            linknode = node.get_linked_target_nodes()
            for n in linknode:
                r_nodes(n)

    for n in node:
        if (n.get_node_title() == 'Backwards Solve'):
            r_nodes(n)

    #print(len(allBackwardsNode))
    #print(len(node))

    def boneOverride(pin):
        subpins = pin.get_sub_pins()
        if (len(subpins) != 2):
            return

        typePin = pin.find_sub_pin('Type')
        namePin = pin.find_sub_pin('Name')

        if (typePin==None):
            return

        if (namePin==None):
            return

        controlName = namePin.get_default_value()

        if (controlName==''):
            return

        if (controlName.endswith('_ctrl')):
            return
        if (controlName.endswith('_space')):
            return

        r_con.set_pin_default_value(typePin.get_pin_path(), 'Bone', True, False)

        table = [i for i in swapBoneTable if i[0]==controlName]
        if (len(table) == 0):
            # use node  or  no control
            return

        if (table[0][0] == 'root'):
            metaTable = "{}".format(hierarchy.get_bones()[0].name)
        else:
            metaTable = meta.humanoid_bone_table.get(table[0][1])
        if (metaTable == None):
            metaTable = 'None'
        
        r_con.set_pin_default_value(namePin.get_pin_path(), metaTable, True, False)
        #for e in meta.humanoid_bone_table:

    for n in allBackwardsNode:
        pins = n.get_pins()
        for pin in pins:
            if (pin.is_array()):
                # finger 無変換チェック
                linknode = n.get_linked_target_nodes()
                if (len(linknode) == 1):
                    if (linknode[0].get_node_title()=='At'):
                        continue
                
                for p in pin.get_sub_pins():
                    boneOverride(p)
            else:
                boneOverride(pin)
        
        
    #sfsdjfkasjk


    ### 骨名対応表。Humanoid名 -> Model名
    humanoidBoneToModel = {"" : ""}
    humanoidBoneToModel.clear()

    humanoidBoneToMannequin = {"" : ""}
    humanoidBoneToMannequin.clear()

    # humanoidBone -> modelBone のテーブル作る
    for searchHumanoidBone in humanoidBoneList:
        bone_h = None
        for e in meta.humanoid_bone_table:
            if ("{}".format(e).lower() == searchHumanoidBone):
                bone_h = e;
                break;

        if (bone_h==None):
            # not found
            continue

        bone_m = meta.humanoid_bone_table[bone_h]

        try:
            i = modelBoneNameList.index(bone_m)
        except:
            i = -1
        if (i < 0):
            # no bone
            continue

        humanoidBoneToModel["{}".format(bone_h).lower()] = "{}".format(bone_m)
        #humanoidBoneToMannequin["{}".format(bone_h).lower()] = "{}".format(bone_m).lower(bone_h)


    #print(humanoidBoneToModel)

    #print(bonePin)
    #print(controlPin)

    if (bonePin != None and controlPin !=None):
        r_con.clear_array_pin(bonePin.get_pin_path())
        r_con.clear_array_pin(controlPin.get_pin_path())

    #c.clear_array_pin(v.get_pin_path())

    #tmp = '(Type=Control,Name='
    #tmp += "{}".format('aaaaa')
    #tmp += ')'
    #r_con.add_array_pin(bonePin.get_pin_path(), default_value=tmp)

    with unreal.ScopedSlowTask(len(swapBoneTable), "Replace Name") as slow_task:
        slow_task.make_dialog()

        for e in swapBoneTable:
            slow_task.enter_progress_frame(1)
            try:
                i = 0
                humanoidBoneToModel[e[1]]
            except:
                i = -1
            if (i < 0):
                # no bone
                continue
            #e[0] -> grayman
            #e[1] -> humanoid
            #humanoidBoneToModel[e[1]] -> modelBone

            bone = unreal.RigElementKey(unreal.RigElementType.BONE, humanoidBoneToModel[e[1]])
            space = unreal.RigElementKey(unreal.RigElementType.NULL, "{}_s".format(e[0]))

            #print('aaa')
            #print(bone)
            #print(space)

            #p = hierarchy.get_first_parent(humanoidBoneToModel[result[0][1]])

            t = hierarchy.get_global_transform(bone)
            #print(t)
            hierarchy.set_global_transform(space, t, True)

            if (bonePin != None and controlPin !=None):
            
                tmp = '(Type=Control,Name='
                tmp += "{}".format(e[0])
                tmp += ')'
                r_con.add_array_pin(controlPin.get_pin_path(), default_value=tmp, setup_undo_redo=False)

                tmp = '(Type=Bone,Name='
                tmp += "\"{}\"".format(humanoidBoneToModel[e[1]])
                tmp += ')'
                r_con.add_array_pin(bonePin.get_pin_path(), default_value=tmp, setup_undo_redo=False)

                # for bone name space
                namePin = bonePin.get_sub_pins()[-1].find_sub_pin('Name')
                r_con.set_pin_default_value(namePin.get_pin_path(), "{}".format(humanoidBoneToModel[e[1]]), True, False)


    # skip invalid bone, controller

    # disable node
    def disableNode(toNoneNode):
        print(toNoneNode)
        pins = toNoneNode.get_pins()
        for pin in pins:
            typePin = pin.find_sub_pin('Type')
            namePin = pin.find_sub_pin('Name')

            if (typePin==None or namePin==None):
                continue
            print(f'DisablePin {typePin.get_default_value()} : {namePin.get_default_value()}')

            # control
            key = unreal.RigElementKey(unreal.RigElementType.CONTROL, "{}".format(namePin.get_default_value()))

            # disable node
            r_con.set_pin_default_value(namePin.get_pin_path(), 'None', True, False)

            if (typePin.get_default_value() == 'Control'):

                #disable control
                if (hierarchy.contains(key) == True):
                    settings = h_con.get_control_settings(key)

                    if ("5." in unreal.SystemLibrary.get_engine_version()):
                        if ("5.0." in unreal.SystemLibrary.get_engine_version()):
                            settings.set_editor_property('shape_enabled',  False)
                        else:
                            settings.set_editor_property('shape_visible',  False)
                    else:
                        settings.set_editor_property('shape_enabled',  False)
                
                    ttt = hierarchy.get_global_control_shape_transform(key, True)
                    ttt.scale3d.set(0.001, 0.001, 0.001)
                    hierarchy.set_control_shape_transform(key, ttt, True)

                    h_con.set_control_settings(key, settings)


    for n in node:
        pins = n.get_pins()
        for pin in pins:
            if (pin.is_array()):
                continue
            else:
                typePin = pin.find_sub_pin('Type')
                namePin = pin.find_sub_pin('Name')
                if (typePin==None or namePin==None):
                    continue
                if (typePin.get_default_value() != 'Bone'):
                    continue
                
                if (namePin.is_u_object() == True):
                    continue

                if (len(n.get_linked_source_nodes()) > 0):
                    continue

                key = unreal.RigElementKey(unreal.RigElementType.BONE, namePin.get_default_value())
                if (hierarchy.contains(key) == True):
                    continue
                print(f'disable linked node from {typePin.get_default_value()} : {namePin.get_default_value()}')
                for toNoneNode in n.get_linked_target_nodes():
                    disableNode(toNoneNode)

#    for n in node:
#        pins = n.get_pins()
#        for pin in pins:
#            if (pin.is_array()):
#                continue
#            else:
#                typePin = pin.find_sub_pin('Type')
#                namePin = pin.find_sub_pin('Name')
#                if (typePin == None):
#                    continue
#                if (typePin.get_default_value() == 'Bone'):
#                            bonePin = pin
#                            continue
#                        if (typePin.get_default_value() == 'Control'):
#                            if ('Name="pelvis"' in r_con.get_pin_default_value(n.find_pin('Items').get_pin_path())):
#                                controlPin = pin
#                                continue
#                for item in pin.get_sub_pins():
#                    checkAndSwapPinBoneToContorl(item)
#                checkAndSwapPinBoneToContorl(pin)



## morph

# -vrm vrm  -rig rig -debugeachsave 0
#args.vrm
#args.rig

command = 'VRM4U_CreateMorphTargetControllerUE5.py ' + '-vrm ' + args.vrm + ' -rig ' + args.rig + ' -debugeachsave 0'

print(command)

unreal.PythonScriptLibrary.execute_python_command(command)

unreal.ControlRigBlueprintLibrary.recompile_vm(rig)
