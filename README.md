# VRM4U

[English Doc](https://github.com/ruyo/VRM4U/blob/master/README_en.md)

## はじめに
VRM4UはUE4で動作する、VRMファイルのインポーターです。

**使い方は [こちらのページにあります](https://ruyo.github.io/VRM4U/)**

※配布用のexe作成、モバイル実行には、ソースリポジトリのデータが必要です。 後述の手順を参照ください。


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
 - UE4.20〜UE4.25
 - Windows, Android, iOS
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

ライセンスの都合上、念の為EpicGamesアカウント紐付けにしています。手間ですみません。。

https://github.com/ruyo/UnrealEngine_VRM4UPlugin （404エラーページが出る場合は[こちらより紐付けをしてください](https://www.unrealengine.com/ja/blog/updated-authentication-process-for-connecting-epic-github-accounts)）

https://github.com/ruyo/assimp


公開の体裁を含め 多くの方の情報を参考にさせて頂いています。  
ありがとうございます。

### リリース履歴
- 2020/05/20
    - UE4.25でOculusVRプラグインが無効の時、停止してしまうのを修正した。
    - インポート時、UAssetを個別に保存できるようにした。
    - SSS切替時のデフォルトパラメータを調整した。
    - RectLightのアーティファクトが目立たなくなるようにした。（ShadowSlopeBiasを変更した）
- 2020/05/17
    - 汎用的なキャラライトを追加した。初期値をSpotLightに戻した。（SSSでノイズが出るため）
    - SSSパラメータをMaterialSystemで一括オーバーライドできるようにした。
    - キャラカメラに、キャラ注視のまま操作できるモードを追加した。
    - 色が残りやすいBloomを追加した。
    - VRMインポート時のデータチェックを厳密にし、クラッシュしにくくした。
- 2020/05/11
    - 描画クオリティをMaterialSystemから変更できるようにした。
    - SSS設定時のラフネス初期値を変更した(0.8->0.7)
    - SSS設定時のNoTranslucentオプションが動作しないことがあるのを修正した。
    - インポート時のマテリアルマージで半透明が判別できないのを修正した。
    - 4.25でのインポート後、エディタ再起動でブレンドシェイプが消えるのを修正した。
- 2020/05/06
    - UE4.25に対応した。
    - キャラマテリアルにPBRモードを追加した。
    - サブライトを設定できるようにした。
    - マテリアル切り替え時にエディタが停止することがあるのを修正した。
    - マテリアルに彩度調整を追加した。
- 2020/05/02
    - Editor用のモジュールを追加した。
    - rig生成時、NearZを自動で変更(10->1)するようにした。選択しにくい場合の対処のためZオフセットを変更した。
    - rigからAnimSequenceを出力できるようにした。
- 2020/04/29
    - キャラライトを面光源にした。オプションでスポットライト変更。
    - MaterialSystemのPostprocess優先度を上げた。0->10
    - morph操作時、初期向きをモデルに合わせた。
    - 全身rigモードを追加した。
    - rigの表示位置をターゲットモデルに合わせた。
- 2020/04/23
    - rigとlookAt組み合わせ時、PIEで頭位置がズレるのを修正した。
    - マテリアル一括編集ツールを追加した。
    - 小物Attach用のモードを追加した。
- 2020/04/20
    - サンプルを更新・追加した（リグ、カラーグレーディング、トラッキング）
    - rig生成時、半透明選択を有効化した。
    - rigの上体の位置と回転を分離した。
    - 初期ポーズのON/OFFを選択可能にした。（アタッチがずれる対処）
- 2020/04/18
    - 初期ポーズ機能をONにした。
    - ポーズを微調整するRigを追加した。
    - テンプレートモデルVRoidSimpleを最適化した。リターゲットポーズを追加した。
- 2020/04/13
    - 前回の更新で揺れ骨が動作しなくなったのを修正した。(初期ポーズ機能をOFFにした)
    - glb, gltfのインポート時に座標系がずれるのを修正した。
    - インポート時にpbrマテリアル指定していても、mtoonで出力されることがあるのを修正した。
- 2020/04/11
    - アニメーションの初期ポーズを適用できるようにした。
    - BPの命名に沿わないものをリネームした。
    - ポスト処理による4色カラーグレーディングを追加した。
    - Standaloneでアニメーションが動作しないのを修正した。
    - ARKitのフェイシャル、OVRLipsyncのアニメをコピーできるようにした。
- 2020/04/04
    - 透過ウインドウに制限を入れた（新規ウインドウ実行時のみ有効）
    - UE4.25 gameビルドに対応した。
    - MToonMaterialSystemに自前のPostProcessVolumeを持たせるようにした。
    - Exposureをオーバーライドするオプションを追加した。
    - 一部のモジュールをMacで動作できるようにした。（ただしインポートは不可）
- 2020/03/31
    - 色再現を優先するモードを追加した（FilmicToneMapperをOFF、色逆変換を無効化）
    - デバッグカメラ操作に、Roll回転とZoomを追加した。
    - 揺れ骨簡易設定のデフォルトパラメータを変更した。
    - AnimBlueprintにトラッキング用IKを追加した。UE4.25用にOculusQuestのハンドトラッキングテストを追加した。
- 2020/03/25
    - 揺れ骨を簡易的に再設定できるようにした。デバッグ表示を追加した。
    - リターゲットポーズを追加した。足の開閉パターン2種類ぶん。
    - ライセンス情報が欠けていたのを修正した。
    - インポートウインドウにモデルのライセンスと画像サムネイルを表示するようにした。
- 2020/03/21
    - ポーズコピー元にSkeletalmeshを指定できるようにした。morphを連動させた。
    - ウェイトが無いパーツについて、追従する骨を正しく設定するようにした。
    - ランタイムリターゲット時、RootMotionが反映されないのを修正した。
    - 背景のクロマキー、ウィンドウ透過サンプルを追加した。
- 2020/03/17
    - 風のゆらぎ、座標によるディレイを追加した。
    - 揺れ骨のカテゴリ変更、風を無視する骨を設定できるようにした。
    - ポーズコピー時、フレームレート操作できるようにした。
    - ドキュメントのサンプルBVHを読めるようにした。
- 2020/03/15
    - 多くの機能をAnimControlComponentにうつした。
    - 風を設定できるようにした。
    - 揺れ骨の設定をActorから操作できるようにした。
    - アニメーションのフレームレートを操作できるようにした。
    - Stencil設定用に、モデルを姿勢ごとコピーするActorを追加した。
- 2020/03/12
    - Toonの初期化をConstructionScriptに移動した。
    - UpperChestが無いモデルの視線追従が弱いのを修正した。
    - CustomAnimSequenceのアセット選択候補をリターゲット元のみにした。
    - Morphコントロールをレベル上で確認可能にした。
    - レベル上でアタッチ可能なアクタ VRMModelActorを追加した。
- 2020/03/08
    - 視線追従Actorのターゲットを修正した。
    - PIEでキャラライトを設定すると 座標がずれるのを修正した。
    - UE4.25pre1に対応した。
- 2020/03/04
    - Skeletonのマージをした後、VRMSpringBoneで停止することがあるのを修正した。
- これ以前のものはこちら → https://github.com/ruyo/VRM4U/blob/master/CHANGELOG.md

