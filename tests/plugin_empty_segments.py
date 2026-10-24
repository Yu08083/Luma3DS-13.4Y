import argparse
from contextlib import ExitStack
import ctypes as c
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / 'Luma3DS_v13_4/sysmodules/rosalina'

class Layout(c.Structure):
    _fields_ = [('image', c.c_uint32), ('executable', c.c_uint32), ('snapshot', c.c_uint32)]

class Program(c.Structure):
    _fields_ = [('code', c.c_uint32 * 32), ('count', c.c_uint32)]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--plugin-zip', action='append', type=Path, default=[])
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    compiler = os.environ.get('CC', 'cc')
    if not shutil.which(compiler) and os.name == 'nt':
        compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
    environment = dict(os.environ)
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    sources = [MODULE / ('source/plugin/' + name) for name in ('file_format.c', 'payload_vm.c')]
    cases = []
    with tempfile.TemporaryDirectory(prefix='luma-empty-segments-') as folder, ExitStack() as cleanup:
        library = Path(folder) / ('loader.dll' if os.name == 'nt' else 'loader.so')
        subprocess.run([compiler, '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2', '-shared', '-fPIC',
                        '-I', str(MODULE / 'include'), *map(str, sources), '-o', str(library)], env=environment, check=True)
        vm = c.CDLL(str(library))
        if os.name == 'nt':
            import _ctypes
            cleanup.callback(_ctypes.FreeLibrary, vm._handle)
        vm.PluginFile_Validate.argtypes = [c.c_void_p, c.c_uint32, c.c_uint32, c.c_uint32, c.POINTER(Layout)]
        vm.PluginFile_Validate.restype = c.c_bool
        vm.PluginFile_ValidateStrings.argtypes = [c.c_void_p, c.c_uint32]
        vm.PluginFile_ValidateStrings.restype = c.c_bool
        vm.PluginPayload_Set.argtypes = [c.POINTER(Program), c.POINTER(c.c_uint32)]
        vm.PluginPayload_Set.restype = c.c_bool
        vm.PluginPayload_Run.argtypes = [c.POINTER(Program), c.c_void_p, c.c_uint32, c.POINTER(c.c_uint32), c.POINTER(c.c_uint32)]
        vm.PluginPayload_Run.restype = c.c_bool
        def validate(name, raw, expected, payloads=False):
            buffer = c.create_string_buffer(raw)
            layout = Layout()
            accepted = vm.PluginFile_Validate(buffer, len(raw), 5 * 1024 * 1024, 0x100, c.byref(layout))
            accepted = accepted and vm.PluginFile_ValidateStrings(buffer, len(raw))
            assert accepted == expected, name
            assert buffer.raw[:len(raw)] == raw
            if accepted:
                word = lambda offset: struct.unpack_from('<I', raw, offset)[0]
                end = word(0x58) + layout.image
                assert end <= len(raw) and end <= layout.snapshot
                if payloads:
                    original = raw[word(0x58):end]
                    for field, argument_offset in ((0x74, 0x38), (0x78, 0x48), (0x7C, 0x48)):
                        if not word(0x30) & (1 if field == 0x74 else 2):
                            continue
                        offset = word(field)
                        encoded = (c.c_uint32 * 32)()
                        code = raw[offset:min(offset + 128, word(0x58))]
                        c.memmove(encoded, code, len(code))
                        program = Program()
                        assert vm.PluginPayload_Set(c.byref(program), encoded), (name, field, 'program')
                        image = c.create_string_buffer(original, len(original))
                        arguments = (c.c_uint32 * 4)(*struct.unpack_from('<4I', raw, argument_offset))
                        checksum = c.c_uint32()
                        assert vm.PluginPayload_Run(c.byref(program), image, len(original), arguments, c.byref(checksum))
                        assert checksum.value == word(0x34) and image.raw == original
            cases.append(dict(name=name, passed=True, expected_accepted=expected,
                              sha256=hashlib.sha256(raw).hexdigest(), bytes=len(raw)))
        for ro_size, data_size in ((0, 0), (0, 0x20), (0x20, 0)):
            for ro_zero in (False, True):
                for data_zero in (False, True):
                    raw = bytearray(0x140 + ro_size + data_size)
                    struct.pack_into('<QII', raw, 0, 0x3230303024584733, 0x80000, 0)
                    struct.pack_into('<10I', raw, 0x58, 0x100, 0 if ro_zero else 0x140,
                                     0 if data_zero else 0x140 + ro_size, 0x40, ro_size, data_size, 0, 0, 0, 0)
                    expected = not (ro_size and ro_zero or data_size and data_zero)
                    validate(f'empty_offsets_{ro_size}_{data_size}_{int(ro_zero)}_{int(data_zero)}', bytes(raw), expected)
                    if expected:
                        validate(f'truncated_{ro_size}_{data_size}_{int(ro_zero)}_{int(data_zero)}', bytes(raw[:-1]), False)
        for archive_path in args.plugin_zip:
            with zipfile.ZipFile(archive_path) as archive:
                assert archive.testzip() is None
                entries = [entry for entry in archive.infolist() if entry.filename.lower().endswith('.3gx')]
                assert entries
                for entry in entries:
                    validate(entry.filename, archive.read(entry), True, True)
    report = dict(all_passed=True, count=len(cases), cases=cases, hardware_tested=False,
                  source_sha256={p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
                  boundary='Actual C bounds/string validation and embedded load/save payload VM; no physical 3DS or complete plugin startup.')
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
    print(json.dumps(dict(all_passed=True, count=len(cases), hardware_tested=False)))

if __name__ == '__main__':
    main()
