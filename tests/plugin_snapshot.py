import argparse
import ctypes as c
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / 'Luma3DS_v13_4/sysmodules/rosalina'


class Layout(c.Structure):
    _fields_ = [('image', c.c_uint32), ('executable', c.c_uint32), ('snapshot', c.c_uint32)]


def run(library):
    library.PluginFile_Validate.argtypes = [c.c_void_p, c.c_uint32, c.c_uint32, c.c_uint32, c.POINTER(Layout)]
    library.PluginFile_Validate.restype = c.c_bool
    data = bytearray(0x240)
    struct.pack_into('<QII', data, 0, 0x3230303024584733, 0x80000, 0)
    struct.pack_into('<I', data, 0x30, 3)
    struct.pack_into('<10I', data, 0x58, 0x100, 0x200, 0x220, 0x100, 0x20, 0x20, 0x400, 0x94, 0x94, 0x94)
    struct.pack_into('<3I', data, 0x94, 0xE3A00000, 0xE12FFF1E, 0xE320F000)
    checks = []

    def verify(name, fixture, file_size, accepted, expected=0x240, memory=5 * 1024 * 1024):
        layout = Layout()
        buffer = c.create_string_buffer(bytes(fixture))
        before = buffer.raw
        result = library.PluginFile_Validate(buffer, file_size, memory, 0x100, c.byref(layout))
        assert result == accepted, name
        assert buffer.raw == before
        if accepted:
            assert layout.snapshot == expected and layout.snapshot <= memory
            assert layout.executable < memory
        checks.append({'name': name, 'passed': True, 'file_size': file_size, 'snapshot_size': layout.snapshot if result else None})

    for size in (len(data), len(data) + 1, 1024 * 1024, 6 * 1024 * 1024, 64 * 1024 * 1024, 0xFFFFFFFF):
        verify('unused_trailer_does_not_increase_snapshot_' + str(size), data, size, True)
    verify('short_executable_still_rejected', data, len(data) - 1, False)
    for field in (0x10, 0x18, 0x20, 0x28):
        altered = bytearray(data)
        struct.pack_into('<2I', altered, field, 16, 0x2F1)
        verify('required_metadata_included_' + hex(field), altered, 0x400, True, 0x310)
        struct.pack_into('<2I', altered, field, 16, 6 * 1024 * 1024)
        verify('required_metadata_exceeding_memory_rejected_' + hex(field), altered, 7 * 1024 * 1024, False)
        struct.pack_into('<2I', altered, field, 16, 0xFFFFFFF1)
        verify('required_metadata_overflow_rejected_' + hex(field), altered, 0xFFFFFFFF, False)
    altered = bytearray(data)
    struct.pack_into('<2I', altered, 0x80, 128, 0x300)
    verify('all_128_target_ids_included', altered, 0x1000, True, 0x500)
    struct.pack_into('<2I', altered, 0x80, 128, 6 * 1024 * 1024)
    verify('target_ids_exceeding_memory_rejected', altered, 7 * 1024 * 1024, False)
    altered = bytearray(data)
    struct.pack_into('<I', altered, 0x70, 0xFFFFFFFF)
    verify('bss_overflow_rejected_even_with_large_trailer', altered, 64 * 1024 * 1024, False)
    altered = bytearray(data)
    struct.pack_into('<I', altered, 0x64, 6 * 1024 * 1024)
    verify('oversize_executable_not_confused_with_unused_trailer', altered, 7 * 1024 * 1024, False)
    return checks


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path)
    options = parser.parse_args()
    compiler = os.environ.get('CC', 'cc')
    if not shutil.which(compiler) and os.name == 'nt':
        compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
    env = os.environ.copy()
    env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
    source = MODULE / 'source/plugin/file_format.c'
    with tempfile.TemporaryDirectory(prefix='luma-snapshot-') as folder:
        library = Path(folder) / ('snapshot.dll' if os.name == 'nt' else 'snapshot.so')
        subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-shared', '-fPIC', '-I', str(MODULE / 'include'), str(source), '-o', str(library)], env=env, check=True)
        loaded = c.CDLL(str(library))
        try:
            checks = run(loaded)
        finally:
            if os.name == 'nt':
                import _ctypes
                _ctypes.FreeLibrary(loaded._handle)
    report = {'all_passed': True, 'count': len(checks), 'cases': checks, 'hardware_tested': False, 'source_sha256': {str(source.relative_to(ROOT)): hashlib.sha256(source.read_bytes()).hexdigest()}, 'boundary': 'Actual C header bounds and required snapshot extent; large file lengths are declared fixtures, no physical SD or full file read.'}
    if options.output:
        options.output.parent.mkdir(parents=True, exist_ok=True)
        options.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
    print(json.dumps({'all_passed': True, 'count': len(checks), 'hardware_tested': False}))


if __name__ == '__main__':
    main()
