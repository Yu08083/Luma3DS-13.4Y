import ctypes
import hashlib
import io
import json
import math
import mmap
import os
from pathlib import Path
import random
import re
import shutil
import struct
import subprocess

import PIL
from PIL import Image, ImageChops, ImageDraw, ImageStat

assert PIL.__version__ == '12.3.0', PIL.__version__
root = Path(__file__).resolve().parents[1]
module = root / 'Luma3DS_v13_4/sysmodules/rosalina'
generated = root / 'build/streaming-jpeg-tests'
generated.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
compiler = shutil.which(compiler) or compiler
environment = os.environ.copy()
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
library_path = generated / ('rp_jpeg.dll' if os.name == 'nt' else 'rp_jpeg.so')
source_path = module / 'source/rp_jpeg.c'
header_path = module / 'include/rp_jpeg.h'
source_hash = hashlib.sha256(source_path.read_bytes()).hexdigest()
command = [compiler, '-shared', '-fPIC', '-O2', '-std=c11', '-Wall', '-Wextra', '-Werror',
           '-I' + str(module / 'include'), str(source_path), '-o', str(library_path)]
subprocess.run(command, check=True, env=environment)
directory = os.add_dll_directory(str(Path(compiler).parent)) if os.name == 'nt' else None
library = ctypes.CDLL(str(library_path))


class Huff(ctypes.Structure):
    _fields_ = [('code', ctypes.c_uint16 * 256), ('size', ctypes.c_uint8 * 256)]


class Encoder(ctypes.Structure):
    _fields_ = [('quality', ctypes.c_int), ('dcY', Huff), ('dcC', Huff), ('acY', Huff), ('acC', Huff),
                ('recipY', ctypes.c_uint16 * 64), ('recipC', ctypes.c_uint16 * 64),
                ('dqtY', ctypes.c_uint8 * 64), ('dqtC', ctypes.c_uint8 * 64)]


assert ctypes.sizeof(Encoder) == 3460
library.RpJpeg_SetQuality.argtypes = [ctypes.POINTER(Encoder), ctypes.c_int]
library.RpJpeg_SetQuality.restype = None
library.RpJpeg_BytesPerPixel.argtypes = [ctypes.c_int]
library.RpJpeg_BytesPerPixel.restype = ctypes.c_uint32
library.RpJpeg_Encode.argtypes = [ctypes.POINTER(Encoder), ctypes.c_void_p, ctypes.c_uint32,
                                ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32,
                                ctypes.c_uint32, ctypes.c_uint32, ctypes.c_int]
library.RpJpeg_Encode.restype = ctypes.c_uint32
cases = []


class Buffer:
    def __init__(self, length, data=None):
        self.length = length
        self.storage = (ctypes.c_uint8 * (length + 128))(*([0xA7] * (length + 128)))
        self.pointer = ctypes.addressof(self.storage) + 64
        if data is not None:
            assert len(data) == length
            ctypes.memmove(self.pointer, data, length)

    def check(self):
        assert bytes(self.storage[:64]) == b'\xA7' * 64
        assert bytes(self.storage[-64:]) == b'\xA7' * 64

    def read(self):
        return ctypes.string_at(self.pointer, self.length)


class FencedBuffer:
    def __init__(self, length, data=None):
        self.length = length
        self.accessible = ((length + 64 + mmap.PAGESIZE - 1) // mmap.PAGESIZE) * mmap.PAGESIZE
        self.total = self.accessible + mmap.PAGESIZE
        if os.name == 'nt':
            self.platform = ctypes.WinDLL('kernel32', use_last_error=True)
            self.platform.VirtualAlloc.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_uint32, ctypes.c_uint32]
            self.platform.VirtualAlloc.restype = ctypes.c_void_p
            self.platform.VirtualProtect.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
            self.platform.VirtualFree.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_uint32]
            self.base = self.platform.VirtualAlloc(None, self.total, 0x3000, 4)
            assert self.base
            old = ctypes.c_uint32()
            assert self.platform.VirtualProtect(self.base + self.accessible, mmap.PAGESIZE, 1, ctypes.byref(old))
        else:
            self.mapping = mmap.mmap(-1, self.total)
            self.base = ctypes.addressof(ctypes.c_char.from_buffer(self.mapping))
            self.platform = ctypes.CDLL(None, use_errno=True)
            self.platform.mprotect.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int]
            assert self.platform.mprotect(self.base + self.accessible, mmap.PAGESIZE, 0) == 0
        self.pointer = self.base + self.accessible - length
        ctypes.memset(self.pointer - 64, 0xA7, length + 64)
        if data is not None:
            assert len(data) == length
            ctypes.memmove(self.pointer, data, length)

    def check(self):
        assert ctypes.string_at(self.pointer - 64, 64) == b'\xA7' * 64

    def read(self):
        return ctypes.string_at(self.pointer, self.length)

    def close(self):
        if os.name == 'nt':
            assert self.platform.VirtualFree(self.base, 0, 0x8000)
        else:
            assert self.platform.mprotect(self.base + self.accessible, mmap.PAGESIZE, 3) == 0
            self.mapping.close()


def encoder(quality=90):
    result = Encoder()
    library.RpJpeg_SetQuality(ctypes.byref(result), quality)
    return result


def decode(data, dimensions):
    assert data[:2] == b'\xFF\xD8' and data[-2:] == b'\xFF\xD9'
    image = Image.open(io.BytesIO(data))
    assert image.format == 'JPEG' and image.size == dimensions
    image.load()
    return image.convert('RGB')


def encode(data, width, rows, stride, row_step, format_value, quality=90, capacity=512000):
    enc = encoder(quality)
    src = Buffer(len(data), data)
    out = Buffer(capacity)
    size = library.RpJpeg_Encode(ctypes.byref(enc), out.pointer, capacity, src.pointer, len(data),
                                 width, rows, stride, row_step, format_value)
    src.check()
    out.check()
    assert src.read() == data
    assert size <= capacity
    return out.read()[:size]


