# VRM4U

[English Doc](https://github.com/ruyo/VRM4U/blob/master/README_en.md)

## はじめに
VRM4UはUnrealEngine5(UE5, UE4)で動作する、VRMファイルのインポーターです。

**使い方は [こちらのページにあります](https://ruyo.github.io/VRM4U/)**

開発・サポートは [@はるべえ](https://twitter.com/ruyo_h) が個人で対応しています。そのためレスポンスが遅いことがあります。

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
 - UE5.3〜5.0, UE4.27〜UE4.20
 - Windows
 - Mac(要プロジェクトビルド)
 - mobile(Android, iOS)
 - UE4.19も動きますが、マテリアルは生成されません。

## 使い方
- [releases](https://github.com/ruyo/VRM4U/releases/latest)より利用するバージョンのプラグインをダウンロードし、「.uproject」とおなじ場所に「Plugins」フォルダを展開、以下のように配置ください

```
 + MyProject
   - MyProject.uproject
   - Plugins
     - VRM4U
       - VRM4U.uplugin
```

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

https://ruyo.github.io/VRM4U/10_detail/

## 作った人
[@ruyo_h](https://twitter.com/ruyo_h)

## ライセンス

|||
|----|----|
|MIT|VRM4U|
|MIT|[RapidJSON](https://github.com/Tencent/rapidjson/)|
|3-clause BSD-License|[assimp(orig)](https://github.com/assimp/assimp), [assimp(vrm4u fork)](https://github.com/ruyo/assimp)|

### ソースが必要なケース＆入手方法

- Windows向け
    - 特別な操作は不要です。zipを展開 or このリポジトリをクローンして利用ください。(assimpはビルド済のlibが入っています)
- Mac向け
    - カスタム版assimpのソース取得＆ビルドが必要です
- モバイル向け
    - 一般的な利用方法であれば、特別な操作は不要です。
    - ランタイムロードを利用する場合は カスタム版assimpのソース取得＆ビルドが必要です。

カスタム版assimpは[こちら](https://github.com/ruyo/assimp)より入手ください

- 2023/11以前のプラグインを利用する場合
    - こちらより昔のソースを取得ください https://github.com/ruyo/UnrealEngine_VRM4UPlugin
    -  （404エラーページが出る場合は[こちらより紐付けをしてください](https://www.unrealengine.com/ja/blog/updated-authentication-process-for-connecting-epic-github-accounts)）

## 開発支援

VRM4Uの開発を支援したい という奇特な方は[こちら](https://ruyo.booth.pm/items/1707224)からどうぞ


公開の体裁を含め 多くの方の情報を参考にさせて頂いています。  
ありがとうございます。

### リリース履歴
- 2024/03/07
    - アセットのサムネイルが真っ黒になってしまうのを修正
    - Humanoid用Skeletonがコンテンツブラウザに表示されないのを修正
- 2024/02/12
    - IKRigにIKBone設定を追加
    - Rig変換中にダイアログ表示を追加
    - シーンキャプチャサンプルを追加
- 2023/11/27
    - **注意）現バージョンより、インポート時に骨やMorphTargetの特殊文字（スペース「 」やピリオド「.」）をアンダースコア「_」に置き換えます。UE標準動作にあわせています。不都合ある場合はインポートオプションより無効化できます。**
    - IKRetargeterにAポーズを自動で追加
    - NormalMapを2つ設定できるようにした
    - Aポーズを微調整した
    - UE5.3でMorphTargetが動かなかったり、フェイシャルのPoseアセットがクラッシュするのを修正
    - VRM10: VRM10として判定されないファイルがあったのを修正
- 2023/11/05
    - リポジトリをpublicに移動。ビルドスクリプトを修正
- 2023/10/30
    - マテリアルパラメータを変更した時にクラッシュすることがあるのを修正
    - Macでのインポート時にクラッシュするのを修正
- 2023/10/08
    - インポート時に骨名を有効な文字列のみに変更するオプションを追加
    - Rig生成のWBPでAssetが自動設定されないことがあるのを修正
    - MorphTargetWBPでエラーが出ることがあるのを修正
- 2023/09/22
    - ControlRigテンプレートについて、足首の位置がずれてしまうのを修正
    - TwoSidedのマテリアルについて法線をカメラ向きに合わせるよう修正
    - UE5向けのMorphTargetWBPを追加
    - UE5.3 MorphTargetに対応するCurveをSkeletonに含めるよう修正
- 2023/09/07
    - UE5.3 正式版に対応
- 2023/08/25
    - VRMA: 再生フレームレートがずれるのを修正
    - UEFN: MorphTargetを出力できるようにした
- 2023/08/13
    - VRMAファイルのインポートに対応
    - ZOffsetにマスクテクスチャを指定を追加
    - リインポート時、Skeletonのカーブ名が重複してしまうのを修正
    - UE5.2 ランタイムロード時、モデルが歪んでしまうのを修正（スキンのウェイトがずれるのを修正）
    - UE5.2 AllRigの生成(モーフ処理)に失敗するのを修正
    - UE5.3preview1 BPでエラーが出ているのを修正
- 2023/07/23
    - インポート時、Skeletonリネームするとクラッシュするのを修正した
    - matcapを2枚設定できるようにした
    - 白目補正マテリアルの中心が 黒フェードしてしまうことがあるのを修正した
    - UE5.2 ポーズアセットを正しく出力できないのを修正した
    - UE5.2 bvhインポートでクラッシュするのを修正した
    - UE5.3preview1 に仮対応した
- 2023/07/16
    - 骨を省略しているモデルをインポートをすると、クラッシュすることがあるのを修正した
- 2023/07/05
    - リインポートに対応した
    - Skin以外のメッシュを含むモデルが動作するようを修正した（骨のActiveリストに含まれるよう修正した）
    - 白目補正のマテリアルが壊れているのを修正した
    - アニメーション補助用のControlRigを追加した
    - UE5.0以降では 従来のBoneMappingContainerを設定しないようにした
    - UEFN: migrate専用のインポートオプションを追加した
- 2023/06/23
    - UE5.2でVRMSpringBoneノードを利用するとアニメーションが動かなくなることがあるのを修正した
    - BodyRigを別モデルに適用した時、クラッシュすることがあるのを修正した
    - VRM10: インポート時にモデルが壊れることがあるのを修正した（頂点最適化を無効化した）
- 2023/05/30
    - 骨数が多いモデルをインポートするとメッシュが描画されないことがあるのを修正した
    - VRM10: ランタイムロード時 クラッシュするモデルがあるのを修正した
- 2023/05/17
    - UE5.2の不具合を修正した。インポート時のウェイト修正、ControlRig生成時のクラッシュ修正
    - 骨のActiveリストを正しくした
    - Linuxビルドエラーを修正した。
- 2023/05/12
    - UE5.2 対応したが、モデルによって新規インポートモデルのウェイトが壊れる不具合がある。migrateした場合は問題ない。
    - 白目補正をmaskマテリアルに切り替えた
    - 自前シャドウマップの方向を任意に指定できるようにした
    - VRM10: ラインタイムロード時、コンストレイントを動作するようにした。
- 2023/04/23
    - UE5.2previewに対応した
- 2023/03/21
    - main/shadeカラーを同時にスケールできるようにした
    - MatCapにマスク、スケールを追加した
    - EyeWhiteがカラースケールできるできるようにした
    - EyeWhiteがDOFの影響をうけるようにした
    - Metallic,Roughnessにmin/maxを設定できるようにした
    - NormalMapにマスクを設定できるようにした
    - VRM10: アウトラインなし設定を読み込めるようにした
    - VRM10: ランタイムロード時、ローカル軸回転を無効化するようにした
- 2023/02/18
    - VRM0のインポートでクラッシュするのを修正した
- 2023/02/17
    - VRM10: クラッシュするモデルがあるのを対処した
    - VRM10: 全てのコンストレイントに対応した
- 2023/02/08
    - UE5.1.1でクラッシュするためバイナリをリビルドした
    - リターゲットノードの親としてアタッチ先モデルを参照できるようにした
    - VRM10: 一部のコンストレイントに対応した
- 2023/01/21
    - mocopi 周期的にTポーズが出てしまうのを修正した
    - mocopi receiverにローカルIPアドレスを列挙する機能を追加した
    - mocopi ワールド座標でのデバッグ表示位置がずれているのを修正した
- 2023/01/16
    - mocopiパケットの読み込み時、フレーム情報だけを読取るようにした。
    - mocopiのモーションをバッファリングできるようにした。
    - VMC受信したBlendShapeをTakeRecorderで記録できないのを修正した。
- 2023/01/13
    - WBPからのVMC受信が動作しないのを修正した
    - IKRetargeterパネルに5.0でのみ利用可のコメントを追加した
    - VRM10: モデルをTポーズで読み込めるようにした
- 2023/01/03
    - mocopiのパケットが分割受信された際も動作するようにした
    - VMCからPerfectSyncを受け取れないのを修正した(追加ぶんのBlendShapeGroupがインポートできないのを修正)
    - assimpを更新した（5.2.5）
- 2022/12/31
    - mocopiReceiverを追加した [解説はこちら](https://ruyo.github.io/VRM4U/08_mocopi/)
    - Captureモジュールを追加、VMCとmocopiの処理を移動させた
    - BVHインポート時、データによってメッシュ位置がずれてしまうのを修正した。
    - BVHインポート時、アニメーションフレームレートを指定できるようにした
- 2022/12/11
    - PostToonにDOFがかかるようにした（RenderAfterDOFをOFFにした）
    - Unlitマテリアルにメインライトカラーを反映できるようにした
    - DirectionalLightが無い時、ダミーデータを渡すようにした
    - UE5.1 IKRetarget用のWBPがエラーになるのを修正
- 2022/11/23
    - SunSky利用時、MToonUnlitが真っ黒になるのを修正した（光源が太陽光のような大きい値でもMToonUnlitが動作するようにした）
    - VMCプロトコルでのパーフェクトシンクを反映できるようにした（対応するBlendShapeGroupが無い場合、直接MorphTargetとして扱うようにした）
    - VirtualShadowmapBiasが保存されないのを修正した
    - UE5.1 ControlRigのうち不要なControllerが無効化されないのを修正した（無効なControllerをスケールを0にした）
- 2022/11/16
    - UE5.1に対応した。ビルドバイナリのみ追加した
- 2022/11/03
    - インポートオプションにて、無効な三角ポリゴンを削除できるようにした。
    - シフトレンズカメラのデバッグ表示に対応した。
    - VRM10: モデルによってインポート時にクラッシュすることがあるのを修正した。
- 2022/10/24
    - UE5.1Preview2に対応した。すべての機能が動作するようにした。
    - UE5.0 インポート時にプログラムがBreakするのを対処した。
    - VirtualShadowmapパラメータを保存できるようにした。
    - シフトレンズ対応カメラを追加した(VRMCineCamera)
    - インポートオプションを追加した。最適化用。
    - VMC受信について、Widgetを経由せず動作できるようにした。
    - PBRマテリアルでインポートした際、ノーマル、ラフネス、メタリックを読み込むようにした。
    - PerBoneMotionBlurを有効化した
- 2022/10/10
    - UE5.1Previewに暫定対応した。一部のBPやWidgetは動作しない。
    - VMCプロトコルを複数ポートで受信した際にうまく動作しないのを修正した。
    - モデルインポート時、GameThreadのタスクで警告(Break)が出るのを修正した。
    - VRM10: カプセルコライダを使っているモデルを読むとクラッシュするのを修正
    - VRM10: 読み込み時の姿勢を Bind/Restポーズから選択できるようにした（ただし頂点はBindポーズのまま）
- 2022/09/15
    - EBPにVirtualShadowmapのパラメータを追加した。セルフシャドウのアーティファクト対策
    - ControlRigにベイクする際、MorphTargetの情報もベイクできるようにした
    - VMCプロトコルのRoot座標が正しく反映されていないのを修正した
    - VMCプロトコルのサンプルマップを開いた時、OSCプラグインをチェックするようにした
- 2022/08/28
    - VrmSpringBoneノードが表示されないことがあるのを修正した
- 2022/08/09
    - AllRigを追加した（BodyRigとMorphRigを統合したもの）
    - assimpを更新(5.2.4)
    - VMCプロトコルでキャラクタを動かす簡易Actorを追加した
    - VMCプロトコルのBlendShape, Root座標を反映するようにした
- 2022/07/25
    - アニメーションをBodyRigにベイクした際、FKのControllerにキーが入らないのを修正した
    - BodyRigのGizmoがずれている箇所を修正した
- 2022/07/11
    - アニメーションをBodyRigにベイクした際、Rigの回転軸がずれるのを修正。不要なControlを無効化した
    - 不正なVRMファイル読み込みでクラッシュするのを修正した
- 2022/07/05
    - BlendShapeでマテリアルパラメータアニメーションにて、不正なデータでクラッシュすることがあるのを修正した
    - ToneCurveAmountによる色補正機能を追加した
- 2022/05/30
    - Aポーズの左足首が数度だけ左右非対称になっていたのを修正した
    - PMX向けにControlRigを作成した際、親指が伸びてしまうのを修正した
- 2022/05/13
    - UE5.0 自動生成IKRigのタイプミスを修正した (RightPinky)
    - UE5.0 Body用ControlRigへのベイクが動作しないのを修正した
    - FovFix時、Outlineが正しく変形しないことがあるのを修正した
- 2022/05/02
    - UE5.0 ランタイムロード時、MorphTargetが読み込まれないのを修正した。
    - UE5.0 骨名にスペースがある時、ControlRigが動作しないのを修正した。
    - アウトラインが無いパーツについて、パラメータ設定後にアウトラインが出るようにした。
- 2022/04/27
    - UE5.0 Body用Rigに機能追加。指・目のリグを追加した。手首と肘の補正を追加した。
- 2022/04/22
    - UE5.0 UE5のBody用ControlRigをVRM対応したものを追加した。変換用Widgetを整理した
    - UE5.0 インポート時、サムネイルやコンテンツブラウザの表示が更新されないのを修正した
    - UE5.0 IKRig自動生成時、Armに肩が入っていたのを、上腕からに修正した
    - UVRotateの計算方法を UEノードのみで組むよう変更した
- 2022/04/16
    - UE5.0 リターゲット用アセットをUE5基準にあわせた。FullIK設定を追加した。[解説はこちら](https://ruyo.github.io/VRM4U/03_gray/)
    - UE5.0 EpicSkeleton用に生成するIKRigをUE5基準にあわせた。
    - A-poseを更新した。手指までグレイマンに近い姿勢に置き換えた。
    - ランタイムリターゲットActor利用時、不要なActorが残ることがあるのを対処。
- 2022/04/06
    - UE5.0 正式版に対応
- 2022/04/05
    - **注意）インポート済のデータがある場合、VrmAssetListでアセット参照警告が出ることがあります。アセットの再保存で解消されます。**
    - UE5.0 インポート時、IKRetargeter用アセットを自動生成
    - UE5.0 EpicSkeletonからVRMへ、リターゲット用アセットを自動生成
    - 調整パネルより、マテリアルをユーザ指定のものに差し替えできるようにした
- 2022/03/17
    - UE5.0 BVHインポートが動作しないのを修正した
    - UE5.0 IKRigアセットを生成するようにした（VRM, EpicSkeleton両対応）
    - PMXインポートでクラッシュするのを修正した
    - AssetListObjectのカラーを白に寄せた
- 2022/03/08
    - UE5.0 VRM4Uの全機能が動作するようにした。
    - UE5.0 インポート後、エディタ再起動するとMorphが消えるのを修正した
    - UE5.0 MorphRigの保存時に停止するのを修正した
    - UE5.0 PKGでのランタイムロードが停止するのを修正した。
    - Mac インポート時に停止するのを修正した。
    - 古いUniVRMで出力するとアルファマスクが抜けないのを修正した（OpaqueとCutoffの両方指定がある場合Maskとして扱うようにした）
- 2022/02/28
    - UE5.0でControlRigコピースクリプトが動作するよう修正した。（ただUE5.0でのMorphTargetはうまく動作しない）
    - IKRigに不要な腰回転があるのを修正した。
    - ランタイムロードで停止するモデルがあったのを修正した。
    - アルファマスクが抜けないのを修正した（opaque判定が間違っているのを修正）
    - assimpを更新(v5.2.2)
- 2022/02/23
    - **注意）5.0のプラグインをPreview対応版に差し替えました。この先はEA版をサポートしません。必要あれば20220214版をご利用ください。**
    - UE5.0preview版に対応した。
    - 5.0でのインポート時、SSSProfileを初期値で作成するようにした。ノイズが出るため。
    - プラグインの配置場所を自由にできるようになった。
- 2022/02/14
    - **注意）MToonMaterialSystemでのShadeShift調整効果を正負反転し、VRM10の挙動に合わせています。全体補正を利用している場合は従来パラメータの正負を反転ください。未使用であれば影響ありません。**
    - 同名のファイルインポート時、リインポートか強制上書きを選択できるようにした。初期設定ではリインポート。
    - プラグイン側のJSON parserを置き換えた「JSON for Modern C++」-> 「RapidJSON」
    - VRM10: モデル法線が反転していたのを修正した。
    - VRM10: 最新のMToonパラメータに対応した。（Linear色空間の対応、ShadeShiftとToonyの処理変更）
- 2022/02/04
    - リターゲット時、スケールや移動量を調整できるようにした。
    - リターゲットの骨名やリファレンスポーズを指定できるようにした。骨名の対応リスト出力ツールを追加した。
    - Macのビルド設定をStaticLibに切り替えた。
- 2022/01/28
    - グレイマンからのリアルタイムリターゲット機能を追加した [使い方](https://ruyo.github.io/VRM4U/03_gray/)
    - リムテクスチャ、UVスクロールマスクテクスチャに対応した。
    - MToonAttachにて不要なDynamicMaterialが生成されるのを対処した。
    - FKRig関連のActorをリネーム、整頓した。
    - Linuxビルドに対応した。
- 2022/01/20
    - サンプルのモデルがUE4.20～4.25で読めないのを修正した。
    - ue4mannequinにリネームしたSkeletonで、ContorolRigによる視線操作が動かないのを修正した。
    - 新FKRigシステム、サンプルマップを追加した。
    - 開発版UE5の警告を対処した。
- 2022/01/08
    - VMCによる姿勢の回転軸が一部間違っていたのを修正した。
    - VMCのトラッカーを利用したサンプルを追加した。
- 2022/01/05
    - PostToon、MorphRIgがUE5で動作しないのを修正した。一部のファイルをUE4.27で更新していたのを巻き戻した。
    - VMC によるキャラクタ移動を反映した。身長差によるスケール指定できるようにした。（初期値はx1.0）
- 2022/01/04
    - VMC Protocolに対応した。
    - VRM1:インポート時の正規化（ローカル軸の破棄）をオプションで選択可能にした。データ的にはVRM0と同じ状態で読み込める。
- これ以前のものはこちら → https://github.com/ruyo/VRM4U/wiki/CHANGELOG

