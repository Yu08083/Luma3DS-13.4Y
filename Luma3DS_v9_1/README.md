# 改変に関する告知 (Modification Notice) — v9.1

本ソフトウェアは Luma3DS を改変したものです。

- 原著作物: Luma3DS (C) Aurorawright, TuxSH, LumaTeam および貢献者
- 3GX プラグインローダー: (C) PabloMK7, Nanquitas ほか
- 本配布物のベース: Luma3DS 3GX フォーク（既存の改変者: ELVESほか）
- ライセンス: GNU General Public License v3.0（同梱の LICENSE.txt）
- 本改変者: ELVES

原著作権表示およびライセンスは保持しています。追加の制限は課していません。本改変部分も GPLv3 に従います。

## 改変履歴 (Change History)

### 2026-07-06 — ELVES
- バージョン切替チェインローダーを追加。Rosalina メニューの「バージョン切替」から SD 上の別バージョン boot.firm を起動する。状態ファイル /luma/chainload_target とフラグ /luma/chainload_ver.flag により v9.1 と v13.4 を相互に切り替える。
- 起動時に自分の版を状態ファイルへ記録するようにし、切替メニューが現在版を正しく判定できるよう修正。
- 書き込み失敗時は再起動せずエラー表示するよう修正。
- チェインロードフラグの処理を CFG_BOOTENV の強制ブート処理より前へ移動（再起動時に確実に発火）。
- クレジット文字列内の不正な制御文字を除去。
- 表示バージョンを v9.1.0 に固定（Makefile）。
- 新しい devkitARM / libctru でビルドするためのビルド設定を変更。

### 2026-07-11 — ELVES
- Old 3DS でのフリーズを修正。Rosalina メニューが OS を凍結したまま APT へ IPC を発行していたため、menuLeave() で凍結を解除してから APT_HardwareResetAsync() を呼ぶよう順序を変更した。バージョン切替・alt-boot 起動・再起動の 3 箇所に適用。
- チェインロード失敗時に無言で現行版が起動していた問題を修正。切替先 boot.firm が不在または破損している場合はエラーを表示して停止する。

### 2026-07-31 — ELVES
- ビルドフラグを修正。C では危険な診断（暗黙の関数宣言・非互換ポインタ・int/ポインタ変換）を
  格下げしてしまう `-fpermissive` を全 Makefile から削除し、それらを常にエラーにする
  `HARDERRORS` を追加。全警告をエラーにする `STRICT=1` を用意した。
- 切替先 boot.firm の存在確認 (`chainloadVersionFirmExists`) がサイズしか見ていなかったのを、
  FIRM マジックも確認するよう修正。
- ビルドスクリプトを整理し、k11_extension のサイズを表示するようにした
  （Old 3DS では BASE メモリ領域から削り取られるため、肥大すると O3DS だけ起動に失敗する）。

### 2026-08-01 — ELVES
- 3gx の世代チェックを追加。`Check_3gx_Magic()` を `3gx.c` に実装し、
  `PluginLoader__Load()` で `Read_3gx_Header()` の前に呼ぶようにした。
  これまで magic を一切見ずにヘッダを読んでいたため、0002 世代 (0.7.x 系) の 3gx を
  読ませると壊れたオフセットを掴んで fs プロセスが飛んでいた。
  上流 v13.4 と同じロジック・同じエラー文言。

### 2026-08-01 (2) — ELVES
- 3gx セレクターで世代の合わないプラグインを選べないようにした。
  `EnumeratePluginFiles()` で各ファイルの先頭を読んで `Check_3gx_Magic()` にかけ、
  結果を `g_pluginEntryOk[]` に保持する。セレクターでは非対応を赤字 + "(incompatible)"
  で表示し、カーソルが乗らないようにした。
  1件だけのときの自動選択と、`.selected3gx` に記憶した選択も対応版に限定。
  フォルダ内が全部非対応なら、そもそもプラグインを読み込まない。

### 2026-08-01 (3) — ELVES
- Rosalina の未翻訳だった表示を日本語化（プラグイン選択、メモリビューア、チート、
  エラー表示など 40 箇所）。
- `font_jp.h` を再生成（153 → 342 文字）。生成には既存の `tools/genfont_jp.py` を使用。
  両ツリーまとめて回す `tools/gen_font_jp.sh` を追加した。
