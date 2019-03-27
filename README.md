# VRM4U

[English Doc](https://github.com/ruyo/VRM4U/blob/master/README_en.md)

## はじめに
VRM4U はVRMファイルのインポーター兼ランタイムローダーです


詳細はこちらから
https://github.com/ruyo/VRM4U/wiki/VRM4U

## 特徴
 - VRMファイルをインポートできます。
     - 骨、Morphtarget・BlendShapeGroup、揺れ骨・コリジョン
     - MToonを再現したマテリアル。アウトライン描画には同梱のActorかCharacterを配置し、AssetListを設定する必要があります。
 - Humanoid用のRIGが生成されます。対応アニメーションをそのままリターゲット可能です。

## 動作環境
 - UE4.20.3、UE4.21.0
 - UE4.19も動きますが、マテリアルは生成されません。

## 使い方
- [releases](https://github.com/ruyo/VRM4U/releases/latest)より利用するバージョンのプラグインをダウンロードし、
   「.uproject」とおなじ場所に「Plugings」フォルダを展開してください。

### サンプルマップ
- VRM4UContent/Maps/VRM4U_sample.umap
- ContentBrowserに表示されない場合は、以下の項目を確認ください。
![3](https://raw.githubusercontent.com/wiki/ruyo/VRM4U/images/samplemap.png)

### 使い方
- VRMファイルをドラッグ＆ドロップしてください

![2](https://github.com/ruyo/VRM4U/wiki/images/overview.gif)

## 作った人
[@ruyo_h](https://twitter.com/ruyo_h)

## ライセンス
MIT(VRM4U)

3-clause BSD-License(assimp)

### ソース
UE4アカウントの紐づけが必要です。  
https://github.com/ruyo/UnrealEngine_VRM4UPlugin

https://github.com/ruyo/assimp

公開の体裁を含め 多くの方の情報を参考にさせて頂いています。  
ありがとうございます。

### リリース履歴
- 2019/03/28 マテリアル最適化時、メモリアクセス違反が起きるのを修正した。
    - VRMSpringBoneがPhysicsCollisionに反応するようにした。
    - リターゲットのプレビューにモデルが表示されないのを修正した。（BoundingBoxをセットした）
- 2019/03/24 サンプルマップを追加した。
- 2019/03/23 VRMSpringBoneを仮実装した。輪郭線の描画方法を変更した。
- 2019/03/05 MotionController,LeapMotionによるトラッキングのAnimBPを追加した
- 2019/02/20 マテリアルをまとめるオプションを追加した
    - UE4.22のレイトレース利用時、マテリアルでエラーが出るのを修正した
- 2019/01/07 アセットを大幅に整頓した
    - インポート時にアセットリストを作成するようにした。
    - テンプレートキャラクタを追加した。
    - Exposureが大きい時、キャラの色合いが白くなってしまうのを修正した。
- 2018/12/31 異なるSkeletonにPhysicsAssetをコピーできるようにした
- 2018/12/25 モデルをスケールした時、Shadowmapがずれるのを修正
- 2018/12/16 マテリアルに調整用のMPCを追加した。
- 2018/12/11 揺れ骨を暴れにくくした。PIE中のファイルDrag&Dropに対応した。
- 2018/12/06 パスに日本語が入っているとassimpでエラーが出るのを修正した。不要なアセットを削除した。
- 2018/12/05 androidで動作できるようにした。releasesにビルド結果を追加した。既存のリソースは削除した。
- 2018/11/28 Materialを選択可能にした。HumanoidのRIGを生成するようにした。
- 2018/11/22 インポートオプションを追加した。MToonの未対応パラメータを対処した。
    - インポート時、不要な骨を含めるか選択可能ににした。Skeletonを選択可能にした。
- 2018/11/14 ソースの整頓（assimpを最新へ更新した。AnimBP対応準備）
- 2018/11/08 日本語が文字化けする箇所があったのを修正（骨名、materialslot）
- 2018/11/07 Meshのプリミティブ原点がずれることがあるのを修正
- 2018/11/06 髪が描画されないVRoidモデルがあったのを修正。（Skin以外のMeshを扱えるようにした）
- 2018/11/05 TwoSidedに対応した。アセット名について、日本語名に対応、無効な文字を置き換えるようにした。
- 2018/11/02 VRMv0.36に対応した。マテリアルパラメータの初期カラーを白にした。不要なランタイムロードを呼ばないようにした。
- 2018/10/29 ライセンスをMITに変更した。マテリアルをMToon再現するものに置き換えた。
- 2018/10/27 法線がY-upのままだったのを修正
- 2018/10/21 大幅に整頓＆機能拡張した
    - ライセンス情報を取り込むようにした。
    - アセット名をUE4の命名規則にあわせた
    - MorphTarget名を元データと同じにした（BlendShapeGroup情報はmetaファイルに移動した）
    - Humanoidと同名の骨を持つモデルを出力するようにした（AnimBPのリファレンス用）
    - 大幅にソースを整頓
- 2018/10/18 骨数が256本を越えるとモデルが崩れるのを修正した。揺れ骨を暴れにくくした（InertiaTensorScaleを2にセットした）
- 2018/10/16 VRoidモデルのMorphTargetをまとめた。MorphのY座標がずれていたのを修正。
- 2018/10/11 MToonの半透明情報だけ反映した。
- 2018/10/10 VRoidモデルが白髪になってしまうのを修正。頂点法線が無効だったのを修正。
- 2018/10/08 ドラッグ&ドロップでのインポートに対応。
- 2018/10/07 公開
