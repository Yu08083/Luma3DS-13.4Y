import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import wave
import zlib

root = Path(__file__).resolve().parents[1]
updater = root / 'updater'
generated = root / 'build/updater-ui-tests'
generated.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
source = (updater / 'source/ui.c').read_text(encoding='utf-8')
font_header = root / 'Luma3DS_v13_4/sysmodules/rosalina/include/japanese_font.h'
codepoints = {int(value, 16) for value in re.findall(r'0x([0-9A-F]{4})', font_header.read_text().split('japaneseGlyphs')[0])}
required = {ord(character) for value in re.findall(r'"(?:\\.|[^"\\])*"', source) for character in value if ord(character) >= 128}
assert required <= codepoints, sorted(required - codepoints)
assert not re.search(r'\b(?:malloc|calloc|realloc|free|fopen|fread|FSUSER_|FSFILE_|fontEnsureMapped|fontGetSystemFont)\b', source)
harness = r'''
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.c"

static unsigned checks;
#define CHECK(value) do { assert(value); ++checks; } while (0)

typedef struct Buffer {
    unsigned char bytes[400 * 240 * 3 + 64];
    UpdaterUiSurface surface;
} Buffer;

static Buffer top_a, top_b, bottom_a, bottom_b;

static void buffer_init(Buffer *buffer, unsigned width, unsigned height)
{
    memset(buffer->bytes, 0xA7, sizeof(buffer->bytes));
    buffer->surface = (UpdaterUiSurface){buffer->bytes + 32, (size_t)width * height * 3u, width, height};
}

static void canaries(const Buffer *buffer)
{
    for (unsigned i = 0; i < 32; ++i)
        CHECK(buffer->bytes[i] == 0xA7);
    for (size_t i = buffer->surface.size + 32; i < sizeof(buffer->bytes); ++i)
        CHECK(buffer->bytes[i] == 0xA7);
}

static uint64_t hash(const UpdaterUiSurface *surface)
{
    uint64_t value = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < surface->size; ++i)
    {
        value ^= surface->pixels[i];
        value *= UINT64_C(1099511628211);
    }
    return value;
}

static void snapshot(const char *path, UpdaterUi *ui, UpdaterAppView *view)
{
    buffer_init(&top_a, 400, 240);
    buffer_init(&bottom_a, 320, 240);
    UpdaterUi_Invalidate(ui);
    UpdaterUi_Draw(ui, view, &top_a.surface, &bottom_a.surface);
    FILE *file = fopen(path, "wb");
    CHECK(file != NULL);
    fprintf(file, "P6\n400 480\n255\n");
    for (unsigned y = 0; y < 480; ++y)
        for (unsigned x = 0; x < 400; ++x)
        {
            unsigned char rgb[3] = {0};
            const UpdaterUiSurface *surface = y < 240 ? &top_a.surface : &bottom_a.surface;
            unsigned local_x = y < 240 ? x : x >= 40 ? x - 40 : 400;
            if (local_x < surface->width)
            {
                size_t offset = ((size_t)local_x * 240 + 239 - y % 240) * 3;
                rgb[0] = surface->pixels[offset + 2];
                rgb[1] = surface->pixels[offset + 1];
                rgb[2] = surface->pixels[offset];
            }
            CHECK(fwrite(rgb, 1, 3, file) == 3);
        }
    CHECK(fclose(file) == 0);
    canaries(&top_a);
    canaries(&bottom_a);
}

static void input_tests(void)
{
    UpdaterUi ui;
    UpdaterAppView view = {0};
    UpdaterUi_Init(NULL, true);
    UpdaterUi_Invalidate(NULL);
    CHECK(UpdaterUi_Input(NULL, &view, UI_KEY_A) == UPDATER_UI_NONE);
    UpdaterUi_Init(&ui, true);
    CHECK(ui.japanese && !ui.confirming);
    CHECK(UpdaterUi_Input(&ui, NULL, UI_KEY_A) == UPDATER_UI_NONE);
    CHECK(UpdaterUi_Input(&ui, &view, 0) == UPDATER_UI_NONE);
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_CHECK);
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_X | UI_KEY_A) == UPDATER_UI_NONE && !ui.japanese);
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_START | UI_KEY_A) == UPDATER_UI_EXIT);
    view.available = true;
    view.stage = UPDATE_STAGE_DONE;
    strcpy(view.version, "v13.4Y-r2");
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_NONE && ui.confirming);
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_B | UI_KEY_A) == UPDATER_UI_NONE && !ui.confirming);
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_NONE && ui.confirming);
    strcpy(view.version, "v13.4Y-r3");
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_NONE && ui.confirming);
    CHECK(!memcmp(ui.confirm_version, view.version, sizeof(view.version)));
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_INSTALL && !ui.confirming);
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_NONE && ui.confirming);
    view.available = false;
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_CHECK && !ui.confirming);
    view.busy = true;
    view.stage = UPDATE_STAGE_DOWNLOAD;
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_NONE);
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_B) == UPDATER_UI_CANCEL);
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_B) == UPDATER_UI_NONE);
    for (int stage = UPDATE_STAGE_NONE; stage <= UPDATE_STAGE_CLOCK; ++stage)
    {
        UpdaterUi_Init(&ui, true);
        view.stage = (UpdateStage)stage;
        CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_B) ==
              (stage == UPDATE_STAGE_INSTALL || stage == UPDATE_STAGE_ROLLBACK ? UPDATER_UI_NONE : UPDATER_UI_CANCEL));
        CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_START) == UPDATER_UI_EXIT);
    }
    UpdaterUi_Init(&ui, true);
    view.cancelling = true;
    view.stage = UPDATE_STAGE_DOWNLOAD;
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_B) == UPDATER_UI_NONE);
    view.busy = false;
    view.cancelling = false;
    view.stage = UPDATE_STAGE_DONE;
    view.result = UPDATE_ERR_SD;
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_B) == UPDATER_UI_NONE && ui.dismissed);
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_CHECK && !ui.dismissed);
    memset(view.version, 'Z', sizeof(view.version));
    view.available = true;
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_NONE);
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_INSTALL);
}

static void drawing_tests(void)
{
    UpdaterUi ui;
    UpdaterAppView view = {0};
    UpdaterUi_Init(&ui, true);
    buffer_init(&top_a, 400, 240);
    buffer_init(&top_b, 400, 240);
    buffer_init(&bottom_a, 320, 240);
    buffer_init(&bottom_b, 320, 240);
    UpdaterUi_Draw(NULL, &view, &top_a.surface, &bottom_a.surface);
    UpdaterUi_Draw(&ui, NULL, &top_a.surface, &bottom_a.surface);
    UpdaterUi_Draw(&ui, &view, NULL, NULL);
    UpdaterUi_Draw(&ui, &view, &top_a.surface, &bottom_a.surface);
    uint64_t jp = hash(&bottom_a.surface);
    CHECK(hash(&top_a.surface) != hash(&bottom_a.surface));
    UpdaterUi_Draw(&ui, &view, &top_b.surface, &bottom_b.surface);
    CHECK(hash(&top_a.surface) == hash(&top_b.surface));
    CHECK(jp == hash(&bottom_b.surface));
    top_a.surface.pixels[0] = 0x51;
    bottom_a.surface.pixels[0] = 0x52;
    UpdaterUi_Draw(&ui, &view, &top_a.surface, &bottom_a.surface);
    CHECK(top_a.surface.pixels[0] == 0x51 && bottom_a.surface.pixels[0] == 0x52);
    UpdaterUi_Invalidate(&ui);
    UpdaterUi_Draw(&ui, &view, &top_a.surface, &bottom_a.surface);
    CHECK(top_a.surface.pixels[0] == 0 && bottom_a.surface.pixels[0] == 0);
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_X) == UPDATER_UI_NONE);
    UpdaterUi_Draw(&ui, &view, &top_a.surface, &bottom_a.surface);
    CHECK(jp != hash(&bottom_a.surface));
    UpdaterUiSurface invalid[] = {
        {NULL, 288000, 400, 240}, {top_a.bytes + 32, 287999, 400, 240},
        {top_a.bytes + 32, 288000, 0, 240}, {top_a.bytes + 32, 288000, 401, 240},
        {top_a.bytes + 32, 288000, 400, 241}, {top_a.bytes + 32, 288000, 400, 0}
    };
    uint64_t previous = hash(&top_a.surface);
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        UpdaterUi_Draw(&ui, &view, &invalid[i], &invalid[i]);
        CHECK(hash(&top_a.surface) == previous);
    }
    for (unsigned language = 0; language < 2; ++language)
        for (int result = UPDATE_ERR_CLOCK; result <= UPDATE_CURRENT; ++result)
            for (int stage = UPDATE_STAGE_NONE; stage <= UPDATE_STAGE_CLOCK; ++stage)
            {
                UpdaterUi_Init(&ui, language != 0);
                view.busy = stage != UPDATE_STAGE_NONE && stage != UPDATE_STAGE_DONE;
                view.available = true;
                view.installing = stage >= UPDATE_STAGE_DOWNLOAD;
                view.stage = (UpdateStage)stage;
                view.result = (UpdateResult)result;
                view.done = UINT32_MAX;
                view.total = result & 1 ? UINT32_MAX : 1;
                memset(view.version, (unsigned char)(0x80 + result + UPDATE_ERR_BUSY), sizeof(view.version));
                UpdaterUi_Draw(&ui, &view, &top_a.surface, &bottom_a.surface);
                canaries(&top_a);
                canaries(&bottom_a);
            }
    unsigned widths[] = {1, 8, 16, 32, 64, 100, 319, 320, 399, 400};
    for (unsigned i = 0; i < sizeof(widths) / sizeof(widths[0]); ++i)
    {
        buffer_init(&top_a, widths[i], i & 1 ? 1 : 239);
        UpdaterUi_Invalidate(&ui);
        UpdaterUi_Draw(&ui, &view, &top_a.surface, &top_a.surface);
        canaries(&top_a);
    }
    for (unsigned first = 0; first < 256; ++first)
    {
        unsigned char value[4] = {(unsigned char)first, 0x80, 0x80, 0x80};
        for (size_t length = 1; length <= sizeof(value); ++length)
        {
            size_t used = 0;
            uint32_t codepoint = decode(value, length, &used);
            CHECK(used >= 1 && used <= length);
            CHECK(codepoint <= 0x10FFFF && !(codepoint >= 0xD800 && codepoint <= 0xDFFF));
        }
    }
    size_t used;
    CHECK(decode((const unsigned char *)"\xE6\x9B\xB4", 3, &used) == 0x66F4 && used == 3);
    CHECK(decode((const unsigned char *)"\xED\xA0\x80", 3, &used) == 0xFFFD && used == 1);
    CHECK(decode((const unsigned char *)"\xF4\x90\x80\x80", 4, &used) == 0xFFFD && used == 1);
    UpdaterUi_Init(&ui, true);
    memset(&view, 0, sizeof(view));
    snapshot("japanese-home.ppm", &ui, &view);
    view.available = true;
    view.stage = UPDATE_STAGE_DONE;
    strcpy(view.version, "v13.4Y-r2");
    CHECK(UpdaterUi_Input(&ui, &view, UI_KEY_A) == UPDATER_UI_NONE);
    snapshot("japanese-confirm.ppm", &ui, &view);
    ui.japanese = false;
    view.busy = true;
    view.installing = true;
    view.stage = UPDATE_STAGE_DOWNLOAD;
    view.done = 37;
    view.total = 100;
    snapshot("english-download.ppm", &ui, &view);
    view.busy = false;
    view.stage = UPDATE_STAGE_DONE;
    view.result = UPDATE_ERR_TLS;
    snapshot("english-error.ppm", &ui, &view);
    ui.japanese = true;
    view.network.tlsError = -9984;
    view.network.verifyFlags = 0x400;
    view.network.httpStatus = 403;
    snapshot("japanese-diagnostics.ppm", &ui, &view);
    view.result = UPDATE_ERR_CLOCK;
    snapshot("japanese-clock-error.ppm", &ui, &view);
    view.busy = true;
    view.stage = UPDATE_STAGE_CLOCK;
    snapshot("japanese-clock.ppm", &ui, &view);
}

int main(void)
{
    input_tests();
    drawing_tests();
    printf("%u UI/input/render checks passed\n", checks);
    return 0;
}
'''
(generated / 'test.c').write_bytes(harness.encode('utf-8'))
include = root / 'Luma3DS_v13_4/sysmodules/rosalina/include'
executable = generated / ('test.exe' if os.name == 'nt' else 'test')
command = [compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-finput-charset=UTF-8',
           '-I' + str(updater / 'include'), '-I' + str(updater / 'source'), '-I' + str(include),
           str(generated / 'test.c'), '-o', str(executable)]
