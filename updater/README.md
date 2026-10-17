# Luma3DS 13.4Y Updater

HOMEメニューからLuma3DS v13.4Yを更新する専用CIAです。作者: `@yu_`。

CIAをインストールして起動し、Wi-Fiへ接続した状態でAを押すと最新版を確認します。Aで内容を確認し、さらにAで更新します。Xで日本語・Englishを切り替えます。Bで戻る・中止、STARTで終了します。

更新先はSDルートの `boot.firm` です。更新前のファイルは `boot.firm.13.4Y.bak` に保存します。設定・テーマ・セーブデータは保持します。

本体が起動しない場合は、先に修復版 `boot.firm` をSDルートへコピーしてください。

## ビルド

devkitARM、libctru 2.7.0、makerom、bannertoolを用意して実行します。

```sh
make -C updater
```

成果物は `updater/Luma3DS-13.4Y-Updater.cia` です。固定バージョンとビルド手順は [.github/workflows/updater.yml](../.github/workflows/updater.yml) にあります。

Luma3DSと更新処理は [GPLv3](../LICENSE.txt)、[美咲フォント](../assets/fonts/LICENSE.txt)と[Mbed TLS](../Luma3DS_v13_4/sysmodules/rosalina/third_party/mbedtls/LICENSE)の条件も維持しています。
