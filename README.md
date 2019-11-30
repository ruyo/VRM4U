# VRM4U

[English Doc](https://github.com/ruyo/VRM4U/blob/master/README_en.md)

## はじめに
VRM4U はVRMファイルのインポーター兼ランタイムローダーです

詳細はこちらから
https://github.com/ruyo/VRM4U/wiki/VRM4U

## 特徴
|||
|----|----|
|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/03.png)|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/04.png)|
|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/01.png)|![2](https://github.com/ruyo/VRM4U/wiki/images/shot/02.png)|

 - VRMファイルをインポートできます。
 - アニメーション
     - 骨、Morphtarget・BlendShapeGroup、揺れ骨・コリジョン などが生成されます。
     - 揺れ骨の挙動はVRMSpringBoneかPhysicsAssetを選択できます。
     - Humanoid用のRIGが生成されるので、アニメーションを手軽にリターゲット可能です。
 - マテリアル
     - MToonを再現したマテリアル。影色の指定や、アウトラインの色・太さ調整、MatCapなど一通り適用されます。
     - 既存のPBR背景の中にキャラクタを描画できます。ポストプロセスを利用しません。
 - モバイル
     - BoneMapリダクションを使うことで、公式のUE4エディタからモバイルでSkeletalMeshを利用できます。
     - 描画はForward/Deferred両方に対応しています。

## 動作環境
 - UE4.20、UE4.21、UE4.22、UE4.23
 - UE4.19も動きますが、マテリアルは生成されません。

## 使い方
- **配布用のexeを作成したり、モバイルで実行する場合は、後述のソースリポジトリからダウンロードしてください。**
- エディタでのみ利用する場合は、[releases](https://github.com/ruyo/VRM4U/releases/latest)より利用するバージョンのプラグインをダウンロードし、
   「.uproject」とおなじ場所に「Plugins」フォルダを展開してください。

### サンプルマップ
- VRM4UContent/Maps/VRM4U_sample.umap
- ContentBrowserに表示されない場合は、以下の項目を確認ください。
![3](https://raw.githubusercontent.com/wiki/ruyo/VRM4U/images/samplemap.png)

### 使い方
- VRMファイルをドラッグ＆ドロップしてください

||
|----|
|![2](https://github.com/ruyo/VRM4U/wiki/images/overview.gif)|
|[![](https://img.youtube.com/vi/Qlz0bUSLjss/0.jpg)](https://www.youtube.com/watch?v=Qlz0bUSLjss) https://www.youtube.com/watch?v=Qlz0bUSLjss|


### 仕組み
詳しく知りたい方はこちら


https://speakerdeck.com/ruyo/vrm4u-wakaru

## 作った人
[@ruyo_h](https://twitter.com/ruyo_h)

## ライセンス
MIT(VRM4U)

3-clause BSD-License(assimp)

### ソース
UE4アカウントの紐づけが必要です。

https://github.com/ruyo/UnrealEngine_VRM4UPlugin （404エラーページが出る場合は[こちらより紐付けをしてください](https://www.unrealengine.com/ja/blog/updated-authentication-process-for-connecting-epic-github-accounts)）

https://github.com/ruyo/assimp


公開の体裁を含め 多くの方の情報を参考にさせて頂いています。  
ありがとうございます。

### リリース履歴
- 2019/12/01
    - 法線補正をカメラ方向に変更した。（従来はライト方向）
    - エディタモードでキャラライトがカメラ座標を参照して動作するようにした。
    - マテリアル未指定のVRMファイルを読み込むと停止するのを修正した。
- 2019/11/18
    - UE4.24previewのビルドエラーを修正した
    - RayTracing利用中にインポートすると停止することがあったのを修正した。
    - クリッピング対処のダミーコリジョンを小さくした。
    - assimp最新をマージした。
- 2019/11/14
    - Springのtypoを修正した。（モデルをインポートしなおす必要あり）
    - 前回の更新でMToonカラーが適用されなくなっていたのを修正した
    - SSAOパラメータを上書きするか、選択できるようにした。
- 2019/11/05
    - ライトリグとカメラを追加した。LightingChannel1番を利用できるようにした。
    - サブサーフェスの濃さを調整できるようにした。
    - bvhファイルの読み込みでクラッシュするのを修正した
- 2019/10/21
    - デフォルトリターゲットをA-poseにした。オプションで従来のT-poseに変更できるようにした。
    - クリッピングを防ぐため、ダミーコリジョンを追加した。
    - 法線方向を補正できるようにした。
    - Lit/Unlit切り替え時の停止することがあるのを修正した。
- 2019/10/16
    - 前回リリースでセルフシャドウが動かなくなっていたのを修正した
- 2019/10/15
    - アウトラインの太さが0の時にノイズがでるのを修正した
- 2019/10/14_1
    - 特定のVRMでassimpがクラッシュするのを修正した（Meshを先頭から読むようにした）
    - デバッグ用の骨1本化機能で停止するのを修正した。
    - MorphTargetのNormalを適用しないようにした（オプションで選択可）
- 2019/10/14
    - BPのみのプロジェクトでパッケージ後にアセットが参照できなくなるのを修正した
    - MorphControlActorによる相対座標での目線の制御を追加した。
- 2019/10/07
    - Engineのpluginとして配置して場合も動作するようにした
- 2019/10/02
    - モデルによって、Root骨を整頓すると腰座標がずれてしまうことがあるのを修正した（主にVRChat用モデル）
    - 輪郭線モデルのMorphTargetが動作しないのを修正した。
    - PIEでのランタイムロード時に停止するのを修正した。
    - ランタイムリターゲットで腰位置がずれることがあるのを修正した。
    - AO Shadow に扱いやすいパラメータを追加した。
- 2019/09/24
    - uv2のインポートに対応した。
    - モデルによって、Root骨がずれてしまうことがあるのを修正した（主にVRChat用モデル）
    - TwoSided用マテリアルを追加した。不要なMaterialOverrideが起きないようにした。
    - VRoidSimpleモデルのリターゲット時、プレビューが出ないのを修正した（Boundsが不正だったのを設定した）
- 2019/09/16
    - VRMインポート時にモデルのタイトル名を表示できるようにした
    - MorphTargetをコントロールするアクタを追加した
- 2019/09/05
    - UE4.23に対応した
- 2019/09/04
    - モバイル向けに軽量シャドウマップを生成できるようにした
    - 特定のVRMインポート時に停止するのを修正した（任意のマテリアル参照順に対応した）
- 2019/08/19
    - 機能追加
    -  2nd Shade Color を設定できるようにした
    -  AO ShadowModelを生成できるようにした
    -  bvhリターゲットのマッピングを ある程度自動判別するようにした
- 2019/07/24
    - bvhファイルのインポートに対応した。
    - モデルによってはリターゲットで顔向きがずれるのを対応した（upperChestが無いVRMの対応をした）
- 2019/07/13　マテリアルを修正＆機能追加
    - ShaderToonyとShaderShiftが本家MToonとずれていたのを修正した。
    - MatCapにシャドウを反映できるようにした
    - Roughnessマップを反映できるようにした。
    - UE4標準のSubsurfaceを利用できるようにした。
    - カスタムのShadowmapサイズを変更できるようにした
- 2019/06/24
    - RimLight, UVScrollに対応した。
    - モデルのtangentの軸がずれていたのを修正した。
- 2019/06/15
    - モデル構造によってマテリアルやテクスチャインデックスがずれることがあったのを修正した。
- 2019/06/10
    - SM4でのマテリアルのエラーを修正した。シェーダQualityを3段階ぶん切り替え可能にした。
    - lookatサンプルを追加した。
    - 他プラグインと依存関係にあるBPをフォルダにまとめた。
- 2019/06/01
    - AttachActorの設定が戻らないことがあるのを修正した。変更時、対象のSkeletalMeshのマテリアルを初期化するようにした。
- 2019/05/27
    - OculusQuest(Android)向けにビルドした時のエラーを修正した。
- 2019/05/24
    - 輪郭線の太さが一定になるようにした。
    - 影がアルファ値を参照していなかったのを、反映するようにした。
    - モバイル向けBoneMapリダクションの骨数最大値を74->75に変更した
- 2019/05/14
    - VRMSpringBoneの重力方向がずれていたのを修正した。
    - VrmCharacterBaseでSpringBoneを利用できるようにした。
    - 輪郭線が明るい場合があるのを修正した。（輪郭線にもEyeAdaptation補正を追加した）
- 2019/05/13
    - VRMSpringBoneの重力、外力をワールド座標で反映するようにした。
    - インポート時間を短縮した。不要な処理をスキップした。
- 2019/04/29
    - AnimBPリターゲット時、停止することがあったのを対処した。
    - CharacterBaseをアタッチベースで作り直した。
    - ランタイムロードサンプルを追加した。
- 2019/04/24
    - モバイル向けにBoneMapリダクションオプションを追加した。
    - PhysicsAssetの参照骨数がSkeletonより多くなってしまうことがある不具合を修正した。
- 2019/04/21 MorphTargetのY座標が正負反転していたのを修正した。
- 2019/04/18 モデル法線を正規化するようにした。モバイルでのマテリアルのエラーを修正した。
- 2019/04/08 大幅に整頓
    - デフォルトマテリアルをUnlitにした
    - 輪郭線描画をアタッチタイプに統一した。サンプルを置き換えた。
    - 動作が安定したのでAsset強制保存をの機能を外した。
- 2019/04/04 アタッチで輪郭線描画できるActorを追加した。
- 2019/03/31 UE4.21のマテリアル編集時に停止するため、パッチ対処した。
    - 停止の仮対処としてAssetを強制保存するようにした。
    - VRMファイルによっては停止することがあったのを、AssImp側で修正した。
- 2019/03/28 マテリアル最適化時、メモリアクセス違反が起きるのを修正した。
    - VRMSpringBoneがPhysicsCollisionに反応するようにした。
    - リターゲットのプレビューにモデルが表示されないのを修正した。（BoundingBoxをセットした）
- 2019/03/24 サンプルマップを追加した。
- 2019/03/23 VRMSpringBoneを仮実装した。輪郭線の描画方法を変更した。
- 2019/03/05 MotionController,LeapMotionによるトラッキングのAnimBPを追加した
- これ以前のものはこちら → https://github.com/ruyo/VRM4U/blob/master/CHANGELOG.md

