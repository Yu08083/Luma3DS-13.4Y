import ctypes
import os
import random
import shutil
import struct
import subprocess
from pathlib import Path

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_CPSR

root = Path(__file__).resolve().parents[1]
module = root / 'Luma3DS_v13_4/sysmodules/rosalina'
generated = root / 'tests/generated'
generated.mkdir(exist_ok=True)
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
library = generated / ('plugin-payload.dll' if os.name == 'nt' else 'plugin-payload.so')
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-shared', '-fPIC', '-I', str(module / 'include'), str(module / 'source/plugin/payload_vm.c'), '-o', str(library)], check=True, env=env)
vm = ctypes.CDLL(str(library))
class Program(ctypes.Structure):
    _fields_ = [('code', ctypes.c_uint32 * 32), ('count', ctypes.c_uint32)]
vm.PluginPayload_Set.argtypes = [ctypes.POINTER(Program), ctypes.POINTER(ctypes.c_uint32)]
vm.PluginPayload_Set.restype = ctypes.c_bool
vm.PluginPayload_Run.argtypes = [ctypes.POINTER(Program), ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
vm.PluginPayload_Run.restype = ctypes.c_bool

def run(words, initial, args=(0, 0, 0, 0)):
    program = Program()
    encoded = (ctypes.c_uint32 * 32)(*words, 0xE320F000)
    assert vm.PluginPayload_Set(ctypes.byref(program), encoded)
    data = ctypes.create_string_buffer(initial, len(initial))
    output = ctypes.c_uint32(0xEFFE)
    arguments = (ctypes.c_uint32 * 4)(*args)
    success = vm.PluginPayload_Run(ctypes.byref(program), data, len(initial), arguments, ctypes.byref(output))
    return success, data.raw, output.value

def emulate(words, initial, args):
    emu = Uc(UC_ARCH_ARM, UC_MODE_ARM)
    for address in [0x10000000, 0x20000000, 0x30000000, 0x40000000, 0x50000000]: emu.mem_map(address, 0x1000)
    emu.mem_write(0x10000000, initial)
    emu.mem_write(0x20000000, struct.pack('<4I', *args))
    emu.mem_write(0x40000000, struct.pack('<' + 'I' * len(words), *words))
    emu.reg_write(UC_ARM_REG_CPSR, 0x10)
    for register, value in [(UC_ARM_REG_R0, 0x10000000), (UC_ARM_REG_R1, 0x10000000 + len(initial)), (UC_ARM_REG_R2, 0x20000000), (UC_ARM_REG_SP, 0x30000100), (UC_ARM_REG_LR, 0x50000000)]:
        emu.reg_write(register, value)
    emu.emu_start(0x40000000, 0x50000000, count=20000)
    return bytes(emu.mem_read(0x10000000, len(initial))), emu.reg_read(UC_ARM_REG_R0)

sum_words = [0xE92D40C0, 0xE3A07000, 0xE4906004, 0xE0877006, 0xE1500001, 0x1AFFFFFB, 0xE1A00007, 0xE8BD80C0]
data = struct.pack('<16I', *range(16))
success, unchanged, checksum = run(sum_words, data)
assert success and unchanged == data and checksum == 120

rng = random.Random(1334)
for _ in range(400):
    words = [0xE1A0C000, 0xE8920078]
    for _ in range(18):
        operation = rng.randrange(16)
        condition = rng.choice([14] * 6 + list(range(14)))
        set_flags = operation in range(8, 12) or rng.randrange(2)
        rn, rd, rm = [rng.randrange(3, 11) for _ in range(3)]
        if operation in range(8, 12): rd = 0
        if operation in (13, 15): rn = 0
        value = condition << 28 | operation << 21 | int(set_flags) << 20 | rn << 16 | rd << 12
        if rng.randrange(2):
            value |= 1 << 25 | rng.randrange(4096)
        elif rng.randrange(2):
            value |= rng.randrange(32) << 7 | rng.randrange(4) << 5 | rm
        else:
            value |= rng.randrange(3, 11) << 8 | rng.randrange(4) << 5 | 16 | rm
        words.append(value)
    words += [0xE88C07F8, 0xE1A00003, 0xE12FFF1E]
    arguments = tuple(rng.getrandbits(32) for _ in range(4))
    success, actual, checksum = run(words, bytes(64), arguments)
    expected, expected_checksum = emulate(words, bytes(64), arguments)
    assert success and (actual, checksum) == (expected, expected_checksum), (words, arguments, actual.hex(), expected.hex(), checksum, expected_checksum)

for words in [
    [0xEF000001, 0xE12FFF1E],
    [0xEE070F15, 0xE12FFF1E],
    [0xE3A00000, 0xE5800000, 0xE12FFF1E],
    [0xE5820000, 0xE12FFF1E],
    [0xE3A0D000, 0xE92D4000, 0xE12FFF1E],
    [0xEAFFFFFE],
    [0xE3A0F000],
    [0xE3A00001, 0xE12FFF10],
]:
    success, actual, _ = run(words, bytes(64))
    assert not success and actual == bytes(64), [hex(word) for word in words]
program = Program()
assert not vm.PluginPayload_Set(ctypes.byref(program), (ctypes.c_uint32 * 32)(*[0xE1A00000] * 32))
assert not vm.PluginPayload_Set(ctypes.byref(program), (ctypes.c_uint32 * 32)(0xE320F000))
print('Plugin payload: 400 independent ARM comparisons, checksum, forbidden system calls, memory bounds, read-only args, stack bounds, invalid returns and loop budget passed')
