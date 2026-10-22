import argparse
import ctypes as c
import hashlib
import json
import os
from pathlib import Path
import random
import shutil
import struct
import subprocess
import tempfile

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_INTR, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_C13_C0_3, UC_ARM_REG_CPSR, UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_SP

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / 'Luma3DS_v13_4/sysmodules/rosalina'
MEMORY = 5 * 1024 * 1024
BASE = 0x07000100
CASES = []
LOOP = (0xE59D3014, 0xE1CD80F0, 0xE58D3038, 0xE59D3018, 0xE3A02002, 0xE58D303C, 0xE1A01004, 0xE3A03000, 0xE28D0020, 0xE58D5020, 0xEB000000, 0xE59D3020, 0xE3530000, 0x1A00000B, 0xEB000000, 0xEAFFFFEF)
ARBITRATE = (0xE92D0030, 0xE59D4008, 0xE59D500C, 0xEF000022, 0xE8BD0030, 0xE12FFF1E)
CONNECT = (0xE52D0004, 0xEF00002D, 0xE49D3004, 0xE5831000, 0xE12FFF1E)


def word(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def put(data, offset, value):
    struct.pack_into('<I', data, offset, value)


def words(data, offset, values):
    struct.pack_into('<' + 'I' * len(values), data, offset, *values)


def branch(source, target):
    return 0xEB000000 | ((target - source - 8) >> 2 & 0xFFFFFF)


def fixture():
    data = bytearray(0x2080)
    words(data, 0, (0x24584733, 0x31303030, 0x70000, 0x2000, 0x80, 7, 0x3C, 7, 0x44, 0, 0, 0, 0, 1, 0x4C))
    data[0x3C:0x43] = b'author\0'
    data[0x44:0x4B] = b'plugin\0'
    put(data, 0x4C, 0x00155100)
    for offset, values in ((0x800, LOOP), (0x100, ARBITRATE), (0x140, CONNECT)):
        words(data, 0x80 + offset, values)
    for offset in (0x180, 0x1C0):
        put(data, 0x80 + offset, 0xE12FFF1E)
    put(data, 0x80 + 0x800 - 724, branch(0x800 - 724, 0x180))
    put(data, 0x80 + 0x800 + 56, branch(0x800 + 56, 0x1C0))
    words(data, 0x80 + 0x800 + 176, (0xE59D001C, branch(0x8B4, 0x180), 0xE5960008, branch(0x8BC, 0x180)))
    return data


def record(name, **details):
    CASES.append({'name': name, 'passed': True, **details})


class Layout(c.Structure):
    _fields_ = [('image', c.c_uint32), ('executable', c.c_uint32), ('snapshot', c.c_uint32)]


class Program(c.Structure):
    _fields_ = [('code', c.c_uint32 * 32), ('count', c.c_uint32)]


class NativeShim:
    def __init__(self, data, old_size, kernel, default):
        code = word(data, 0x58)
        self.uc = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        self.uc.mem_map(0x07000000, (len(data) + 0x10FF) & ~0xFFF)
        self.uc.mem_write(BASE, bytes(data[code:]))
        self.uc.mem_map(0x03000000, 0x1000)
        self.uc.mem_map(0x04000000, 0x20000)
        self.uc.mem_map(0x05000000, 0x1000)
        self.uc.mem_map(0x1FF82000, 0x1000)
        self.uc.reg_write(UC_ARM_REG_C13_C0_3, 0x1FF82000)
        self.uc.reg_write(UC_ARM_REG_CPSR, 0x10)
        self.uc.mem_write(0x07000000, struct.pack('<8I', 0x24584733, 0x70000, 0x06000000, 0x300000, 0x200000, default, 0x03000000, 0x03000004))
        self.storage = BASE + old_size
        self.fetch = self.storage + 16
        self.reply = self.fetch + 36
        self.init = self.reply + 184
        self.init_ex = self.init + 88
        self.kernel = BASE + kernel
        self.events = []
        self.kernel_calls = 0
        self.uc.hook_add(UC_HOOK_INTR, self.interrupt)
        self.uc.hook_add(UC_HOOK_CODE, self.trace)

    def read(self, address):
        return word(self.uc.mem_read(address, 4), 0)

    def write(self, address, value):
        self.uc.mem_write(address, struct.pack('<I', value & 0xFFFFFFFF))

    def trace(self, uc, address, size, data):
        if address == self.kernel:
            self.kernel_calls += 1
            uc.reg_write(UC_ARM_REG_PC, uc.reg_read(UC_ARM_REG_LR))

    def interrupt(self, uc, number, data):
        opcode = self.read(uc.reg_read(UC_ARM_REG_PC) - 4) & 0xFFFFFF
        event = {'svc': opcode, 'r0': uc.reg_read(UC_ARM_REG_R0), 'r1': uc.reg_read(UC_ARM_REG_R1), 'r2': uc.reg_read(UC_ARM_REG_R2), 'r3': uc.reg_read(UC_ARM_REG_R3)}
        if opcode == 0x2D:
            pointer = event['r1']
            assert bytes(uc.mem_read(pointer, 8)) == b'plg:ldr\0'
            uc.reg_write(UC_ARM_REG_R1, 0x31)
        elif opcode == 0x32:
            assert event['r0'] == 0x31 and self.read(0x1FF82080) == 0x90000
            self.write(0x1FF8208C, 0x41)
        elif opcode == 0x22:
            assert event['r0'] == 0x41
            if event['r2'] == 1:
                self.write(0x03000000, 0)
        elif opcode == 9:
            uc.emu_stop()
        else:
            raise AssertionError(event)
        self.events.append(event)
        uc.reg_write(UC_ARM_REG_R0, 0)

    def call(self, address, argument=0, exits=False):
        for register, value in ((UC_ARM_REG_SP, 0x04010000), (UC_ARM_REG_LR, 0x05000000), (UC_ARM_REG_R0, argument), (UC_ARM_REG_R4, 0x44444444), (UC_ARM_REG_R5, 0x55555555), (UC_ARM_REG_R6, 0x66666666)):
            self.uc.reg_write(register, value)
        self.uc.emu_start(address, 0x05000000, count=10000)
        if not exits:
            assert self.uc.reg_read(UC_ARM_REG_PC) == 0x05000000
            assert self.uc.reg_read(UC_ARM_REG_SP) == 0x04010000
            for register, value in ((UC_ARM_REG_R4, 0x44444444), (UC_ARM_REG_R5, 0x55555555), (UC_ARM_REG_R6, 0x66666666)):
                assert self.uc.reg_read(register) == value
        return self.uc.reg_read(UC_ARM_REG_R0)


def run(library, sample):
    library.PluginLegacy_ValidateHeader.argtypes = [c.c_void_p, c.c_uint32, c.c_uint32]
    library.PluginLegacy_ValidateHeader.restype = c.c_bool
    library.PluginLegacy_Validate.argtypes = library.PluginLegacy_ValidateHeader.argtypes
    library.PluginLegacy_Validate.restype = c.c_bool
    library.PluginLegacy_Convert.argtypes = [c.c_void_p, c.c_uint32, c.c_uint32, c.POINTER(c.c_uint32)]
    library.PluginLegacy_Convert.restype = c.c_bool
    library.PluginFile_Validate.argtypes = [c.c_void_p, c.c_uint32, c.c_uint32, c.c_uint32, c.POINTER(Layout)]
    library.PluginFile_Validate.restype = c.c_bool
    library.PluginFile_ValidateStrings.argtypes = [c.c_void_p, c.c_uint32]
    library.PluginFile_ValidateStrings.restype = c.c_bool
    library.PluginPayload_Set.argtypes = [c.POINTER(Program), c.POINTER(c.c_uint32)]
    library.PluginPayload_Set.restype = c.c_bool
    library.PluginPayload_Run.argtypes = [c.POINTER(Program), c.c_void_p, c.c_uint32, c.POINTER(c.c_uint32), c.POINTER(c.c_uint32)]
    library.PluginPayload_Run.restype = c.c_bool

    def convert(raw, memory=MEMORY, expected=True):
        guard = b'\xA5' * 32
        buffer = c.create_string_buffer(guard + bytes(raw) + bytes(max(0, memory - len(raw))) + guard)
        pointer = c.byref(buffer, len(guard))
        original = buffer.raw
        output = c.c_uint32(0xBAD0)
        result = library.PluginLegacy_Convert(pointer, len(raw), memory, c.byref(output))
        assert result == expected, (result, expected, memory, hashlib.sha256(raw).hexdigest())
        assert buffer.raw[:32] == guard and buffer.raw[32 + max(memory, len(raw)):32 + max(memory, len(raw)) + 32] == guard
        if not result:
            assert buffer.raw == original and output.value == 0xBAD0
            return None
        converted = buffer.raw[32:32 + output.value]
        layout = Layout()
        assert library.PluginFile_Validate(pointer, output.value, memory, 0x100, c.byref(layout))
        assert library.PluginFile_ValidateStrings(pointer, output.value)
        assert layout.executable < memory and layout.image == word(raw, 0xC) + 360
        for field in (0x74, 0x78, 0x7C):
            payload = (c.c_uint32 * 32)()
            encoded = converted[word(converted, field):word(converted, 0x58)][:128]
            c.memmove(payload, encoded, len(encoded))
            program = Program()
            assert library.PluginPayload_Set(c.byref(program), payload)
            image = c.create_string_buffer(converted[word(converted, 0x58):], layout.image)
            checksum = c.c_uint32(0xBAD0)
            assert library.PluginPayload_Run(c.byref(program), image, layout.image, (c.c_uint32 * 4)(), c.byref(checksum))
            assert checksum.value == 0 and image.raw == converted[word(converted, 0x58):]
        return converted

    raw = fixture()
    converted = convert(raw)
    assert word(converted, 4) == 0x32303030
    assert converted[word(converted, 0x14):word(converted, 0x14) + 7] == b'author\0'
    assert converted[word(converted, 0x1C):word(converted, 0x1C) + 7] == b'plugin\0'
    assert word(converted, word(converted, 0x84)) == 0x00155100
    protected = set(range(0x800 - 724, 0x800 - 720)) | set(range(0x800, 0x838)) | set(range(0x8B0, 0x8C0))
    original_image = raw[word(raw, 0x10):]
    new_image = converted[word(converted, 0x58):]
    assert all(a == b for offset, (a, b) in enumerate(zip(original_image, new_image)) if offset not in protected)
    record('bounded_legacy_to_modern_metadata_targets_payload_and_only_declared_ARM_sites_change')
    mutations = [(0, 0), (4, 0x32303030), (0xC, 0), (0xC, 0xFFFFFFFC), (0x10, 0xFFFFFFFF), (0x14, 257), (0x18, 0x3B), (0x1C, 257), (0x20, len(raw)), (0x24, 0xFFFFFFFF), (0x34, 129), (0x38, len(raw)), (0x80 + 0x800, 0), (0x80 + 0x140, 0), (0x80 + 0x100, 0), (0x80 + 0x800 - 724, 0xEA000000), (0x80 + 0x800 - 724, 0xEB7FFFFF), (0x80 + 0x838, 0xEB7FFFFF), (0x80 + 0x8B0, 0), (0x80 + 0x8B4, 0), (0x80 + 0x8B8, 0), (0x80 + 0x8BC, 0)]
    for offset, value in mutations:
        altered = bytearray(raw)
        put(altered, offset, value)
        convert(altered, expected=False)
    for offset in (0x42, 0x4A):
        altered = bytearray(raw)
        altered[offset] = 65
        convert(altered, expected=False)
    for pattern in (LOOP, ARBITRATE, CONNECT):
        altered = bytearray(raw)
        words(altered, 0x80 + 0x1000, pattern)
        convert(altered, expected=False)
    for size in (0, 7, 59, 60, 127, len(raw) - 1):
        convert(raw[:size], expected=False)
    for memory in (0, len(raw) - 1, len(raw), 0x2200):
        convert(raw, memory=memory, expected=False)
    record('malformed_bounds_strings_duplicate_patterns_branches_short_files_and_heap_guard_reject_without_mutation', mutations=len(mutations) + 15)
    rng = random.Random(13409)
    for index in range(20000):
        header = bytearray(raw[:60])
        for ignored in range(rng.randrange(1, 5)):
            put(header, rng.randrange(15) * 4, rng.getrandbits(32))
        library.PluginLegacy_ValidateHeader(c.create_string_buffer(bytes(header)), rng.getrandbits(32), rng.getrandbits(32))
    record('twenty_thousand_actual_C_header_overflow_and_boundary_inputs', count=20000)
    for default in (0, 1):
        shim = NativeShim(converted, 0x2000, 0x180, default)
        shim.call(shim.init_ex)
        assert shim.kernel_calls == 1
        assert [item['svc'] for item in shim.events] == [0x2D, 0x32]
        assert [shim.read(shim.storage + i * 4) for i in range(4)] == [0x31, 0x41, 0x03000000, 0x03000004]
        assert shim.read(0x07000018) == default
        assert shim.read(0x0700001C) == 0x03000004
        record('ARM_init_preserves_kernel_call_captures_modern_event_reply_and_restores_legacy_default_flag', default=default, events=shim.events)
        for event in (0, 1, 2, 3, 4, 0xFFFFFFFF):
            shim.write(0x03000000, event)
            assert shim.call(shim.fetch) == event
        record('ARM_fetch_reads_event_through_original_captured_pointer', default=default)
        for event in (0, 1, 2, 5, 6):
            shim.write(0x03000000, event)
            shim.write(0x03000004, 0x1234)
            before = len(shim.events)
            shim.call(shim.reply, event)
            assert shim.read(0x03000004) == 0
            assert shim.read(0x03000000) == (0 if event <= 2 else event)
            assert len(shim.events) == before + (event > 2)
        record('ARM_sleep_and_home_event_replies_preserve_callee_saved_registers_and_stack', default=default)
        shim.write(0x03000000, 3)
        before = len(shim.events)
        shim.call(shim.reply, 3)
        assert shim.read(0x03000000) == 0
        assert [item['r2'] for item in shim.events[before:]] == [0, 1]
        record('ARM_swap_reply_wait_wakeup_protocol', default=default)
        shim.call(shim.reply, 4, exits=True)
        assert shim.events[-1]['svc'] == 9
        record('ARM_exit_event_acknowledges_and_exits_legacy_thread', default=default)
    if sample:
        sample_data = sample.read_bytes()
        old = c.create_string_buffer(sample_data)
        assert not library.PluginFile_Validate(old, len(sample_data), MEMORY, 0x100, c.byref(Layout()))
        output = convert(sample_data)
        record('external_actual_legacy_sample_rejected_by_modern_layout_then_safely_normalized', input_sha256=hashlib.sha256(sample_data).hexdigest(), input_size=len(sample_data), converted_sha256=hashlib.sha256(output).hexdigest(), converted_size=len(output))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--sample', type=Path)
    parser.add_argument('--output', type=Path)
    options = parser.parse_args()
    compiler = os.environ.get('CC', 'cc')
    if not shutil.which(compiler) and os.name == 'nt':
        compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
    env = os.environ.copy()
    env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
    sources = [MODULE / 'source/plugin' / name for name in ('legacy.c', 'file_format.c', 'payload_vm.c')]
    with tempfile.TemporaryDirectory(prefix='luma-legacy-') as folder:
        library = Path(folder) / ('legacy.dll' if os.name == 'nt' else 'legacy.so')
        subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-shared', '-fPIC', '-I', str(MODULE / 'include'), *map(str, sources), '-o', str(library)], env=env, check=True)
        loaded = c.CDLL(str(library))
        try:
            run(loaded, options.sample)
        finally:
            if os.name == 'nt':
                import _ctypes
                _ctypes.FreeLibrary(loaded._handle)
    report = {'all_passed': True, 'count': len(CASES), 'cases': CASES, 'hardware_tested': False, 'source_sha256': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in sources}, 'boundary': 'Actual C legacy validator/converter/current format and payload VM, real appended ARM init/event functions; OS port/arbiter services are fixtures. No physical SD or game startup claim.'}
    if options.output:
        options.output.parent.mkdir(parents=True, exist_ok=True)
        options.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'all_passed': True, 'count': len(CASES), 'hardware_tested': False}))


if __name__ == '__main__':
    main()
