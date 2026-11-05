# Luma3DS v13.5Y

Luma3DS v13.5 に Luma3DS_New v1.3 由来の機能と日本語・English対応を組み込んだ版です。改修作者: `@yu_`。ver.y20。更新日: 2026-11-05。

Yu_ キャプチャの開始中は、Rosalinaメニューの右上に本体IPを表示します。

転送する画面が複雑で画質を下げた場合も、設定した画質への復帰を定期的に試します。

画質90以上では色の細部を保って転送します。画質を維持したままJPEG処理と送信の待ち時間を見直しました。軽さを優先する場合は画質を90未満にします。

New 3DSでは送信待ちの遅れを小さく補正し、圧縮処理の負荷を減らしました。画質と解像度は維持します。

## 導入

[リリース](https://github.com/Yu08083/Luma3DS-13.4Y/releases/latest)の `Luma3DS-v13.5Y.zip` を展開し、中身をSDカードのルートへ上書きします。`boot.firm` と `3ds/luma/9.1/boot.firm`、`3ds/luma/13.5/boot.firm` が入ります。同梱のv9.1から切り替えられるよう、v13.5Yを `3ds/luma/13.4/boot.firm` にも配置します。

[本家v13.5配布版](https://github.com/LumaTeam/Luma3DS/releases/tag/v13.5)の[Homebrew Menu](https://github.com/devkitPro/3ds-hbmenu)（`boot.3dsx`）と `config/ssl/cacert.pem` も同梱しています。

- Rosalina: L + 十字下 + SELECTで開きます。`Language / 言語` で日本語・Englishを切り替え、Aで保存します。
- Rosalinaの起動ボタン: STARTを押しながら起動し、`Rosalinaを開くボタン...` を選びます。Aでボタンを切り替え、STARTで保存、Bで取り消し、Xで標準へ戻します。
- 起動設定: SELECTを押しながら起動し、Xで言語を切り替えます。その場で反映・保存されます。
- 配色: X→Aで標準の明るい紫・黒背景に戻します。背景テーマ画像は残ります。
- メニュー操作: 長押し開始までの時間・連続移動の間隔などを設定できます。
- 背景テーマ: `SD:/3ds/luma/RosaliaTheme/` に画像を置き、メニューで選びます。PNG・無圧縮BMP・ベースラインJPEG、最大1024×1024・4MB。320×240を推奨します。
- 3GX検査: 元のLuma3DS_New v1.3と同じ検知方式です。警告でAを押して許可した3GXはSDへ記録し、再起動後も保持します。対象領域の内容が変わった場合は再確認します。
- FTP: Wi-Fiへ接続して開始し、接続待ちになったら表示IPのポート `5000` へ接続します。暗号化なし・パッシブモードを指定します。
- Yu_ キャプチャ: Rosalinaの `Yu_ Capture... / Yu_ キャプチャ...` を開き、Aで開始します。同じネットワークの受信アプリに、表示された本体IPを入力して接続します。PCではSnickerstream・Chokistream、Android・iOSではYu_ キャプチャを使います。Rosalinaメニューも下画面として転送します。Aで停止します。音声転送にも対応します。

Old 3DS / Old 2DSは低負荷の送信経路を使用します。画質と解像度は設定どおり保ち、撮影頻度とCPU使用時間を抑えます。Old本体の実機動作・到達FPSは未確認です。

画面転送にはTCP `8000`・UDP `8001`、音声にはUDP `8002` を使います。スリープ中・Wi-Fi切断中は一時停止します。

3DS実機での起動・FTP接続・画面転送は未確認です。

v13.5Yへの更新はZIPの全ファイルをSDカードへ上書きし、本体を再起動してください。

## ビルド

リリースの `Luma3DS-v13.5Y-source.zip`、またはこのリポジトリのソースを使用します。devkitProの3DS環境、git、make、Python 3、makerom、firmtoolを用意し、空白・日本語を含まないパスで実行します。

```sh
bash build.bat 1
```

Windowsでは `build.bat 1` で実行できます。成果物は `3DS/` に生成されます。libctruは初回に固定バージョンを取得してビルドするため、ネット接続が必要です。

使用する[libctru](https://github.com/devkitPro/libctru)はv2.7.1（`62b7c42c88b1ee837f513358409d2e5c8f9001a9`）です。その他のツールの固定版と検証手順は [.github/workflows/build.yml](.github/workflows/build.yml) にあります。

makeromは[固定コミット](https://github.com/3DSGuy/Project_CTR/tree/e8f5f529c54ff9b22a2491a480ffa69206bf7b19)を取得し、`python3 tools/patch_makerom.py <Project_CTR>/makerom/src/code.c` を適用してからビルド・配置してください。CIも同じ手順で準備します。

切り替え用v9.1の元ファイルは `assets/legacy/boot-9.1.firm`、対応ソースは `Luma3DS_v9_1/` です。通常のビルドではv13.5Yをビルドし、検証済みのv9.1を同梱します。

## ライセンス

Luma3DSと改変部分は [GPLv3](LICENSE.txt) です。元の著作権・ライセンス表記は各ソースに保持しています。[美咲フォント](assets/fonts/LICENSE.txt)、[テーマ画像デコーダー](third_party/theme/LICENSE.txt)、[無線画面転送](third_party/streaming/LICENSE.txt)、[Mbed TLS](Luma3DS_v13_4/sysmodules/rosalina/third_party/mbedtls/LICENSE)のライセンスも同梱しています。
