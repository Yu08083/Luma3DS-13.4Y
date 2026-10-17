import argparse
import ctypes
import os
import random
import shutil
import struct
import subprocess
import zipfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
module = root / 'Luma3DS_v13_4/sysmodules/rosalina'
parser = argparse.ArgumentParser()
parser.add_argument('--plugin-zip', action='append', type=Path, default=[])
args = parser.parse_args()
generated = root / 'tests/generated'
generated.mkdir(exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt': compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
library = generated / ('plugin-format.dll' if os.name == 'nt' else 'plugin-format.so')
subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-shared', '-fPIC', '-I', str(module / 'include'), str(module / 'source/plugin/file_format.c'), str(module / 'source/plugin/payload_vm.c'), '-o', str(library)], check=True, env=env)
vm = ctypes.CDLL(str(library))
class Layout(ctypes.Structure):
    _fields_ = [('image', ctypes.c_uint32), ('executable', ctypes.c_uint32), ('snapshot', ctypes.c_uint32)]
vm.PluginFile_Validate.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(Layout)]
vm.PluginFile_Validate.restype = ctypes.c_bool
vm.PluginFile_ValidateStrings.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
vm.PluginFile_ValidateStrings.restype = ctypes.c_bool
class Program(ctypes.Structure):
    _fields_ = [('code', ctypes.c_uint32 * 32), ('count', ctypes.c_uint32)]
