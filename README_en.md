# VRM4U
Runtime VRM loader for UnrealEngine4

## Description
VRM4U is importer for VRM.
Also it can load models on runtime.

https://github.com/ruyo/VRM4U/wiki/VRM4U

## Features
|||
|----|----|
|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/03.png)|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/04.png)|
|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/01.png)|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/02.png)|

- Import VRM file
- Animation
    - Generate bone, blendshape, swing bone, collision and humanoid rig.
    - Switch swing bone type PhysicsAsset/VRMSpringBone.
- Material
    - MToon simulated material. No postprocess.
- Mobile
    - Vanilla UE4Editor can use VRM on mobile by using BoneMap reduction.
    - Available on Forward/Deferred.

## Requirement
 - UE4.20.3, UE4.21, UE4.22

## SampleMap
- VRM4UContent/Maps/VRM4U_sample.umap
![3](https://raw.githubusercontent.com/wiki/ruyo/VRM4U/images/samplemap.png)

## Usage
 - Drag and drop VRM file.

![2](https://github.com/ruyo/VRM4U/wiki/images/overview.gif)


## Author
[@ruyo_h](https://twitter.com/ruyo_h)

## License
MIT(VRM4U)

3-clause BSD-License(assimp)

### Source
https://github.com/ruyo/UnrealEngine_VRM4UPlugin

https://github.com/ruyo/assimp

Thanks.
