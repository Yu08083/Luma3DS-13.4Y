:<<"::BASH_BEGIN"
@echo off
setlocal EnableExtensions
title Luma3DS v13.4Y build
cd /d "%~dp0"

set "RC=0"
set "BASH="
if defined DEVKITPRO if exist "%DEVKITPRO%\msys2\usr\bin\bash.exe" set "BASH=%DEVKITPRO%\msys2\usr\bin\bash.exe"
if not defined BASH if exist "C:\devkitPro\msys2\usr\bin\bash.exe" set "BASH=C:\devkitPro\msys2\usr\bin\bash.exe"
if defined BASH goto :run
echo [ERROR] devkitPro no bash.exe ga mitsukarimasen.
echo   devkitPro wo irete kudasai: https://devkitpro.org/wiki/Getting_Started
echo   C:\devkitPro igai ni ireta baai wa, saki ni  set DEVKITPRO=D:\devkitPro  to shite kudasai.
set "RC=1"
goto :done

:run
set "CHERE_INVOKING=1"
set "MSYSTEM=MSYS"
"%BASH%" -l "%~nx0" %1
set "RC=%ERRORLEVEL%"

:done
echo.
if "%~1"=="" pause
exit /b %RC%
::BASH_BEGIN
set -Eeuo pipefail

SEL="${1:-1}"
case "$SEL" in
  1|2|all|v13.4|v13.4Y) MODE=v13.4 ;;
  4|clean) MODE=clean ;;
  *) echo "!! 1・2（ビルド）か4（clean）を指定してください（指定値: $SEL）" >&2; exit 1 ;;
esac

: "${DEVKITPRO:?DEVKITPRO が設定されていません}"
: "${DEVKITARM:?DEVKITARM が設定されていません}"

ROOT="$(cd "$(dirname "$0")" && pwd)"

case "$ROOT" in
  *[!A-Za-z0-9/._-]*)
    echo "!! このフォルダのパスに、make が扱えない文字が含まれています:" >&2
    echo "     $ROOT" >&2
    echo "   空白・括弧・日本語などを含まない、英数字と _ - . だけのパスに移してください。" >&2
    exit 1 ;;
esac

CACHE="$ROOT/.libctru"
DIST="$ROOT/3DS"
LOG="$ROOT/build.log"
: > "$LOG"

V134_DIR="$ROOT/Luma3DS_v13_4"
V134_CTRU=v2.7.0

V134_FIRM="$V134_DIR/boot.firm"

if [ ! -f "$V134_DIR/Makefile" ]; then
  echo "!! Luma3DS_v13_4/Makefile が見つかりません。build.bat と同じ場所に Luma3DS_v13_4 フォルダを置いてください。" >&2
  exit 1
fi

STAGE="準備"
trap 'echo; echo "!! ${STAGE}に失敗しました。詳細は build.log を確認してください。" >&2' ERR

if [ "$MODE" = clean ]; then
  STAGE="make clean"
  printf 'v13.4Y を make clean 中...'
  make -C "$V134_DIR" clean >> "$LOG" 2>&1
  echo " 完了"
  trap - ERR
  exit 0
fi

if ! command -v firmtool >/dev/null 2>&1; then
  for d in \
      "$HOME/.local/bin" \
      /c/Users/*/AppData/Local/Programs/Python/Python3*/Scripts \
      /c/Users/*/AppData/Roaming/Python/Python3*/Scripts \
      /c/Python3*/Scripts \
      /mingw64/bin
  do
    if [ -x "$d/firmtool" ] || [ -x "$d/firmtool.exe" ]; then
      PATH="$PATH:$d"
      break
    fi
  done
fi
export PATH

need() {
  local tool="$1" probe="${2:-}" path out rc=0
  path="$(command -v "$tool" 2>/dev/null || true)"
  if [ -z "$path" ]; then
    echo "!! $tool が見つかりません。" >&2
    return 1
  fi
  out="$("$tool" $probe 2>&1)" || rc=$?
  if [ "$rc" -ge 126 ] && [ -z "$out" ]; then
    echo "!! $tool は $path にありますが起動できません（必要な DLL が足りない可能性があります）。" >&2
    return 1
  fi
  printf '%-10s %s\n' "$tool" "$path" >> "$LOG"
}

toolfail=0
need git --version   || toolfail=1
need make --version  || toolfail=1
need makerom         || toolfail=1
need firmtool --help || toolfail=1
if [ "$toolfail" != 0 ]; then
  echo "必要なツールが揃っていません。" >&2
  exit 1
fi

count_sources() {
  find "$@" -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.s' -o -name '*.S' \) 2>/dev/null | wc -l
}

libctru_revision() {
  case "$1" in
    v2.7.0) printf '%s\n' 36fe1ada5b7ebe53ba4decda36d764a55f8fefb6 ;;
    *) echo "!! 未対応のlibctruバージョンです: $1" >&2; return 1 ;;
  esac
}

