# VRM4U

[English Doc](https://github.com/ruyo/VRM4U/blob/master/README_en.md)
| [Korean Doc](https://github.com/ruyo/VRM4U/blob/master/README_ko.md)

## 소개

VRM4U는 UE4와 함께 작동하는 VRM 파일 임폴터 입니다.

**자세한 사용 방법은 [이 페이지에서 확인하세요.](https://ruyo.github.io/VRM4U/)**

※ 배포용 EXE, 모바일 실행의 경우에는 소스 리포지토리 데이터가 필요합니다. 아래 절차를 참고해주세요.

## 기능

|||
|----|----|
|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/03.png)|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/04.png)|
|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/01.png)|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/02.png)|

- VRM 파일을 가져올 수 있습니다.
- 애니메이션
    - 뼈, MorphTarget, BlendShapeGroup, 스윙본, 콜리전 등이 생성됩니다.
    - 스윙본의 타입은 VRMSpringBone 또는 PhysicsAsset으로 선택할 수 있습니다.
    - 휴머노이드용 Rig가 생성되므로 애니메이션을 간단하게 리타겟팅 가능합니다.
- 메테리얼
    - MToon을 시뮬레이션한 메테리얼, 그림자 색 지정, 윤곽선의 색 및 굵기 조정, MatCap 등 그대로 적용됩니다.
    - 기존 PBR 배경에 캐릭터를 그릴 수 있습니다.
    - 포스트 프로세싱을 사용하지 않습니다.
- 모바일
    - BoneMap 리덕션을 사용하여 공식 UE4 편집기에서 모바일 SkeletalMesh를 사용할 수 있습니다.
    - Forward/Deferred 둘 다 지원합니다.

## 운영체제

- UE4.20〜UE4.27
- Windows, Android, iOS, Mac(프로젝트 빌드 필요)
- UE4.19도 작동하지만 메테리얼은 생성되지 않습니다.

## 사용법

- **배포용 exe를 만드시거나 모바일 환경에서 실행 하시려면 아래 리포지토리에서 다운로드 하세요.**
- 에디터에서 사용하려면 [여기](https://github.com/ruyo/VRM4U/releases/latest)에서 가장 높은 버전의 플러그인을 다운로드 하시고 ".uproject"와 함께 있는 "
  Plugins" 폴더에 넣으세요

### 예제 맵

- VRM4UContent/Maps/VRM4U_sample.umap
- 컨텐츠 브라우저에 표시되지 않는다면 아래 사진을 참고하세요.
  ![3](https://raw.githubusercontent.com/wiki/ruyo/VRM4U/images/samplemap.png)

### 사용 방법

- VRM 파일을 끌어다 놓으세요.

||
|----|
|![2](https://github.com/ruyo/VRM4U/wiki/images/overview.gif)|
|[![](https://img.youtube.com/vi/Qlz0bUSLjss/0.jpg)](https://www.youtube.com/watch?v=Qlz0bUSLjss) https://www.youtube.com/watch?v=Qlz0bUSLjss|

### 작동원리

[더 자세한 정보는 여기서 확인하세요.](https://speakerdeck.com/ruyo/vrm4u-wakaru)

## 만든 사람

[@ruyo_h](https://twitter.com/ruyo_h)

## 라이센스

|||
|----|----|
|MIT|VRM4U|
|MIT|[RapidJSON](https://github.com/Tencent/rapidjson/)|
|3-clause BSD-License|[assimp](https://github.com/assimp/assimp), [assimp](https://github.com/ruyo/assimp)|

### 출처

UE4 계정이 연결되어 있어야 합니다.

편의 및 만약을 위해 EpicGames 계정을 연결하여야 합니다. 번거롭게 해서 죄송합니다.

https://github.com/ruyo/UnrealEngine_VRM4UPlugin
（404 오류 페이지가 표시되는
경우 [여기에서 연결하세요.](https://www.unrealengine.com/ja/blog/updated-authentication-process-for-connecting-epic-github-accounts)）

https://github.com/ruyo/assimp