- `process_list.c` の重ね描き座標を訳文の文字位置に合わせて修正。

### 2026-08-01 (4) — ELVES
- 表示言語を英語／日本語で切り替えられるようにした（`i18n.c` / `i18n.h`）。
  ソースの文字列は英語に戻し、表示時に `TR()` を通す方式。選択は
  `SD:/luma/rosalina_lang.txt` に保存し、次回起動時に読む。
- Rosalina メニューに「Language」を追加。
- 以前から日本語だった箇所も対訳表に移した（英語原文は上流 v9.1 から復元）。
- `process_list.c` の重ね描き位置を `I18n_Columns()` で実文字列から計算するようにし、
  英語・日本語のどちらでもずれないようにした。

### 2026-08-01 (5) — ELVES
- `loadVersionFirm()` が目標＝自分自身のときにチェインロードしないようにした。
  `SD:/boot.firm` がこの版そのものだと、同じ FIRM を自分の上に読み直すことになり
  意味が無いうえ、途中で止まると画面が出ないまま固まる。

### 2026-08-01 (6) — ELVES
- 状態ファイルを `/luma/chainload_ver.flag` の 1 本に統合。中身が目標バージョン文字列。
  ARM9 が毎起動 `chainload_target` を書いていたのを廃止し、SD への書き込みを
  「切り替え実行時の 1 回」だけにした（SD 破損の機会を減らすため）。
- Rosalina は現在版をコンパイル時定数 `MY_LUMA_VERSION` で持つようにし、
  現在版の判定で SD を読まないようにした。

### 2026-08-01 (7) — ELVES
- 3gx セレクターで X を押しながら起動すると、セレクターを開かず
  `/luma/plugins/default.3gx` を使うようにした。

### 2026-08-01 (8) — ELVES
- 日本語化を撤回し、表示は英語一本にした。`i18n.c` / `i18n.h` / `i18n_table.h` /
  `font_jp.h` と生成ツールを削除し、`draw.c` を上流 v9.1 に戻した
  （GCC14 で通らない `(u32)` キャストのみ再適用）。
- 破壊 3gx チェッカーの検査前の画面フラッシュを削除（v13.4 と挙動を揃えるため）。

### 2026-08-01 (9) — ELVES
- `draw.c` を上流 v9.1 に戻した際に、libctru の型名変更への追従
  (`GSPGPU_FramebufferFormats` → `GSPGPU_FramebufferFormat`) まで巻き戻していたのを修正。
  上流 v9.1 は 2018 年のコードなので旧名のまま。新しい libctru では通らない。

### 2026-08-01 (10) — ELVES
- `menus.c` に足していた `#include <string.h>` を削除。Luma の `memory.h` が
  memcpy/memcmp/strlen などを独自シグネチャ (u32) で宣言しており、newlib の
  string.h と衝突する。これらは memory.h 側で揃うので string.h は不要。
- バージョン切替画面に残っていた `targetIs13` の参照を `MY_LUMA_VERSION` / `target` に修正。

### 2026-08-02 — ELVES
- **配列外書き込みの修正。** `utf16_to_utf8()` は出力バッファに収まらなくても -1 を返さず、
  「本来必要だった長さ」を返す（libctru の実装で確認）。返り値をそのまま
  `g_pluginEntries[n][units] = 0` の添字に使っていたため、長いファイル名で
  配列外に書き込む可能性があった。`FS_DirectoryEntry.name` は 262 UTF-16 単位なので
  UTF-8 では最大 783 バイトになりうる（行は 256 バイト）。
  上限を明示し、超える名前は候補から外すようにした。上流 Luma3DS にも同じ形が残っている。
- `sprintf` に 2 次元配列の要素を直接渡すのをやめ、長さの分かる局所配列を経由するようにした
  （`-Wformat-overflow` の警告対策）。
- `IsPluginLoadable()` のパスバッファを `g_path` と名前の合計に合わせた。

## 改変したファイル (Modified Files)

- source/firm.c, source/firm.h
- source/main.c
- sysmodules/rosalina/source/menus.c
- sysmodules/rosalina/source/3gx.c
- sysmodules/rosalina/include/3gx.h
- sysmodules/rosalina/source/plgloader.c
- sysmodules/rosalina/include/menus.h
- Makefile, k11_extension/Makefile

## ビルド要件

- devkitARM / libctru 2.2.2 / armips / makerom / firmtool
