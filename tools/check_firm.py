import hashlib
import struct
import sys
from pathlib import Path
from firm_modules import bundled_native_modules

root = Path(sys.argv[1])
for path in (root / 'boot.firm', root / '3ds/luma/9.1/boot.firm', root / '3ds/luma/13.4/boot.firm'):
    data = path.read_bytes()
    assert data[:4] == b'FIRM' and len(data) >= 0x200, str(path)
    regions = []
    for index in range(4):
        header = 0x40 + index * 0x30
        offset, address, size, method = struct.unpack_from('<4I', data, header)
        if not size:
            continue
        assert offset >= 0x200 and offset % 0x200 == 0 and offset + size <= len(data), str(path)
        assert method <= 2, str(path)
        assert hashlib.sha256(data[offset:offset + size]).digest() == data[header + 16:header + 48], str(path)
        assert all(offset + size <= a or offset >= b for a, b in regions), str(path)
        regions.append((offset, offset + size))
    assert regions, str(path)
    if '9.1' not in path.parts:
        budget=bundled_native_modules(data)
        print(path, 'bundled NCCH', hex(budget['total']), '/', hex(budget['limit']), 'known-working size gate OK')
    print(path, len(data), 'bytes, section hashes OK')
assert (root / 'boot.firm').read_bytes() == (root / '3ds/luma/13.4/boot.firm').read_bytes()