def pack_color(color, format_value, alpha=255):
    red, green, blue = color
    if format_value == 0:
        return bytes((alpha, blue, green, red))
    if format_value == 1:
        return bytes((blue, green, red))
    if format_value == 2:
        value = (red >> 3) << 11 | (green >> 2) << 5 | blue >> 3
    elif format_value == 3:
        value = (red >> 3) << 11 | (green >> 3) << 6 | (blue >> 3) << 1 | (alpha >> 7)
    else:
        value = (red >> 4) << 12 | (green >> 4) << 8 | (blue >> 4) << 4 | (alpha >> 4)
    return struct.pack('<H', value)


def screen_pattern(width):
    image = Image.new('RGB', (width, 240))
    for y in range(240):
        for x in range(width):
            color = ((255, 0, 0) if x < width // 2 else (0, 255, 0)) if y < 120 else \
                    ((0, 0, 255) if x < width // 2 else (255, 255, 0))
            if 104 <= x < 136 and 80 <= y < 112:
                color = (255, 255, 255)
            if width - 64 <= x < width - 32 and 32 <= y < 64:
                color = (0, 0, 0)
            image.putpixel((x, y), color)
    return image


def native_pixels(image, format_value, alpha=255, padding=0, row_step=1):
    rows, width = image.width, image.height
    bpp = library.RpJpeg_BytesPerPixel(format_value)
    stride = width * bpp + padding
    span = (rows - 1) * stride * row_step + width * bpp
    data = bytearray(b'\xD3' * span)
    for row in range(rows):
        for column in range(width):
            start = row * stride * row_step + column * bpp
            data[start:start + bpp] = pack_color(image.getpixel((row, width - 1 - column)), format_value, alpha)
    return bytes(data), stride


for format_value, bpp in enumerate((4, 3, 2, 2, 2)):
    assert library.RpJpeg_BytesPerPixel(format_value) == bpp
for value in (-2147483648, -1, 5, 255, 2147483647):
    assert library.RpJpeg_BytesPerPixel(value) == 0
cases.append({'case': 'bytes-per-pixel', 'formats': 5, 'invalid': 5})

for screen_width in (400, 320):
    original = screen_pattern(screen_width)
    samples = [(24, 24), (screen_width - 24, 24), (24, 216), (screen_width - 24, 216),
               (120, 96), (screen_width - 48, 48)]
    for format_value in range(5):
        data, stride = native_pixels(original, format_value)
        jpeg = encode(data, 240, screen_width, stride, 1, format_value)
        decoded = decode(jpeg, (240, screen_width))
        errors = []
        for x, y in samples:
            actual = decoded.getpixel((239 - y, x))
            expected = original.getpixel((x, y))
            error = max(abs(actual[channel] - expected[channel]) for channel in range(3))
            assert error <= 8, (screen_width, format_value, (x, y), actual, expected)
            errors.append(error)
        restored = decoded.transpose(Image.Transpose.ROTATE_90)
        assert restored.size == original.size
        for position in samples:
            assert restored.getpixel(position) == decoded.getpixel((239 - position[1], position[0]))
        if format_value == 1:
            (generated / ('top.jpg' if screen_width == 400 else 'bottom.jpg')).write_bytes(jpeg)
            restored.save(generated / ('top-restored.png' if screen_width == 400 else 'bottom-restored.png'))
        cases.append({'case': 'screen-format-color-rotation', 'screen_width': screen_width,
                      'jpeg_size': [240, screen_width], 'format': format_value, 'jpeg_bytes': len(jpeg),
                      'sample_max_channel_errors': errors})


def psnr(original, decoded):
    statistics = ImageStat.Stat(ImageChops.difference(original, decoded))
    mse = sum(statistics.sum2) / (3 * original.width * original.height)
    return 10 * math.log10(255 * 255 / mse) if mse else 100.0


for screen_width in (400, 320):
    color_pixels = Image.new('RGB', (screen_width, 240))
    color_pixels.putdata([(255, 0, 0) if (x + y) & 1 else (0, 0, 255)
                         for y in range(240) for x in range(screen_width)])
    for format_value in range(5):
        packed, stride = native_pixels(color_pixels, format_value)
        low = encode(packed, 240, screen_width, stride, 1, format_value, quality=70)
        high = encode(packed, 240, screen_width, stride, 1, format_value, quality=90)
        low_image = decode(low, (240, screen_width)).transpose(Image.Transpose.ROTATE_90)
        high_image = decode(high, (240, screen_width)).transpose(Image.Transpose.ROTATE_90)
        low_psnr = psnr(color_pixels, low_image)
        high_psnr = psnr(color_pixels, high_image)
        assert low_psnr < 10 and high_psnr > 35 and high_psnr - low_psnr > 25
        assert Image.open(io.BytesIO(low)).layer == [(1, 2, 2, 0), (2, 1, 1, 1), (3, 1, 1, 1)]
        assert Image.open(io.BytesIO(high)).layer == [(1, 1, 1, 0), (2, 1, 1, 1), (3, 1, 1, 1)]
        cases.append({'case': 'full-color-preserves-native-one-pixel-color-edges', 'screen_width': screen_width,
                      'format': format_value, 'quality70_psnr_db': low_psnr, 'quality90_psnr_db': high_psnr,
                      'quality70_bytes': len(low), 'quality90_bytes': len(high)})

ui = Image.new('RGB', (400, 240), (240, 230, 190))
ui_draw = ImageDraw.Draw(ui)
for row in range(12):
    color = ((230, 30, 30), (20, 100, 240), (20, 140, 30))[row % 3]
    ui_draw.text((5, row * 19 + 2), 'HP123/456 Item ABC xyz 0123456789', fill=color)
    ui_draw.rectangle((315, row * 19 + 2, 329, row * 19 + 16), fill=color)
ui_packed, ui_stride = native_pixels(ui, 1)
for quality in (70, 89, 90, 95, 100):
    jpeg = encode(ui_packed, 240, 400, ui_stride, 1, 1, quality=quality)
    actual = decode(jpeg, (240, 400)).transpose(Image.Transpose.ROTATE_90)
    reference_data = io.BytesIO()
    ui.transpose(Image.Transpose.ROTATE_270).save(reference_data, format='JPEG', quality=quality,
                                               subsampling=0 if quality >= 90 else 2)
    reference_jpeg = Image.open(io.BytesIO(reference_data.getvalue()))
    actual_jpeg = Image.open(io.BytesIO(jpeg))
    reference = reference_jpeg.convert('RGB').transpose(Image.Transpose.ROTATE_90)
    assert actual_jpeg.quantization == reference_jpeg.quantization
    assert actual_jpeg.layer[0][1:3] == ((1, 1) if quality >= 90 else (2, 2))
    if quality <= 95:
        assert psnr(ui, actual) >= psnr(ui, reference) - 1
    (generated / ('ui-quality' + str(quality) + '.jpg')).write_bytes(jpeg)
    cases.append({'case': 'colored-ui-matches-standard-jpeg-profile', 'quality': quality,
                  'sampling': '444' if quality >= 90 else '420', 'jpeg_bytes': len(jpeg),
                  'actual_psnr_db': psnr(ui, actual), 'reference_psnr_db': psnr(ui, reference)})

small = Image.new('RGB', (32, 32))
for y in range(32):
    for x in range(32):
        small.putpixel((x, y), (255 if x < 16 else 0, 255 if y < 16 else 0, 128))
for format_value in range(5):
    packed, packed_stride = native_pixels(small, format_value)
    baseline = encode(packed, 32, 32, packed_stride, 1, format_value)
    for row_step, padding in ((1, 19), (2, 0), (3, 13)):
        padded, stride = native_pixels(small, format_value, padding=padding, row_step=row_step)
        actual = encode(padded, 32, 32, stride, row_step, format_value)
        decode(actual, (32, 32))
        assert actual == baseline
        cases.append({'case': 'stride-rowstep-ignored-padding', 'format': format_value,
                      'row_step': row_step, 'padding': padding, 'exact_jpeg': True})
    if format_value in (0, 3, 4):
        transparent, stride = native_pixels(small, format_value, alpha=0)
        assert encode(transparent, 32, 32, stride, 1, format_value) == baseline
        cases.append({'case': 'alpha-ignored', 'format': format_value})

quality_input, quality_stride = native_pixels(small, 1)
lowest = encoder(1)
highest = encoder(100)
for quality in (-2147483648, -1, 0, 1, 2, 25, 50, 75, 99, 100, 101, 2147483647):
    enc = encoder(quality)
    expected = max(1, min(100, quality))
    assert enc.quality == expected
    assert all(1 <= value <= 255 for value in enc.dqtY) and all(1 <= value <= 255 for value in enc.dqtC)
    if quality < 1:
        assert bytes(enc) == bytes(lowest)
    if quality > 100:
        assert bytes(enc) == bytes(highest)
    jpeg = encode(quality_input, 32, 32, quality_stride, 1, 1, quality=quality)
    decode(jpeg, (32, 32))
    cases.append({'case': 'quality-clamp-and-decode', 'requested': quality, 'effective': expected, 'jpeg_bytes': len(jpeg)})

valid_data = bytes((0x51, 0xA0, 0x20)) * (16 * 16)
enc = encoder()
source_buffer = Buffer(len(valid_data), valid_data)
output_buffer = Buffer(8192)
defaults = {'width': 16, 'rows': 16, 'stride': 48, 'rowStep': 1, 'format': 1,
            'srcSize': len(valid_data), 'outCap': 8192}
invalid = [('width-zero', {'width': 0}), ('rows-zero', {'rows': 0}), ('width-unaligned', {'width': 15}),
           ('rows-unaligned', {'rows': 15}), ('width-u16-overflow', {'width': 65536}),
           ('rows-u16-overflow', {'rows': 65536}), ('rowstep-zero', {'rowStep': 0}),
           ('stride-zero', {'stride': 0}), ('stride-short', {'stride': 47}),
           ('source-one-short', {'srcSize': len(valid_data) - 1}), ('source-zero', {'srcSize': 0}),
           ('format-negative', {'format': -1}), ('format-five', {'format': 5}),
           ('stride-pitch-overflow', {'stride': 0xFFFFFFFF, 'rowStep': 2}),
           ('rowstep-pitch-overflow', {'stride': 48, 'rowStep': 0xFFFFFFFF}),
           ('u64-span-overflow-input', {'stride': 0xFFFFFFFF, 'rowStep': 0xFFFFFFFF, 'rows': 65520}),
           ('capture-cap-exceeded', {'stride': 960, 'width': 240, 'rows': 400, 'rowStep': 2, 'srcSize': 0xFFFFFFFF})]
invalid.extend(('output-cap-' + str(capacity), {'outCap': capacity}) for capacity in (0, 1, 1023, 5119))
for name, overrides in invalid:
    values = defaults | overrides
    before = output_buffer.read()
    count = library.RpJpeg_Encode(ctypes.byref(enc), output_buffer.pointer, values['outCap'], source_buffer.pointer,
                                  values['srcSize'], values['width'], values['rows'], values['stride'],
                                  values['rowStep'], values['format'])
    assert count == 0 and output_buffer.read() == before
    source_buffer.check()
    output_buffer.check()
    cases.append({'case': name, 'rejected_before_output': True})

for name, enc_pointer, out_pointer, src_pointer in (
        ('null-encoder', None, output_buffer.pointer, source_buffer.pointer),
        ('null-output', ctypes.byref(enc), None, source_buffer.pointer),
        ('null-source', ctypes.byref(enc), output_buffer.pointer, None)):
    assert library.RpJpeg_Encode(enc_pointer, out_pointer, 8192, src_pointer, len(valid_data), 16, 16, 48, 1, 1) == 0
    cases.append({'case': name, 'rejected': True})
maximum = (1 << (ctypes.sizeof(ctypes.c_void_p) * 8)) - 1
assert library.RpJpeg_Encode(ctypes.byref(enc), output_buffer.pointer, 8192, maximum - 16, len(valid_data), 16, 16, 48, 1, 1) == 0
assert library.RpJpeg_Encode(ctypes.byref(enc), maximum - 16, 8192, source_buffer.pointer, len(valid_data), 16, 16, 48, 1, 1) == 0
cases.append({'case': 'source-output-pointer-wrap', 'pointer_bits': ctypes.sizeof(ctypes.c_void_p) * 8, 'rejected': True})
uninitialized = Encoder()
assert library.RpJpeg_Encode(ctypes.byref(uninitialized), output_buffer.pointer, 8192, source_buffer.pointer,
                             len(valid_data), 16, 16, 48, 1, 1) == 0
cases.append({'case': 'uninitialized-encoder', 'rejected': True})

noise = random.Random(0x134).randbytes(240 * 320 * 3)
complete = encode(noise, 240, 320, 720, 1, 1, quality=100)
decode(complete, (240, 320))
for capacity in (5120, 6000, 12000, 32768):
    truncated = encode(noise, 240, 320, 720, 1, 1, quality=100, capacity=capacity)
    assert not truncated
    cases.append({'case': 'output-exhaustion-canary', 'capacity': capacity, 'rejected': True})
for capacity in (len(complete) + 4096, 512000):
    jpeg = encode(noise, 240, 320, 720, 1, 1, quality=100, capacity=capacity)
    assert jpeg == complete
    decode(jpeg, (240, 320))
    cases.append({'case': 'noise-full-decode', 'capacity': capacity, 'jpeg_bytes': len(jpeg)})

for width, rows, format_value in ((16, 16, 0), (240, 400, 0), (240, 320, 1)):
    bpp = library.RpJpeg_BytesPerPixel(format_value)
    data = bytes([0x41] * (width * rows * bpp))
    src = FencedBuffer(len(data), data)
    out = FencedBuffer(512000)
    try:
        count = library.RpJpeg_Encode(ctypes.byref(enc), out.pointer, out.length, src.pointer, src.length,
                                      width, rows, width * bpp, 1, format_value)
        src.check()
        out.check()
        assert src.read() == data
        decode(out.read()[:count], (width, rows))
        assert library.RpJpeg_Encode(ctypes.byref(enc), out.pointer, out.length, src.pointer, src.length - 1,
                                     width, rows, width * bpp, 1, format_value) == 0
        cases.append({'case': 'source-output-noaccess-page-fence', 'width': width, 'rows': rows,
                      'format': format_value, 'source_bytes': len(data), 'jpeg_bytes': count})
    finally:
        src.close()
        out.close()

remote_path = module / 'source/remote_play.c'
remote_source = remote_path.read_text(encoding='utf-8')


def declaration(start, closing):
    offset = remote_source.index(start)
    end = remote_source.index(closing, offset) + len(closing)
    return remote_source[offset:end]


retry_helper = declaration('static u32 encodeCapture(', '\n}')
settings_helper = declaration('static bool takeSettings(', '\n}')
slot_type = declaration('typedef struct JpegSlot\n', '} JpegSlot;')
capture_type = declaration('typedef struct Capture\n', '} Capture;')
screen_type = declaration('typedef struct ScreenState\n', '} ScreenState;')
encoder_type = declaration('typedef struct EncoderState\n', '} EncoderState;')
capacity = int(re.search(r'#define JPEG_SLOT_CAP\s+\((\d+) \* 1024\)', remote_source)[1]) * 1024
assert capacity == 128 * 1024
assert int(re.search(r'#define QUALITY_RECOVERY_US\s+(\d+)', remote_source)[1]) == 2000000
assert int(re.search(r'#define QUALITY_PROBE_US\s+(\d+)', remote_source)[1]) == 8000000
retry_prefix = r'''
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "rp_jpeg.h"
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
#define JPEG_SLOT_CAP (128 * 1024)
#define CAPTURE_CAP 384000
#define VIDEO_PACKET_SIZE 1448
#define SYSCLOCK_ARM11 268123480ULL
#define TICKS_PER_US (SYSCLOCK_ARM11 / 1000000)
#define QUALITY_RECOVERY_US 2000000
#define QUALITY_PROBE_US 8000000
static u64 retryNow;
static u64 svcGetSystemTick(void) { return retryNow; }
static u32 settingsLock,settingsGeneration,viewerIp,settingMode,settingQuality,settingQos;
static void LightLock_Lock(u32 *lock) { ++*lock; }
static void LightLock_Unlock(u32 *lock) { --*lock; }
static bool stopRequested,preTerminationRequested;
static unsigned attempts,history[8],stopAfter,preAfter,qualityChanges,failAboveQuality;
static void tracedQuality(RpJpeg *encoder,int quality)
{
    ++qualityChanges;RpJpeg_SetQuality(encoder,quality);
}
static u32 tracedEncode(const RpJpeg *encoder,u8 *out,u32 outCap,const u8 *src,u32 srcSize,
                        u32 width,u32 rows,u32 stride,u32 step,int format)
{
    if(attempts<8)history[attempts]=encoder->quality;
    ++attempts;
    u32 size=RpJpeg_Encode(encoder,out,outCap,src,srcSize,width,rows,stride,step,format);
    if(stopAfter&&attempts==stopAfter)stopRequested=true;
    if(preAfter&&attempts==preAfter)preTerminationRequested=true;
    if(failAboveQuality&&(unsigned)encoder->quality>failAboveQuality)return 0;
    return size;
}
#define RpJpeg_Encode tracedEncode
#define RpJpeg_SetQuality tracedQuality
'''
retry_suffix = r'''
#undef RpJpeg_Encode
static struct { u8 prefix[64]; JpegSlot slot; u8 suffix[64]; } guarded;
void Retry_Init(int quality,int stop,int pre,unsigned afterStop,unsigned afterPre)
{
    memset(&guarded,0xA7,sizeof(guarded));
    memset(&enc,0,sizeof(enc));retryNow=0;settingsGeneration=1;viewerIp=1;
    settingMode=0x102;settingQuality=quality;settingQos=2097152;takeSettings();
    attempts=qualityChanges=failAboveQuality=0;memset(history,0,sizeof(history));stopAfter=afterStop;preAfter=afterPre;
    stopRequested=stop!=0;preTerminationRequested=pre!=0;
}
u32 Retry_Run(const u8 *pixels,u32 rows,u32 pitch,int format)
{
    Capture capture={0};capture.pixels=pixels;capture.rows=rows;capture.pitch=pitch;capture.format=format;
    return encodeCapture(&guarded.slot,&capture,&enc.screens[rows==400]);
}
unsigned Retry_Attempts(void) { return attempts; }
unsigned Retry_History(unsigned index) { return index<8?history[index]:0; }
int Retry_Quality(void) { return enc.jpeg.quality; }
const u8 *Retry_Data(void) { return guarded.slot.data; }
void Retry_Retrace(void) { attempts=qualityChanges=0;memset(history,0,sizeof(history)); }
void Retry_Time(u64 microseconds) { retryNow=microseconds*TICKS_PER_US; }
void Retry_Ticks(u64 ticks) { retryNow=ticks; }
void Retry_Stop(int stop,int pre,unsigned afterStop,unsigned afterPre)
{
    stopRequested=stop!=0;preTerminationRequested=pre!=0;stopAfter=afterStop;preAfter=afterPre;
}
int Retry_Settings(unsigned quality,unsigned mode,unsigned qos,unsigned ip)
{
    settingQuality=quality;settingMode=mode;settingQos=qos;viewerIp=ip;++settingsGeneration;
    return takeSettings();
}
unsigned Retry_ScreenQuality(unsigned screen) { return enc.screens[screen!=0].quality; }
unsigned Retry_SmallFrames(unsigned screen) { return enc.screens[screen!=0].smallFrames; }
unsigned Retry_QualityChanges(void) { return qualityChanges; }
unsigned Retry_ScreenBytes(void) { return sizeof(ScreenState); }
void Retry_FailAbove(unsigned quality) { failAboveQuality=quality; }
int Retry_Guards(void)
{
    for(unsigned i=0;i<64;++i)if(guarded.prefix[i]!=0xA7||guarded.suffix[i]!=0xA7)return 0;
    return 1;
}
'''
retry_c = generated / 'retry.c'
retry_c.write_bytes((retry_prefix + screen_type + encoder_type + '\nstatic EncoderState enc;\n' + slot_type + capture_type + settings_helper + retry_helper + retry_suffix).encode())
retry_path = generated / ('retry.dll' if os.name == 'nt' else 'retry.so')
subprocess.run([compiler, '-shared', '-fPIC', '-O2', '-std=c11', '-Wall', '-Wextra', '-Werror',
                '-I' + str(module / 'include'), str(source_path), str(retry_c), '-o', str(retry_path)],
               check=True, env=environment)
retry = ctypes.CDLL(str(retry_path))
retry.Retry_Init.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint, ctypes.c_uint]
retry.Retry_Run.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_int]
retry.Retry_Run.restype = ctypes.c_uint32
retry.Retry_History.argtypes = [ctypes.c_uint]
retry.Retry_Data.restype = ctypes.c_void_p
retry.Retry_Time.argtypes = [ctypes.c_uint64]
retry.Retry_Ticks.argtypes = [ctypes.c_uint64]
retry.Retry_Stop.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_uint, ctypes.c_uint]
retry.Retry_Settings.argtypes = [ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint]
retry.Retry_ScreenQuality.argtypes = [ctypes.c_uint]
retry.Retry_SmallFrames.argtypes = [ctypes.c_uint]
retry.Retry_FailAbove.argtypes = [ctypes.c_uint]
for rows in (400, 320):
    for seed in (0x134, 0x256, 0x134001):
        data = random.Random(seed).randbytes(240 * rows * 3)
        native = Buffer(len(data), data)
        retry.Retry_Init(100, 0, 0, 0, 0)
        count = retry.Retry_Run(native.pointer, rows, 720, 1)
        assert 0 < count < capacity and retry.Retry_Attempts() == 3
        assert [retry.Retry_History(index) for index in range(3)] == [100, 90, 80]
        assert retry.Retry_Quality() == 80 and retry.Retry_Guards()
        jpeg = ctypes.string_at(retry.Retry_Data(), count)
        decode(jpeg, (240, rows))
        retry.Retry_Retrace()
        repeated = retry.Retry_Run(native.pointer, rows, 720, 1)
        assert repeated == count and retry.Retry_Attempts() == 1 and retry.Retry_History(0) == 80
        assert ctypes.string_at(retry.Retry_Data(), repeated) == jpeg
        native.check()
        assert native.read() == data and retry.Retry_Guards()
        cases.append({'case': 'actual-retry-100-overflow-recovers-80-and-persists',
                      'rows': rows, 'seed': seed, 'jpeg_bytes': count, 'attempt_qualities': [100, 90, 80],
                      'following_frame_attempt_qualities': [80]})

