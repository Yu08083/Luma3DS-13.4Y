import os
import shutil
import struct
import subprocess
import zlib
from pathlib import Path

root = Path(__file__).resolve().parents[1]
generated = root / 'tests/generated'
generated.mkdir(exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
base = root / 'Luma3DS_v13_4/sysmodules/rosalina'
harness = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "theme_image.c"
static uint16_t pixels[320 * 240 + 32];
static uint64_t workspace[THEME_IMAGE_WORK_SIZE / 8 + 2];
static bool cancel, cancelAfter, failAfter;
static unsigned reads;
static bool readImage(void *context, uint32_t offset, void *buffer, uint32_t size)
{ return !(++reads > 2 && failAfter) && fseek(context, (long)offset, SEEK_SET) == 0 && fread(buffer, 1, size, context) == size; }
static bool cancelled(void *context) { (void)context; return cancel || (cancelAfter && reads >= 2); }
int main(int argc, char **argv)
{
    if(argc == 1)
    {
        unsigned long long checks = 0;
        for(uint32_t divisor = 1; divisor <= THEME_IMAGE_MAX_DIMENSION; ++divisor)
            for(uint32_t value = 0; value <= divisor * 320; ++value)
            {
                assert(imageScale(value, UINT32_MAX / divisor) == (value + divisor - 1) / divisor);
                ++checks;
            }
        printf("%llu exact scaling checks across all supported dimensions: OK\n", checks);
        for(unsigned value = 0; value < 256; ++value)
            for(unsigned alpha = 0; alpha < 256; ++alpha)
                assert(imageChannel(value, alpha) == value * alpha / 255);
        puts("65536 exact alpha channel checks: OK");
        return 0;
    }
    assert(argc >= 3);
    FILE *file = fopen(argv[1], "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    assert(size >= 0);
    ThemeImageSource source = { file, (uint32_t)size, readImage, cancelled };
    for(unsigned i = 0; i < sizeof(pixels) / sizeof(pixels[0]); ++i) pixels[i] = 0xA55A;
    workspace[0] = workspace[sizeof(workspace) / 8 - 1] = 0xA55A;
    cancel = argc > 3 && !strcmp(argv[3], "cancel");
    cancelAfter = argc > 3 && !strcmp(argv[3], "cancel-after");
    failAfter = argc > 3 && !strcmp(argv[3], "read-fail-after");
    bool valid = ThemeImage_Decode(&source, pixels + 16, workspace + 1, THEME_IMAGE_WORK_SIZE);
    fclose(file);
    for(unsigned i = 0; i < 16; ++i) assert(pixels[i] == 0xA55A && pixels[320 * 240 + 16 + i] == 0xA55A);
    assert(workspace[0] == 0xA55A && workspace[sizeof(workspace) / 8 - 1] == 0xA55A);
    assert(ThemeImage_Calloc(1, 1) == NULL);
    if(!valid) return 2;
    file = fopen(argv[2], "wb");
    assert(file);
    fwrite(pixels + 16, 2, 320 * 240, file);
    fclose(file);
    return 0;
}
'''
target = generated / 'themes_decode.c'
target.write_text(harness, encoding='utf-8', newline='\n')
exe = target.with_suffix('.exe' if os.name == 'nt' else '')
subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-unused-variable', '-Wno-sign-compare', '-Wno-implicit-fallthrough', '-O2', '-I' + str(base / 'include'), '-I' + str(base / 'source'), str(target), str(base / 'source/theme_codec.c'), '-o', str(exe)], check=True, env=env)
subprocess.run([str(exe)], check=True, env=env)

def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))

def png(width, height, rgba, interlaced=False):
    passes = [(0, 0, 1, 1)] if not interlaced else [(0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4), (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2)]
    data = b''
    for ox, oy, sx, sy in passes:
        if ox >= width or oy >= height:
            continue
        for y in range(oy, height, sy):
            data += b'\0' + b''.join(bytes(rgba(x, y)) for x in range(ox, width, sx))
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, int(interlaced))) + chunk(b'IDAT', zlib.compress(data)) + chunk(b'IEND', b'')

def rgb565(rgba):
    r, g, b, a = rgba
    return ((r * a // 255) >> 3) << 11 | ((g * a // 255) >> 2) << 5 | (b * a // 255) >> 3

def expected(width, height, rgba):
    dw, dh = (320, max(1, height * 320 // width)) if width * 240 > height * 320 else (max(1, width * 240 // height), 240)
    left, top = (320 - dw) // 2, (240 - dh) // 2
    result = [0] * (320 * 240)
    for x in range(dw):
        for y in range(dh):
            result[(left + x) * 240 + 239 - top - y] = rgb565(rgba(x * width // dw, y * height // dh))
    return struct.pack('<' + 'H' * len(result), *result)

checks = 0
def decode(name, data, wanted=None, valid=True, cancel=False, interruption=None):
    global checks
    path = generated / name
    path.write_bytes(data)
    output = generated / 'theme-decoded.rgb565'
    result = subprocess.run([str(exe), str(path), str(output)] + ([interruption or 'cancel'] if cancel or interruption else []), env=env)
    assert result.returncode == (0 if valid else 2), (name, result.returncode)
    if wanted is not None:
        assert output.read_bytes() == wanted, name
    checks += 1
    return output.read_bytes() if valid else None

rgba = lambda x, y: ((x * 37) % 256, (y * 51) % 256, ((x + y) * 29) % 256, (0, 80, 255)[(x + y) % 3])
for width, height in [(9, 7), (1, 1), (320, 240), (401, 255), (1024, 1024), (1, 1024), (1024, 1)]:
    for interlaced in [False, True]:
        data = png(width, height, rgba, interlaced)
        decode('rgba.png', data, expected(width, height, rgba))
        decode('cancel.png', data, valid=False, cancel=True)
        if width == height == 1024:
            for interruption in ['cancel-after', 'read-fail-after']:
                decode('interrupted.png', data, valid=False, interruption=interruption)
width, height = 17, 11
opaque = lambda x, y: ((x * 37) % 256, (y * 51) % 256, ((x + y) * 29) % 256, 255)
for bits in [24, 32]:
    for top_down in [False, True]:
        rows = []
        stride = (width * (bits // 8) + 3) & ~3
        for y in (range(height) if top_down else reversed(range(height))):
            row = b''
            for x in range(width):
                r, g, b, a = opaque(x, y)
                row += bytes((b, g, r)) + (b'\0' if bits == 32 else b'')
            rows.append(row.ljust(stride, b'\0'))
        payload = b''.join(rows)
        bmp = b'BM' + struct.pack('<IHHI', 54 + len(payload), 0, 0, 54) + struct.pack('<IiiHHIIiiII', 40, width, -height if top_down else height, 1, bits, 0, len(payload), 0, 0, 0, 0) + payload
        decode('theme.bmp', bmp, expected(width, height, opaque))
        decode('bad.bmp', bmp[:-1], valid=False)
        for interruption in ['cancel-after', 'read-fail-after']:
            decode('interrupted.bmp', bmp, valid=False, interruption=interruption)
        for offset, value in [(10, len(bmp)), (14, 0xFFFFFFFF), (18, 0), (18, 1025), (22, 0x80000000), (22, 1025), (28, 16), (30, 1)]:
            bad = bytearray(bmp)
            struct.pack_into('<I', bad, offset, value)
            decode('malformed.bmp', bad, valid=False)
for path in (root / 'tests/fixtures/themes').glob('*.jpg'):
    valid = path.name != 'progressive.jpg'
    data = path.read_bytes()
    output = decode('theme.jpg', data, valid=valid)
    if valid:
        values = struct.unpack('<' + 'H' * (320 * 240), output)
        value = values[160 * 240 + 119]
        r, g, b = (value >> 11) * 255 // 31, ((value >> 5) & 63) * 255 // 63, (value & 31) * 255 // 31
        if path.name == 'gray.jpg':
            assert abs(r - g) <= 8 and abs(g - b) <= 8
        else:
            assert abs(r - 190) <= 10 and abs(g - 70) <= 10 and abs(b - 150) <= 10
        decode('bad.jpg', data[:-10], valid=False)
        for interruption in ['cancel-after', 'read-fail-after']:
            decode('interrupted.jpg', data, valid=False, interruption=interruption)
for data in [b'broken' * 20, png(1025, 1, rgba), png(9, 7, rgba)[:-5]]:
    decode('invalid.image', data, valid=False)
data = bytearray(png(9, 7, rgba))
data[-6] ^= 1
decode('bad-crc.png', data, valid=False)
print(checks, 'theme image checks: PNG/RGBA/Adam7/alpha, baseline JPEG, BMP orientation/padding, aspect ratio, size limits, corruption and cancellation OK')
