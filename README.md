# Luma3DS v13.4Y

Luma3DS_New v1.3 を改修した日本語・English対応版です。改修作者: `@yu_`。更新日: 2026-10-17。

## 導入

[リリース](https://github.com/Yu08083/Luma3DS-13.4Y/releases/latest)の `Luma3DS-v13.4Y.zip` を展開し、`3DS` フォルダーの中身をSDカードのルートへ上書きします。

- Rosalina: L + 十字下 + SELECTで開きます。`Language / 言語` で日本語・Englishを切り替え、Aで保存します。
- 起動設定: SELECTを押しながら起動し、Xで言語を切り替えます。その場で反映・保存されます。
- 配色: X→Aで標準の明るい紫・黒背景に戻します。背景テーマ画像は残ります。
- メニュー操作: 長押し開始までの時間・連続移動の間隔などを設定できます。
- 背景テーマ: `SD:/3ds/luma/RosaliaTheme/` に画像を置き、メニューで選びます。PNG・無圧縮BMP・ベースラインJPEG、最大1024×1024・4MB。320×240を推奨します。
- FTP: Wi-Fiへ接続して開始し、接続待ちになったら表示IPのポート `5000` へ接続します。暗号化なし・パッシブモードを指定します。

3DS実機での起動・FTP接続は未確認です。

## ビルド

リリースの `Luma3DS-v13.4Y-source.zip`、またはこのリポジトリのソースを使用します。devkitProの3DS環境、git、make、Python 3、makerom、firmtoolを用意し、空白・日本語を含まないパスで実行します。

```sh
bash build.bat 1
```

Windowsでは `build.bat 1` で実行できます。成果物は `3DS/` に生成されます。libctruは初回に固定バージョンを取得してビルドするため、ネット接続が必要です。

使用する[libctru](https://github.com/devkitPro/libctru)はv2.7.0（`36fe1ada5b7ebe53ba4decda36d764a55f8fefb6`）です。その他のツールの固定版と検証手順は [.github/workflows/build.yml](.github/workflows/build.yml) にあります。

## ライセンス

Luma3DSと改変部分は [GPLv3](LICENSE.txt) です。元の著作権・ライセンス表記は各ソースに保持しています。[美咲フォント](assets/fonts/LICENSE.txt)、[テーマ画像デコーダー](third_party/theme/LICENSE.txt)のライセンスも同梱しています。