cancel_data = random.Random(0x134).randbytes(240 * 400 * 3)
native = Buffer(len(cancel_data), cancel_data)
for name, controls, expected_attempts in (
        ('stop-before', (1, 0, 0, 0), 0), ('preterminate-before', (0, 1, 0, 0), 0),
        ('stop-between-attempts', (0, 0, 1, 0), 1), ('preterminate-between-attempts', (0, 0, 0, 1), 1)):
    retry.Retry_Init(100, *controls)
    assert retry.Retry_Run(native.pointer, 400, 720, 1) == 0
    assert retry.Retry_Attempts() == expected_attempts and retry.Retry_Guards()
    cases.append({'case': 'actual-retry-' + name, 'encode_attempts': expected_attempts})
for quality, history in ((100, [100, 90, 80, 40]), (90, [90, 80, 40, 1]), (80, [80, 40, 1]), (40, [40, 1]), (1, [1])):
    retry.Retry_Init(quality, 0, 0, 0, 0)
    assert retry.Retry_Run(native.pointer, 400, 720, 5) == 0
    assert retry.Retry_Attempts() == len(history) and retry.Retry_Guards()
    assert [retry.Retry_History(index) for index in range(len(history))] == history
    assert retry.Retry_ScreenQuality(1) == 1
    cases.append({'case': 'actual-retry-final-failure-bounded', 'start_quality': quality,
                  'attempt_qualities': history})


