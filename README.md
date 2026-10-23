# Luma3DS v13.4Y

Luma3DS_New v1.3 を改修した日本語・English対応版です。改修作者: `@yu_`。ver.y12。更新日: 2026-10-23。

## 導入

[リリース](https://github.com/Yu08083/Luma3DS-13.4Y/releases/latest)の `Luma3DS-v13.4Y.zip` を展開し、中身をSDカードのルートへ上書きします。`boot.firm` と `3ds/luma/9.1/boot.firm`、`3ds/luma/13.4/boot.firm` が入ります。

- Rosalina: L + 十字下 + SELECTで開きます。`Language / 言語` で日本語・Englishを切り替え、Aで保存します。
- Rosalinaの起動ボタン: STARTを押しながら起動し、`Rosalinaを開くボタン...` を選びます。Aでボタンを切り替え、STARTで保存、Bで取り消し、Xで標準へ戻します。
- 起動設定: SELECTを押しながら起動し、Xで言語を切り替えます。その場で反映・保存されます。
- 配色: X→Aで標準の明るい紫・黒背景に戻します。背景テーマ画像は残ります。
- メニュー操作: 長押し開始までの時間・連続移動の間隔などを設定できます。
- 背景テーマ: `SD:/3ds/luma/RosaliaTheme/` に画像を置き、メニューで選びます。PNG・無圧縮BMP・ベースラインJPEG、最大1024×1024・4MB。320×240を推奨します。
- 3GX検査: 元のLuma3DS_New v1.3と同じ検知方式です。警告でAを押して許可した3GXはSDへ記録し、再起動後も保持します。対象領域の内容が変わった場合は再確認します。
- FTP: Wi-Fiへ接続して開始し、接続待ちになったら表示IPのポート `5000` へ接続します。暗号化なし・パッシブモードを指定します。
- Yu_ キャプチャ（New 3DS / New 2DS）: Rosalinaの `Yu_ Capture... / Yu_ キャプチャ...` を開き、Aで開始します。同じネットワークの受信アプリをNTRモードにし、表示された本体IPへ接続します。PCではSnickerstream・Chokistream、AndroidではYu_ キャプチャを使います。開始後はRosalinaを閉じてゲーム画面へ戻ります。Aで停止します。HzMod方式には対応していません。音声転送にも対応します。

画面転送にはTCP `8000`・UDP `8001`、音声にはUDP `8002` を使います。Rosalina操作中・スリープ中・Wi-Fi切断中は一時停止します。

3DS実機での起動・FTP接続・画面転送は未確認です。

yu_ installerのLumaタブから更新できます。ver.y16以降では切り替え用の2ファイルも補完します。更新後は本体を再起動してください。

## ビルド

リリースの `Luma3DS-v13.4Y-source.zip`、またはこのリポジトリのソースを使用します。devkitProの3DS環境、git、make、Python 3、makerom、firmtoolを用意し、空白・日本語を含まないパスで実行します。

```sh
bash build.bat 1
```

Windowsでは `build.bat 1` で実行できます。成果物は `3DS/` に生成されます。libctruは初回に固定バージョンを取得してビルドするため、ネット接続が必要です。

使用する[libctru](https://github.com/devkitPro/libctru)はv2.7.0（`36fe1ada5b7ebe53ba4decda36d764a55f8fefb6`）です。その他のツールの固定版と検証手順は [.github/workflows/build.yml](.github/workflows/build.yml) にあります。

切り替え用v9.1の元ファイルは `assets/legacy/boot-9.1.firm`、対応ソースは `Luma3DS_v9_1/` です。通常のビルドではv13.4Yをビルドし、検証済みのv9.1を同梱します。

## ライセンス

Luma3DSと改変部分は [GPLv3](LICENSE.txt) です。元の著作権・ライセンス表記は各ソースに保持しています。[美咲フォント](assets/fonts/LICENSE.txt)、[テーマ画像デコーダー](third_party/theme/LICENSE.txt)、[無線画面転送](third_party/streaming/LICENSE.txt)、[Mbed TLS](Luma3DS_v13_4/sysmodules/rosalina/third_party/mbedtls/LICENSE)のライセンスも同梱しています。
