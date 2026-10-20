import argparse
import ctypes as c
import json
import os
import random
import shutil
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--original-library', type=Path)
parser.add_argument('--report', type=Path)
options = parser.parse_args()
root = Path(__file__).resolve().parents[1]
module = root / 'Luma3DS_v13_4/sysmodules/rosalina'
generated = root / 'tests/generated/plugin-legacy-scanner'
generated.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
loader = (module / 'source/plugin/file_loader.c').read_text()
scanner_start = loader.index('typedef struct DestructiveKeyword')
scanner = loader[scanner_start:loader.index('static Result   OpenFile(IFile *file, const char *path)', scanner_start)]
prefix = r'''
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t Result;
typedef struct { u64 pos; } IFile;
typedef struct {
    struct { u32 codeOffset, codeSize, rodataOffset, rodataSize, dataOffset, dataSize; } executable;
} _3gx_Header;
static u8 approvalDisk[512];
static u64 approvalSize;
static bool approvalExists;
static u32 faults, openFiles, prompts, lastRisk;
static u32 nextKeys[8], keyPosition;
static bool menuShouldExit;
#define R_FAILED(r) ((r) < 0)
#define R_SUCCEEDED(r) ((r) >= 0)
#define ARCHIVE_SDMC 0
#define PATH_EMPTY 0
#define PATH_ASCII 0
#define FS_OPEN_READ 1
#define FS_OPEN_WRITE 2
#define FS_OPEN_CREATE 4
#define FS_WRITE_FLUSH 1
#define fsMakePath(t,d) (d)
#define COLOR_TITLE 0
#define COLOR_WHITE 0
#define COLOR_YELLOW 0
#define COLOR_RED 0
#define SPACING_Y 11
#define STR_MATCHED_LOCATIONS 0
#define STR_X_BACK_TO_THE_WARNING 0
#define KEY_A 1
#define KEY_B 2
#define KEY_X 4
static Result IFile_Open(IFile *file, int archive, const char *archivePath, const char *path, int flags)
{
    (void)archive; (void)archivePath;
    assert(!strcmp(path, "/luma/plugins/.approved3gx"));
    if(flags == FS_OPEN_READ && (!approvalExists || (faults & 1))) return -1;
    if((flags & FS_OPEN_WRITE) && (faults & 2)) return -1;
    if(flags & FS_OPEN_CREATE) approvalExists = true;
    file->pos = 0;
    ++openFiles;
    return 0;
}
static void IFile_Close(IFile *file) { (void)file; assert(openFiles); --openFiles; }
static Result IFile_Read(IFile *file, u64 *total, void *buffer, u32 size)
{
    u64 available = file->pos < approvalSize ? approvalSize - file->pos : 0;
    *total = size < available ? size : available;
    if(faults & 4) { *total = 0; return -1; }
    if((faults & 8) && *total) --*total;
    memcpy(buffer, approvalDisk + file->pos, *total);
    file->pos += *total;
    return 0;
}
static Result IFile_Write(IFile *file, u64 *total, const void *buffer, u32 size, int flags)
{
    assert(flags == FS_WRITE_FLUSH);
    assert(file->pos + size <= sizeof(approvalDisk));
    *total = faults & 16 ? 0 : size;
    if((faults & 32) && *total) --*total;
    memcpy(approvalDisk + file->pos, buffer, *total);
    file->pos += *total;
    if(file->pos > approvalSize) approvalSize = file->pos;
    return faults & 16 ? -1 : 0;
}
static Result IFile_SetSize(IFile *file, u64 size)
{
    (void)file;
    if(faults & 64) return -1;
    assert(size <= sizeof(approvalDisk));
    approvalSize = size;
    return 0;
}
static const char *Lang(int id) { (void)id; return ""; }
static const char *Lang_Translate(const char *text) { return text; }
static void menuEnter(void) { ++prompts; }
static void menuLeave(void) {}
static void Draw_Lock(void) {}
static void Draw_Unlock(void) {}
static void Draw_ClearFramebuffer(void) {}
static void Draw_FlushFramebuffer(void) {}
static u32 waitInput(void) { assert(keyPosition < 8); return nextKeys[keyPosition++]; }
static u32 Draw_DrawString(int x, u32 y, int color, const char *text)
{
    (void)x; (void)color;
    unsigned risk;
    if(sscanf(text, "Risk : %u", &risk) == 1) lastRisk = risk;
    return y + 11;
}
static u32 Draw_DrawFormattedString(int x, u32 y, int color, const char *format, ...)
{ (void)x; (void)color; (void)format; return y + 11; }
'''
adapter = r'''
#if defined(_WIN32)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif
EXPORT void fixture_scan(const u8 *buffer, u32 size, u32 base, bool *found, u32 *offsets, u8 *units)
{ ScanForKeywords(buffer, size, base, true, found, offsets, units); }
EXPORT u32 fixture_weight(u32 index) { return g_destructiveKeywords[index].weight; }
EXPORT void fixture_reset(bool eraseDisk)
{
    assert(!openFiles);
    memset(g_approvedHashes, 0, sizeof(g_approvedHashes));
    g_approvedCount = 0;
    g_approvedLoaded = false;
    menuShouldExit = false;
    faults = 0;
    if(eraseDisk)
    {
        memset(approvalDisk, 0, sizeof(approvalDisk));
        approvalSize = 0;
        approvalExists = false;
    }
}
EXPORT u32 fixture_check(const u8 *file, u32 size, const u32 *ranges, u32 keys)
{
    _3gx_Header header = {{ranges[0], ranges[1], ranges[2], ranges[3], ranges[4], ranges[5]}};
    keyPosition = 0;
    lastRisk = UINT32_MAX;
    for(u32 i = 0; i < 8; ++i) { nextKeys[i] = keys & 7; keys >>= 3; }
    u32 before = prompts;
    bool result = CheckForDestructivePlugin(file, &header, size);
    assert(!openFiles);
    return (result ? 1u : 0u) | ((prompts - before) << 1);
}
EXPORT void fixture_remember(u64 hash) { RememberApproval(hash); assert(!openFiles); }
EXPORT bool fixture_approved(u64 hash) { bool result = IsHashApproved(hash); assert(!openFiles); return result; }
EXPORT u32 fixture_count(void) { return g_approvedCount; }
EXPORT u32 fixture_risk(void) { return lastRisk; }
EXPORT u64 fixture_disk_size(void) { return approvalSize; }
EXPORT void fixture_read_disk(u8 *output) { memcpy(output, approvalDisk, approvalSize); }
EXPORT void fixture_set_disk(const u8 *input, u32 size)
{
    assert(size <= sizeof(approvalDisk));
    memcpy(approvalDisk, input, size);
    approvalSize = size;
    approvalExists = true;
}
EXPORT void fixture_faults(u32 flags) { faults = flags; }
EXPORT void fixture_exit(bool value) { menuShouldExit = value; }
'''
source = generated / 'legacy_scanner.c'
source.write_text(prefix + scanner + adapter, encoding='utf-8')
library = generated / ('legacy_scanner.dll' if os.name == 'nt' else 'legacy_scanner.so')
command = [compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-shared']
if os.name != 'nt':
    command.append('-fPIC')
subprocess.run(command + [str(source), '-o', str(library)], check=True, env=env)
dll = c.CDLL(str(library))
dll.fixture_scan.argtypes = [c.c_void_p, c.c_uint32, c.c_uint32, c.POINTER(c.c_bool), c.POINTER(c.c_uint32), c.POINTER(c.c_uint8)]
dll.fixture_weight.argtypes = [c.c_uint32]
dll.fixture_weight.restype = c.c_uint32
dll.fixture_reset.argtypes = [c.c_bool]
dll.fixture_check.argtypes = [c.c_void_p, c.c_uint32, c.POINTER(c.c_uint32), c.c_uint32]
dll.fixture_check.restype = c.c_uint32
dll.fixture_remember.argtypes = [c.c_uint64]
dll.fixture_approved.argtypes = [c.c_uint64]
dll.fixture_approved.restype = c.c_bool
dll.fixture_count.restype = c.c_uint32
dll.fixture_risk.restype = c.c_uint32
dll.fixture_disk_size.restype = c.c_uint64
dll.fixture_read_disk.argtypes = [c.c_void_p]
dll.fixture_set_disk.argtypes = [c.c_void_p, c.c_uint32]
dll.fixture_faults.argtypes = [c.c_uint32]
dll.fixture_exit.argtypes = [c.c_bool]
keywords = [('boot9strap', 40), ('boot.firm', 40), ('private', 25), ('cias', 20), ('gm9', 20), ('fbi', 20), ('Nintendo 3DS', 15), ('DCIM', 10), ('JKSV', 10), ('Splashes', 10)]
assert [dll.fixture_weight(i) for i in range(10)] == [weight for _, weight in keywords]


def scan(segments, reference=None):
    found = (c.c_bool * 10)()
    offsets = (c.c_uint32 * 10)()
    units = (c.c_uint8 * 10)()
    for base, data in segments:
        buffer = c.create_string_buffer(bytes(data))
        if reference:
            reference.audit_scan(buffer, len(data), base, True, found, offsets, units)
        else:
            dll.fixture_scan(buffer, len(data), base, found, offsets, units)
    return tuple(found), tuple(offsets), tuple(units)


def fnv(segments):
    digest = 0xCBF29CE484222325
    for segment in segments:
        for value in segment:
            digest = ((digest ^ value) * 0x100000001B3) & ((1 << 64) - 1)
    return digest


def check(data, spans, keys=2):
    assert len(spans) == 3
    ranges = (c.c_uint32 * 6)(*(value for span in spans for value in span))
    result = dll.fixture_check(c.create_string_buffer(bytes(data)), len(data), ranges, keys)
    return bool(result & 1), result >> 1


def disk():
    output = c.create_string_buffer(dll.fixture_disk_size())
    dll.fixture_read_disk(output)
    return output.raw


corpus = []
for index, (keyword, _) in enumerate(keywords):
    for unit, encoding in ((1, 'ascii'), (2, 'utf-16-le')):
        for spelling in (keyword, keyword.swapcase()):
            for prefix_text in ('/', ':', '\\', '', 'x'):
                for suffix in ('/', '"', '\0', '', '\\child', '.bak', '-notes', 'x'):
                    data = (prefix_text + spelling + suffix).encode(encoding)
                    expected = prefix_text in ('/', ':') and suffix in ('/', '"', '\0', '')
                    result = scan([(0x100, data)])
                    assert result[0][index] == expected, (keyword, unit, prefix_text, suffix, result)
                    assert sum(result[0]) == int(expected)
                    if expected:
                        assert result[1][index] == 0x100 + unit and result[2][index] == unit
                    corpus.append([(0x100, data)])
        for position in (0, 1, 8160, 8170, 8180, 8190, 8191, 8192, 8200, 16350):
            data = b'x' * position + ('/' + keyword + '\0').encode(encoding)
            result = scan([(0x100, data)])
            assert result[0][index] and result[1][index] == 0x100 + position + unit
            corpus.append([(0x100, data)])
        encoded = ('/' + keyword).encode(encoding)
        for cut in range(len(encoded)):
            result = scan([(0x100, encoded[:cut])])
            assert not any(result[0])
            corpus.append([(0x100, encoded[:cut])])
        if unit == 2:
            for high in range(1, len(encoded), 2):
                invalid = bytearray(encoded)
                invalid[high] = 1
                result = scan([(0x100, invalid)])
                assert not result[0][index]
                corpus.append([(0x100, invalid)])
assert not any(scan([(0x100, b'/boot.'), (0x200, b'firm\0')])[0])
assert scan([(0x100, b'/boot.firm\0/boot.firm\0')])[1][1] == 0x101
assert scan([(0x100, b'/boot.firm\0'), (0x200, b'/boot.firm\0')])[1][1] == 0x101
rng = random.Random(0x1304)
for _ in range(500):
    keyword, _ = rng.choice(keywords)
    encoding = rng.choice(('ascii', 'utf-16-le'))
    text = (rng.choice(('/', ':', '\\')) + keyword.swapcase() + rng.choice(('/', '"', '\0', '', '.bak', '\\child'))).encode(encoding)
    position = rng.choice((0, 1, 8160, 8170, 8180, 8190, 8191, 8192, 8200, 16350))
    data = bytes(rng.randrange(256) for _ in range(position)) + text
    if rng.choice((True, False)):
        data += b'\0' * 64
    corpus.append([(0x100, data)])
reference_cases = 0
if options.original_library:
    original = c.CDLL(str(options.original_library.resolve()))
    original.audit_scan.argtypes = [c.c_void_p, c.c_uint32, c.c_uint32, c.c_bool, c.POINTER(c.c_bool), c.POINTER(c.c_uint32), c.POINTER(c.c_uint8)]
    for segments in corpus:
        assert scan(segments) == scan(segments, original), segments
        reference_cases += 1

dll.fixture_reset(True)
for keyword, weight in keywords:
    for encoding in ('ascii', 'utf-16-le'):
        data = ('/' + keyword + '\0').encode(encoding)
        assert check(data, [(0, len(data)), (0, 0), (0, 0)]) == (False, 1)
        assert dll.fixture_risk() == weight
for data, risk in ((b'/boot.firm\0/boot.firm\0', 40),
                   (b'/boot.firm\0/boot9strap/\0', 80),
                   (b'/boot.firm\0/boot9strap/\0/private/\0', 100)):
    assert check(data, [(0, len(data)), (0, 0), (0, 0)]) == (False, 1)
    assert dll.fixture_risk() == risk
metadata = b'author /boot.firm\0description /boot9strap/\0symbol /private/\0'
normal = b'normal plugin data'
raw = metadata + normal + b'/boot.firm\0'
spans = [(len(metadata), len(normal)), (len(raw), 0), (len(raw), 0)]
assert check(raw, spans) == (True, 0)
for segment in range(3):
    active = [(0, 0), (0, 0), (0, 0)]
    active[segment] = (len(metadata) + len(normal), 11)
    assert check(raw, active) == (False, 1)
assert check(b'/boot.firm\0', [(0, 6), (6, 5), (11, 0)]) == (True, 0)
encoded_raw = bytes(value ^ 0xAA for value in b'/boot.firm\0')
assert check(encoded_raw, [(0, len(encoded_raw)), (0, 0), (0, 0)]) == (True, 0)
for invalid_span in ((len(raw) + 1, 0), (len(raw), 1), (0xFFFFFFFF, 2), (1, 0xFFFFFFFF)):
    assert check(raw, [invalid_span, (0, 0), (0, 0)]) == (False, 0)
assert check(b'/boot.firm\0', [(0, 11), (0, 0), (0, 0)], 4 | (2 << 3)) == (False, 1)
wide = b'X' + '/boot.firm\0'.encode('utf-16-le')
assert check(wide, [(1, len(wide) - 1), (0, 0), (0, 0)], 4 | (2 << 3)) == (False, 1)
dll.fixture_exit(True)
assert check(b'/boot.firm\0', [(0, 11), (0, 0), (0, 0)]) == (False, 1)
dll.fixture_exit(False)

dll.fixture_reset(True)
segments = [b'/boot.firm\0', b'/private/\0', b'/DCIM/\0']
raw = metadata + b''.join(segments)
spans = []
position = len(metadata)
for segment in segments:
    spans.append((position, len(segment)))
    position += len(segment)
digest = fnv(segments)
assert check(raw, spans, 1) == (True, 1)
assert dll.fixture_count() == 1 and disk() == digest.to_bytes(8, 'little')
assert check(raw, spans) == (True, 0)
dll.fixture_reset(False)
assert check(raw, spans) == (True, 0)
changed_metadata = bytes([raw[0] ^ 1]) + raw[1:]
assert check(changed_metadata, spans) == (True, 0)
for offset, _ in spans:
    changed = bytearray(raw)
    changed[offset + 1] ^= 0x20
    assert check(changed, spans) == (False, 1)
assert disk() == digest.to_bytes(8, 'little')

dll.fixture_reset(True)
for value in range(1, 34):
    dll.fixture_remember(value)
assert dll.fixture_count() == 32
assert disk() == b''.join(value.to_bytes(8, 'little') for value in range(2, 34))
dll.fixture_remember(20)
assert dll.fixture_count() == 32
dll.fixture_reset(False)
assert not dll.fixture_approved(1)
assert all(dll.fixture_approved(value) for value in range(2, 34))
assert dll.fixture_count() == 32
legacy_record = (0x1234567890ABCDEF).to_bytes(8, 'little')
for tail in (b'', b'\xAA', b'\xAA' * 7):
    dll.fixture_reset(True)
    payload = legacy_record + tail
    dll.fixture_set_disk(c.create_string_buffer(payload), len(payload))
    assert dll.fixture_approved(0x1234567890ABCDEF) and dll.fixture_count() == 1
oversized = b''.join(value.to_bytes(8, 'little') for value in range(1, 65))
dll.fixture_reset(True)
dll.fixture_set_disk(c.create_string_buffer(oversized), len(oversized))
assert dll.fixture_approved(32) and not dll.fixture_approved(33) and dll.fixture_count() == 32
for fault in (1, 4, 8):
    dll.fixture_reset(True)
    dll.fixture_set_disk(c.create_string_buffer(legacy_record), len(legacy_record))
    dll.fixture_faults(fault)
    assert not dll.fixture_approved(0x1234567890ABCDEF) and dll.fixture_count() == 0
for fault in (2, 16, 32, 64):
    dll.fixture_reset(True)
    dll.fixture_faults(fault)
    dll.fixture_remember(0x1234567890ABCDEF)
    assert dll.fixture_approved(0x1234567890ABCDEF)
    dll.fixture_reset(False)
    restored = dll.fixture_approved(0x1234567890ABCDEF)
    if fault in (2, 16, 32):
        assert not restored
    if fault == 64:
        assert restored

assert loader.index('CheckForDestructivePlugin(snapshot') < loader.index('Read_3gx_LoadSegments(snapshot')
assert 'ScanPluginFile' not in loader and 'ConfirmSensitivePluginPaths' not in loader
report = {'result': 'passed', 'host_fixture': True, 'device_tested': False,
          'scanner_corpus_cases': len(corpus), 'original_C_differential_cases': reference_cases,
          'checked': ['ten legacy keywords and weights', 'ASCII and UTF-16LE case and boundaries',
                      'unique keyword risk sum and 100 percent cap', 'invalid span rejection',
                      'raw code, rodata, data spans', 'metadata exclusion and decoded image exclusion',
                      'segment separation', 'FNV1a64 raw-span binding', 'legacy approval file format',
                      'approval survives restart', 'raw span changes warn again', '32 entry FIFO',
                      'duplicate approval', 'missing, partial and failed approval I/O', 'cancel and match detail view'],
          'limits': ['Host scanner fixture does not run FIRM, embedded ARM payloads, or a physical 3DS.',
                     'Approval save failures retain the approval in RAM for this boot, matching legacy behavior.']}
if options.report:
    options.report.parent.mkdir(parents=True, exist_ok=True)
    options.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(f'Legacy plugin scanner: {len(corpus)} corpus cases, {reference_cases} original C comparisons, raw spans and persistent approvals passed')