def recovery_run(data, rows, now, expected, changes=None, ticks=False):
    if ticks:
        retry.Retry_Ticks(now)
    else:
        retry.Retry_Time(now)
    retry.Retry_Retrace()
    native = Buffer(len(data), data)
    size = retry.Retry_Run(native.pointer, rows, 720, 1)
    history = [retry.Retry_History(index) for index in range(retry.Retry_Attempts())]
    assert history == expected, (now, rows, history, expected)
    assert retry.Retry_Guards()
    native.check()
    assert native.read() == data
    if changes is not None:
        assert retry.Retry_QualityChanges() == changes
    if size:
        assert size < capacity
        jpeg = ctypes.string_at(retry.Retry_Data(), size)
        decode(jpeg, (240, rows))
    return size


for rows in (400, 320):
    noise = random.Random(0x134).randbytes(240 * rows * 3)
    simple = bytes((32, 64, 96)) * (240 * rows)
    screen = int(rows == 400)
    retry.Retry_Init(100, 0, 0, 0, 0)
    assert recovery_run(noise, rows, 0, [100, 90, 80], 3) > capacity // 4
    for now in (500000, 1000000, 1500000, 1999999):
        assert recovery_run(simple, rows, now, [80], 0) < capacity // 4
    assert retry.Retry_SmallFrames(screen) == 3
    recovery_run(simple, rows, 2000000, [100], 1)
    assert retry.Retry_ScreenQuality(screen) == 100 and retry.Retry_SmallFrames(screen) == 0
    recovery_run(simple, rows, 8000000, [100], 0)
    cases.append({'case': 'actual-recovery-complex-to-simple-both-screens', 'rows': rows,
                  'first_attempts': [100, 90, 80], 'pre_deadline_attempts': [80], 'recovered_attempts': [100],
                  'minimum_small_frames': 3, 'cooldown_us': 2000000})

    retry.Retry_Init(100, 0, 0, 0, 0)
    recovery_run(noise, rows, 0, [100, 90, 80])
    for now in (100000, 200000, 300000):
        recovery_run(simple, rows, now, [80])
    recovery_run(noise, rows, 1900000, [80])
    assert retry.Retry_SmallFrames(screen) == 0
    for now in (2000000, 2100000, 2200000):
        recovery_run(simple, rows, now, [80])
    recovery_run(simple, rows, 2300000, [100])
    cases.append({'case': 'actual-recovery-large-frame-clears-small-history', 'rows': rows})

    retry.Retry_Init(100, 0, 0, 0, 0)
    recovery_run(noise, rows, 0, [100, 90, 80])
    probes = []
    for index in range(1, 81):
        now = index * 500000
        probing = now % 8000000 == 0
        recovery_run(noise, rows, now, [100, 90, 80] if probing else [80], 3 if probing else 0)
        if probing:
            probes.append(now)
    assert retry.Retry_SmallFrames(screen) == 0
    assert probes == [8000000, 16000000, 24000000, 32000000, 40000000]
    cases.append({'case': 'actual-recovery-continuous-noise-bounded-periodic-probe', 'rows': rows,
                  'seconds': 40, 'following_encodes': 80, 'probe_times_us': probes,
                  'following_high_quality_attempts': len(probes) * 2})

    amplitude = 80 if rows == 400 else 112
    rng = random.Random(0x594355)
    moderate = Image.frombytes('RGB', (rows, 240), bytes(rng.randrange(128 - amplitude // 2,
                    128 + amplitude // 2) for _ in range(rows * 240 * 3)))
    moderate_data, moderate_stride = native_pixels(moderate, 1)
    assert moderate_stride == 720
    high = encode(moderate_data, 240, rows, 720, 1, 1, quality=90, capacity=capacity)
    low = encode(moderate_data, 240, rows, 720, 1, 1, quality=80, capacity=capacity)
    assert high and len(low) > capacity // 4
    retry.Retry_Init(90, 0, 0, 0, 0)
    recovery_run(noise, rows, 0, [90, 80])
    for now in (100000, 200000, 2000000, 7999999):
        assert recovery_run(moderate_data, rows, now, [80], 0) == len(low)
    assert retry.Retry_SmallFrames(screen) == 0
    assert retry.Retry_Settings(90, 0x102, 512 * 1024, 1)
    assert recovery_run(moderate_data, rows, 8000000, [90], 1) == len(high)
    assert ctypes.string_at(retry.Retry_Data(), len(high)) == high
    for now in (16000000, 64000000, 3600000000):
        recovery_run(moderate_data, rows, now, [90], 0)
    high_decoded = decode(high, (240, rows)).transpose(Image.Transpose.ROTATE_90)
    low_decoded = decode(low, (240, rows)).transpose(Image.Transpose.ROTATE_90)
    high_psnr, low_psnr = psnr(moderate, high_decoded), psnr(moderate, low_decoded)
    assert high_psnr > low_psnr + 5
    cases.append({'case': 'actual-recovery-over-quarter-cap-scene-restores-native-444', 'rows': rows,
                  'fallback_bytes': len(low), 'restored_bytes': len(high), 'probe_us': 8000000,
                  'fallback_psnr_db': low_psnr, 'restored_psnr_db': high_psnr,
                  'recovered_sha256': hashlib.sha256(high).hexdigest(), 'mode_qos_update_preserves_deadline': True})

rows = 400
simple = bytes((32, 64, 96)) * (240 * rows)
for requested, fallback in ((100, 80), (90, 80), (79, 40), (70, 40), (40, 1)):
    attempt_history = [100, 90, 80] if requested == 100 else [requested, fallback]
    retry.Retry_Init(requested, 0, 0, 0, 0)
    retry.Retry_FailAbove(fallback)
    recovery_run(simple, rows, 0, attempt_history)
    for now in (100000, 200000, 1999999):
        recovery_run(simple, rows, now, [fallback], 0)
    recovery_run(simple, rows, 2000000, attempt_history, len(attempt_history))
    for now in (2100000, 2200000, 3999999):
        recovery_run(simple, rows, now, [fallback], 0)
    recovery_run(simple, rows, 4000000, attempt_history, len(attempt_history))
    retry.Retry_FailAbove(0)
    for now in (4100000, 4200000):
        recovery_run(simple, rows, now, [fallback])
    recovery_run(simple, rows, 6000000, [requested])
    cases.append({'case': 'actual-recovery-injected-overflow-probes-wall-clock-bounded',
                  'requested': requested, 'fallback': fallback, 'failed_probe_us': [2000000, 4000000],
                  'successful_probe_us': 6000000})

retry.Retry_Init(100, 0, 0, 0, 0)
retry.Retry_FailAbove(1)
assert recovery_run(simple, rows, 0, [100, 90, 80, 40], 5) == 0
assert retry.Retry_ScreenQuality(1) == 1
for now in (100000, 200000, 300000):
    recovery_run(simple, rows, now, [1], 0)
assert recovery_run(simple, rows, 2000000, [100, 90, 80, 40], 5) == 0
for now in (2100000, 2200000, 2300000):
    recovery_run(simple, rows, now, [1], 0)
retry.Retry_FailAbove(0)
recovery_run(simple, rows, 3600000000, [100], 1)
cases.append({'case': 'actual-recovery-four-attempt-bound-and-long-idle-single-probe',
              'failed_probe_attempts': [100, 90, 80, 40], 'following_frame_quality': 1,
              'long_idle_probe_attempts': [100]})

ticks_per_us = 268123480 // 1000000
cooldown_ticks = 2000000 * ticks_per_us
for origin in (0, 2 ** 32 - cooldown_ticks // 2, 13 * 2 ** 32 - 1, 2 ** 64 - cooldown_ticks // 2):
    retry.Retry_Init(100, 0, 0, 0, 0)
    retry.Retry_FailAbove(80)
    recovery_run(simple, rows, origin, [100, 90, 80], ticks=True)
    for offset in (1, 2, cooldown_ticks - 1):
        recovery_run(simple, rows, origin + offset, [80], 0, ticks=True)
    retry.Retry_FailAbove(0)
    recovery_run(simple, rows, origin + cooldown_ticks, [100], 1, ticks=True)
    cases.append({'case': 'actual-recovery-unsigned-tick-wrap-exact-cooldown',
                  'failure_tick': origin, 'cooldown_ticks': cooldown_ticks})

retry.Retry_Init(100, 0, 0, 0, 0)
retry.Retry_FailAbove(80)
for now in (0, 1, 2):
    recovery_run(simple, rows, now, [100, 90, 80] if now == 0 else [80], ticks=True)
retry.Retry_FailAbove(0)
recovery_run(simple, rows, 120 * 2 ** 32, [100], 1, ticks=True)
recovery_run(simple, rows, 120 * 2 ** 32 + cooldown_ticks - 1, [100], 0, ticks=True)
recovery_run(simple, rows, 120 * 2 ** 32 + cooldown_ticks, [100], 0, ticks=True)
assert retry.Retry_ScreenBytes() == 40
cases.append({'case': 'actual-recovery-long-idle-full-wrap-probes-without-deferral',
              'idle_ticks': 120 * 2 ** 32, 'maximum_deferral_ticks': 0,
              'screen_state_bytes': retry.Retry_ScreenBytes()})

periodic_ticks = 8000000 * ticks_per_us
noise = random.Random(0x134).randbytes(240 * rows * 3)
for origin in (0, 2 ** 32 - periodic_ticks // 2, 13 * 2 ** 32 - 1, 2 ** 64 - periodic_ticks // 2):
    retry.Retry_Init(90, 0, 0, 0, 0)
    recovery_run(noise, rows, origin, [90, 80], ticks=True)
    recovery_run(noise, rows, origin + periodic_ticks - 1, [80], 0, ticks=True)
    recovery_run(noise, rows, origin + periodic_ticks, [90, 80], 2, ticks=True)
    recovery_run(noise, rows, origin + periodic_ticks * 2 - 1, [80], 0, ticks=True)
    recovery_run(noise, rows, origin + periodic_ticks * 2, [90, 80], 2, ticks=True)
    cases.append({'case': 'actual-recovery-periodic-exact-deadline-and-failure-rearm',
                  'failure_tick': origin, 'periodic_ticks': periodic_ticks})

retry.Retry_Init(100, 0, 0, 0, 0)
retry.Retry_FailAbove(80)
for now in (0, 100000, 200000):
    recovery_run(simple, rows, now, [100, 90, 80] if now == 0 else [80])
assert retry.Retry_Settings(100, 0x100, 1048576, 1)
assert retry.Retry_SmallFrames(1) == 3 and retry.Retry_ScreenQuality(1) == 80
recovery_run(simple, rows, 1999999, [80])
assert retry.Retry_Settings(30, 0x102, 2097152, 1)
assert retry.Retry_ScreenQuality(1) == 30 and retry.Retry_ScreenQuality(0) == 30
recovery_run(simple, rows, 8000000, [30])
assert retry.Retry_SmallFrames(1) == 0
assert retry.Retry_Settings(100, 0x102, 2097152, 1)
assert retry.Retry_ScreenQuality(1) == 100 and retry.Retry_SmallFrames(1) == 0
recovery_run(simple, rows, 9000000, [100, 90, 80])
assert not retry.Retry_Settings(100, 0x102, 2097152, 0)
assert retry.Retry_ScreenQuality(1) == 80
cases.append({'case': 'actual-settings-quality-change-replaces-effective-quality-and-lower-ceiling-wins',
              'lowered_quality': 30, 'mode_qos_changes_preserve_cooldown': True})

retry.Retry_Init(100, 0, 0, 0, 0)
retry.Retry_FailAbove(80)
for now in (0, 100000, 200000):
    recovery_run(simple, rows, now, [100, 90, 80] if now == 0 else [80])
assert retry.Retry_Settings(70, 0x102, 2097152, 1)
retry.Retry_FailAbove(40)
recovery_run(simple, rows, 10000000, [70, 40])
for now in (10100000, 10200000, 11999999):
    recovery_run(simple, rows, now, [40])
retry.Retry_FailAbove(0)
recovery_run(simple, rows, 12000000, [70])
cases.append({'case': 'actual-settings-new-fallback-replaces-stale-recovery-history',
              'old_requested': 100, 'new_requested': 70, 'new_fallback': 40,
              'new_failed_tick_us': 10000000, 'earliest_new_probe_us': 12000000})

retry.Retry_Init(100, 0, 0, 0, 0)
noise = random.Random(0x134).randbytes(240 * 400 * 3)
bottom_simple = bytes((32, 64, 96)) * (240 * 320)
recovery_run(noise, 400, 0, [100, 90, 80])
for now in (100000, 200000, 300000, 3000000):
    recovery_run(bottom_simple, 320, now, [100])
assert retry.Retry_ScreenQuality(1) == 80 and retry.Retry_SmallFrames(1) == 0
assert retry.Retry_ScreenQuality(0) == 100 and retry.Retry_SmallFrames(0) == 0
recovery_run(noise, 400, 3100000, [80])
recovery_run(noise, 400, 8000000, [100, 90, 80])
recovery_run(bottom_simple, 320, 16000000, [100])
recovery_run(noise, 400, 15999999, [80])
recovery_run(noise, 400, 16000000, [100, 90, 80])
cases.append({'case': 'actual-recovery-screen-isolation', 'top_quality': 80, 'bottom_quality': 100,
              'top_periodic_probes_us': [8000000, 16000000]})

for stop, pre in ((1, 0), (0, 1)):
    retry.Retry_Init(100, 0, 0, 0, 0)
    retry.Retry_FailAbove(80)
    for now in (0, 100000, 200000):
        recovery_run(simple, rows, now, [100, 90, 80] if now == 0 else [80])
    retry.Retry_Stop(stop, pre, 0, 0)
    assert recovery_run(simple, rows, 2000000, [], 0) == 0
    assert retry.Retry_ScreenQuality(1) == 80 and retry.Retry_SmallFrames(1) == 3
    retry.Retry_Stop(0, 0, stop, pre)
    assert recovery_run(simple, rows, 2000000, [100], 2) == 0
    retry.Retry_Stop(0, 0, 0, 0)
    recovery_run(simple, rows, 2000001, [90, 80], 1)
    cases.append({'case': 'actual-recovery-cancellation-before-and-during-probe', 'stop': stop,
                  'preterminate': pre, 'subsequent_attempts': [90, 80]})

    retry.Retry_Init(90, 0, 0, 0, 0)
    recovery_run(noise, rows, 0, [90, 80])
    retry.Retry_Stop(stop, pre, 0, 0)
    assert recovery_run(noise, rows, 8000000, [], 0) == 0
    retry.Retry_Stop(0, 0, stop, pre)
    assert recovery_run(noise, rows, 8000000, [90], 2) == 0
    retry.Retry_Stop(0, 0, 0, 0)
    recovery_run(noise, rows, 8000001, [80], 0)
    recovery_run(noise, rows, 15999999, [80], 0)
    recovery_run(noise, rows, 16000000, [90, 80], 2)
    cases.append({'case': 'actual-recovery-periodic-cancellation-before-and-during-probe', 'stop': stop,
                  'preterminate': pre, 'next_periodic_probe_us': 16000000})

for requested in (1, 40, 79, 100):
    retry.Retry_Init(requested, 0, 0, 0, 0)
    recovery_run(simple, rows, 0, [requested], 1)
    for now in (2000000, 5000000, 10000000):
        recovery_run(simple, rows, now, [requested], 0)
    assert retry.Retry_SmallFrames(1) == 0
    cases.append({'case': 'actual-recovery-requested-quality-steady-no-table-rebuild', 'requested': requested})

retry.Retry_Init(100, 0, 0, 0, 0)
recovery_run(simple, rows, 0, [100])
assert retry.Retry_SmallFrames(1) == 0 and retry.Retry_SmallFrames(0) == 0
assert retry.Retry_ScreenQuality(1) == 100 and retry.Retry_ScreenQuality(0) == 100
cases.append({'case': 'actual-recovery-restart-clears-screen-history'})
assert hashlib.sha256(source_path.read_bytes()).hexdigest() == source_hash
report = {'all_passed': True, 'cases': cases, 'case_count': len(cases), 'decoder': 'Pillow ' + PIL.__version__,
          'retry_helper_sha256': hashlib.sha256(retry_helper.encode()).hexdigest(),
          'sources': {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
                      for path in (source_path, header_path, remote_path, Path(__file__))},
          'hardware_tested': False}
(generated / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
print(str(len(cases)) + ' JPEG decoder/color/rotation/bounds/recovery cases passed')