subprocess.run(command, check=True, env=env)
result = subprocess.run([str(executable)], cwd=generated, check=True, env=env, capture_output=True, text=True)
print(result.stdout.strip())
try:
    from PIL import Image
    for path in generated.glob('*.ppm'):
        Image.open(path).save(path.with_suffix('.png'))
except ImportError:
    pass
asset_hashes = {}
for name, dimensions in [('icon48x48.png', (48, 48)), ('banner256x128.png', (256, 128))]:
    data = (updater / 'assets' / name).read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n'
    offset = 8
    compressed = b''
    found = []
    while offset < len(data):
        length = struct.unpack_from('>I', data, offset)[0]
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        assert len(payload) == length and offset + length + 12 <= len(data)
        assert zlib.crc32(kind + payload) == struct.unpack_from('>I', data, offset + length + 8)[0]
        found.append(kind)
        if kind == b'IHDR':
            assert struct.unpack('>IIBBBBB', payload) == (*dimensions, 8, 2, 0, 0, 0)
        elif kind == b'IDAT':
            compressed += payload
        elif kind == b'IEND':
            assert length == 0
        offset += length + 12
    assert found == [b'IHDR', b'IDAT', b'IEND'] and offset == len(data)
    pixels = zlib.decompress(compressed)
    width, height = dimensions
    assert len(pixels) == height * (width * 3 + 1)
    colors = set()
    for row in range(height):
        line = pixels[row * (width * 3 + 1):(row + 1) * (width * 3 + 1)]
        assert line[0] == 0
        colors.update(line[x:x + 3] for x in range(1, len(line), 3))
    assert colors == {b'\0\0\0', bytes((0xCB, 0x9C, 0xFF))}
    asset_hashes[name] = hashlib.sha256(data).hexdigest()
with wave.open(str(updater / 'assets/silent.wav')) as audio:
    assert (audio.getnchannels(), audio.getsampwidth(), audio.getframerate(), audio.getnframes()) == (1, 2, 32000, 8000)
    assert not any(audio.readframes(8000))
asset_hashes['silent.wav'] = hashlib.sha256((updater / 'assets/silent.wav').read_bytes()).hexdigest()
tracked = [updater / 'source/ui.c', updater / 'include/ui.h', updater / 'tools/generate_assets.py',
           Path(__file__), font_header, include / 'font.h']
report = {'all_passed': True, 'host_checks': int(result.stdout.split()[0]), 'font_characters': len(required),
          'ui_sha256': hashlib.sha256((updater / 'source/ui.c').read_bytes()).hexdigest(),
          'sources': {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest() for path in tracked},
          'assets': asset_hashes,
          'hardware_tested': False, 'snapshots': [path.name for path in generated.glob('*.png')]}
(generated / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