libctru_cached() {
  local prefix="$CACHE/$1"
  [ -s "$prefix/lib/libctru.a" ] &&
    [ -d "$prefix/include" ] &&
    [ -f "$prefix/.revision" ] &&
    [ "$(<"$prefix/.revision")" = "$(libctru_revision "$1")" ]
}

fetch_libctru() {
  local ver="$1" src="$CACHE/src-$1" revision
  revision="$(libctru_revision "$ver")"
  LIBCTRU_FILES=0
  libctru_cached "$ver" && return 0
  rm -rf "$src"
  mkdir -p "$src"
  git -C "$src" init --quiet
  git -C "$src" remote add origin https://github.com/devkitPro/libctru
  git -C "$src" fetch --quiet --depth 1 origin "$revision" >> "$LOG" 2>&1
  git -C "$src" checkout --quiet --detach FETCH_HEAD >> "$LOG" 2>&1
  [ "$(git -C "$src" rev-parse HEAD)" = "$revision" ] || return 1
  sed -i 's/-Werror//g' "$src/libctru/Makefile"
  LIBCTRU_FILES=$(( 2 * ( $(count_sources "$src/libctru/source") + $(find "$src/libctru/data" -name '*.bin' | wc -l) ) ))
}

build_libctru() {
  local ver="$1" prefix="$CACHE/$1" src="$CACHE/src-$1"
  libctru_cached "$ver" && return 0
  make -C "$src/libctru" -j"$(nproc)"
  rm -rf "$prefix"
  mkdir -p "$prefix"
  cp -a "$src/libctru/include" "$src/libctru/lib" "$prefix/"
  libctru_revision "$ver" > "$prefix/.revision"
  rm -rf "$src"
}

COMPILED='^[A-Za-z0-9_.+-]+\.(c|cpp|s|S|bin)$|^arm-none-eabi-(gcc|as) .*\.(c|s|S)( |$)'

progress() {
  local label="$1" total="$2" n=0 pct last=-1 line
  while IFS= read -r line; do
    line="${line%$'\r'}"
    [[ $line =~ $COMPILED ]] || continue
    n=$(( n + 1 ))
    pct=$(( n * 100 / total ))
    [ "$pct" -gt 99 ] && pct=99
    [ "$pct" = "$last" ] && continue
    last=$pct
    printf '\r%s をビルド中... %d%%' "$label" "$pct"
  done
}

build_version() {
  local label="$1" dir="$2" ver="$3" target="$4" firm="$5" total
  STAGE="$label のビルド"
  printf '%s をビルド中... 0%%' "$label"
  fetch_libctru "$ver"
  total=$(( LIBCTRU_FILES + $(count_sources "$dir") ))
  {
    trap - ERR
    build_libctru "$ver"
    make -C "$dir" clean || true
    CTRULIB="$CACHE/$ver" STRICT="${STRICT:-0}" GIT_CEILING_DIRECTORIES="$ROOT" make -C "$dir" "$target"
  } 2>&1 | tee -a "$LOG" | progress "$label" "$total"
  if [ ! -s "$firm" ]; then
    echo
    echo "!! $label: boot.firm が生成されていません ($firm)" >&2
    return 1
  fi
  printf '\r%s をビルド中... 100%%\n' "$label"
}

report_kext() {
  local elf="$1" base="$2" label="$3" end size
  [ -f "$elf" ] || return 0
  end="$("$DEVKITARM/bin/arm-none-eabi-nm" "$elf" | awk '$3 == "__end__" { print $1 }')"
  [ -n "$end" ] || return 0
  size=$(( 0x$end - base ))
  printf '%s k11_extension = %d bytes (0x%X)\n' "$label" "$size" "$size" >> "$LOG"
  if [ -n "${K11_MAX:-}" ] && [ "$size" -gt "$((K11_MAX))" ]; then
    echo "!! $label: k11_extension が K11_MAX ($((K11_MAX)) bytes) を超えています" >&2
    exit 1
  fi
}

build_version "v13.4Y" "$V134_DIR" "$V134_CTRU" all "$V134_FIRM"
report_kext "$V134_DIR/k11_extension/k11_extension.elf" 0x70000000 "v13.4Y"

STAGE="3DS フォルダの作成"
rm -rf "$DIST"
mkdir -p "$DIST/3ds/luma/13.4"
cp "$V134_FIRM" "$DIST/boot.firm"
cp "$V134_FIRM" "$DIST/3ds/luma/13.4/boot.firm"

{
  echo
  echo "=== 要確認の警告（-fpermissive が隠していた型の問題） ==="
  grep -hoE "\[-W(int-conversion|incompatible-pointer-types|implicit-function-declaration|implicit-int|return-mismatch|return-type)[^]]*\]" "$LOG" \
    | sort | uniq -c | grep . || echo "なし"
} >> "$LOG" 2>&1

trap - ERR
echo
echo "完了しました。3DS フォルダの中身を SD カードのルートにコピーしてください。"