vm.PluginPayload_Set.argtypes = [ctypes.POINTER(Program), ctypes.POINTER(ctypes.c_uint32)]
vm.PluginPayload_Set.restype = ctypes.c_bool
vm.PluginPayload_Run.argtypes = [ctypes.POINTER(Program), ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
vm.PluginPayload_Run.restype = ctypes.c_bool
sum_program = (0xE92D40C0, 0xE3A07000, 0xE4906004, 0xE0877006, 0xE1500001, 0x1AFFFFFB, 0xE1A00007, 0xE8BD80C0)

def valid(data, memory=5 * 1024 * 1024):
    layout = Layout()
    buffer = ctypes.create_string_buffer(bytes(data))
    return vm.PluginFile_Validate(buffer, len(data), memory, 0x100, ctypes.byref(layout)) and vm.PluginFile_ValidateStrings(buffer, len(data))

def verify_sum_payloads(data):
    word = lambda offset: struct.unpack_from('<I', data, offset)[0]
    code_offset = word(0x58)
    image_size = sum(word(offset) for offset in (0x64, 0x68, 0x6C))
    original = bytes(data[code_offset:code_offset + image_size])
    expected = sum(value[0] for value in struct.iter_unpack('<I', original)) & 0xFFFFFFFF
    assert expected == word(0x34)
    for field, arguments_offset in ((0x74, 0x38), (0x78, 0x48), (0x7C, 0x48)):
        offset = word(field)
        assert struct.unpack_from('<8I', data, offset) == sum_program
        encoded = (ctypes.c_uint32 * 32)()
        length = min(128, code_offset - offset)
        ctypes.memmove(encoded, bytes(data[offset:offset + length]), length)
        program = Program()
        assert vm.PluginPayload_Set(ctypes.byref(program), encoded) and program.count == 8
        image = ctypes.create_string_buffer(original, len(original))
        arguments = (ctypes.c_uint32 * 4)(*struct.unpack_from('<4I', data, arguments_offset))
        checksum = ctypes.c_uint32()
        assert vm.PluginPayload_Run(ctypes.byref(program), image, len(original), arguments, ctypes.byref(checksum))
        assert checksum.value == expected and image.raw == original

def metadata_fixture(trailing):
    result = bytearray(0x3C0)
    struct.pack_into('<QII', result, 0, 0x3230303024584733, 0x70200, 0)
    struct.pack_into('<I', result, 0x30, 3)
    struct.pack_into('<10I', result, 0x58, 0x240, 0x340, 0x380, 0x100, 0x40, 0x40, 0x400, 0x1C7, 0x1EB, 0x20F)
    for offset in (0x1C7, 0x1EB, 0x20F):
        struct.pack_into('<9I', result, offset, *sum_program, 0xE320F000)
    for offset in range(0x240, 0x3C0, 4):
        struct.pack_into('<I', result, offset, (offset * 0x71498107 + 0x12345678) & 0xFFFFFFFF)
    checksum = sum(value[0] for value in struct.iter_unpack('<I', result[0x240:])) & 0xFFFFFFFF
    struct.pack_into('<I', result, 0x34, checksum)
    offset = len(result) if trailing else 0x94
    for field, length, value in ((0x10, 38, 65), (0x18, 20, 84), (0x20, 38, 83), (0x28, 102, 68)):
        text = bytes([value]) * (length - 1) + b'\0'
        struct.pack_into('<2I', result, field, length, offset)
        result[offset:offset + length] = text
        offset += length
    return result

for at_end in (False, True):
    fixture = metadata_fixture(at_end)
    assert valid(fixture), ('trailing_metadata', at_end)
    verify_sum_payloads(fixture)
    for field in (0x10, 0x18, 0x20, 0x28):
        length, offset = struct.unpack_from('<2I', fixture, field)
        altered = bytearray(fixture)
        altered[offset + length - 1] = 33
        assert not valid(altered), ('unterminated_metadata', at_end, field)
        for bad_length, bad_offset in ((length, 0x93), (length, len(fixture) - length + 1), (length, 0xFFFFFFF0), (0xFFFFFFFF, offset)):
            altered = bytearray(fixture)
            struct.pack_into('<2I', altered, field, bad_length, bad_offset)
            assert not valid(altered), ('metadata_bounds', at_end, field, bad_length, bad_offset)
    if at_end:
        assert not valid(fixture[:-1])

data = bytearray(0x400)
struct.pack_into('<QII', data, 0, 0x3230303024584733, 0x70200, 0)
struct.pack_into('<4I', data, 0x10, 7, 0x94, 7, 0x9C)
data[0x94:0x9B] = b'author\0'
data[0x9C:0xA3] = b'plugin\0'
struct.pack_into('<I', data, 0x30, 0x23)
struct.pack_into('<10I', data, 0x58, 0x200, 0x300, 0x380, 0x100, 0x80, 0x80, 0x400, 0xB0, 0xBC, 0xC8)
for offset in (0xB0, 0xBC, 0xC8): struct.pack_into('<3I', data, offset, 0xE3A00000, 0xE12FFF1E, 0xE320F000)
assert valid(data)
for field, value in [(0, 0), (4, 0), (12, 1), (0x30, 0xFFFFFFFF), (0x30, 0x33), (0x58, 0), (0x58, 0x204), (0x58, 0xFFFFFF00), (0x64, 0), (0x64, 0xFFFFFFFC), (0x68, 0xFFFFFFFF), (0x6C, 0xFFFFFFFC), (0x70, 0xFFFFFFFF), (0x80, 129), (0x80, 0x40000001), (0x10, 257), (0x14, 0xFFFFFFF0), (0x74, 0), (0x74, 0x204), (0x78, 0xFFFFFFFC), (0x7C, 0x1FF)]:
    altered = bytearray(data)
    struct.pack_into('<I', altered, field, value)
    assert not valid(altered), (hex(field), hex(value))
altered = bytearray(data)
altered[0x94:0x9B] = b'author!'
assert not valid(altered)
altered = bytearray(data)
struct.pack_into('<I', altered, 0x74, 0x1FC)
struct.pack_into('<I', altered, 0x1FC, 0xE12FFF1E)
struct.pack_into('<I', altered, 0x200, 0xE320F000)
assert not valid(altered)
for size in range(0x94): assert not valid(data[:size])
assert not valid(data, 0x1000)
large = bytearray(data) + bytes(3 * 1024 * 1024)
struct.pack_into('<I', large, 0x6C, len(large) - 0x380)
assert valid(large)
trailing = bytearray(data) + struct.pack('<I', 0x00155100)
struct.pack_into('<2I', trailing, 0x80, 1, len(data))
assert valid(trailing)
rng = random.Random(134)
for _ in range(20000):
    altered = bytearray(data)
    for _ in range(rng.randrange(1, 5)):
        struct.pack_into('<I', altered, rng.randrange(0x94 // 4) * 4, rng.getrandbits(32))
    valid(altered)
print('Plugin format: valid packed payloads, large in-place images, short reads, overflows, bounds, strings, terminators and 20000 malformed headers passed')
print('Plugin metadata: all four fields before/after the image, unaligned sum8 payloads, NUL termination and out-of-file/overflow rejection passed')
for archive in args.plugin_zip:
    count = 0
    with zipfile.ZipFile(archive) as source:
        for entry in source.infolist():
            if not entry.filename.lower().endswith('.3gx'):
                continue
            plugin = source.read(entry)
            assert valid(plugin), (archive.name, entry.filename, 'format')
            verify_sum_payloads(plugin)
            count += 1
    assert count, archive.name
    print(f'Plugin ZIP: {archive.name}: {count} actual C validator and unaligned sum8 payload checks passed')
