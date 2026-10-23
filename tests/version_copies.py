from pathlib import Path
import hashlib
import importlib.util
import json
import struct
import tempfile

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('stage_firmware', root / 'tools/stage_firmware.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
origin = json.loads((root / 'assets/legacy/origin.json').read_bytes())
for name, digest in origin['source_files'].items():
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == digest
assert len(origin['source_files']) == 330
legacy = root / 'assets/legacy/boot-9.1.firm'
assert legacy.stat().st_size == module.LEGACY_SIZE
assert hashlib.sha256(legacy.read_bytes()).hexdigest() == module.LEGACY_SHA256
with tempfile.TemporaryDirectory() as temporary:
    temporary = Path(temporary)
    data = bytearray(1024)
    data[:4] = b'FIRM'
    struct.pack_into('<4I', data, 0x40, 512, 0x20000000, 512, 0)
    data[0x50:0x70] = hashlib.sha256(data[512:]).digest()
    current = temporary / 'current.firm'
    current.write_bytes(data)
    output = temporary / 'SD'
    module.stage(current, output, legacy)
    expected = {'boot.firm', '3ds/luma/9.1/boot.firm', '3ds/luma/13.4/boot.firm', 'SHA256SUMS.txt'}
    assert {p.relative_to(output).as_posix() for p in output.rglob('*') if p.is_file()} == expected
    assert (output / 'boot.firm').read_bytes() == (output / '3ds/luma/13.4/boot.firm').read_bytes() == data
    assert (output / '3ds/luma/9.1/boot.firm').read_bytes() == legacy.read_bytes()
    for line in (output / 'SHA256SUMS.txt').read_text().splitlines():
        digest, name = line.split('  ')
        assert hashlib.sha256((output / name).read_bytes()).hexdigest() == digest
    assert len((output / 'SHA256SUMS.txt').read_text().splitlines()) == 3
    for name, current_data, legacy_data in (
        ('invalid-current', b'BAD!' + data[4:], legacy.read_bytes()),
        ('wrong-legacy', data, bytes(data)),
        ('corrupt-legacy', data, legacy.read_bytes()[:-1] + bytes([legacy.read_bytes()[-1] ^ 1]))):
        current.write_bytes(current_data)
        candidate = temporary / (name + '.firm')
        candidate.write_bytes(legacy_data)
        rejected = temporary / name
        try:
            module.stage(current, rejected, candidate)
        except ValueError:
            pass
        else:
            raise AssertionError(name)
        assert not rejected.exists()
print(json.dumps({'passed': True, 'runtime_firmwares': 3, 'legacy_source_files': 330, 'rejections': 3}))
