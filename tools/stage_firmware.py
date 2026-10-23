import hashlib
from pathlib import Path
import struct
import sys

LEGACY_SIZE = 242176
LEGACY_SHA256 = '76eb1f3b226cea65d643493a574b1427ad1e0da33f8fe9deca7a7d6caf025116'


def check_firm(data):
    if len(data) < 512 or data[:4] != b'FIRM':
        raise ValueError('Invalid FIRM header')
    regions = []
    for index in range(4):
        header = 0x40 + index * 0x30
        offset, address, size, method = struct.unpack_from('<4I', data, header)
        if not size:
            continue
        if offset < 512 or offset % 512 or offset + size > len(data) or method > 2:
            raise ValueError('Invalid FIRM section')
        if any(offset < end and offset + size > start for start, end in regions):
            raise ValueError('Overlapping FIRM sections')
        if hashlib.sha256(data[offset:offset + size]).digest() != data[header + 16:header + 48]:
            raise ValueError('Invalid FIRM section hash')
        regions.append((offset, offset + size))
    if not regions:
        raise ValueError('Empty FIRM')


def stage(current, destination, legacy):
    current_data = current.read_bytes()
    legacy_data = legacy.read_bytes()
    check_firm(current_data)
    check_firm(legacy_data)
    if len(legacy_data) != LEGACY_SIZE or hashlib.sha256(legacy_data).hexdigest() != LEGACY_SHA256:
        raise ValueError('Unexpected v9.1 FIRM')
    files = {'boot.firm': current_data, '3ds/luma/13.4/boot.firm': current_data,
             '3ds/luma/9.1/boot.firm': legacy_data}
    for name, data in files.items():
        path = destination / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    sums = ''.join(hashlib.sha256(data).hexdigest() + '  ' + name + '\n' for name, data in sorted(files.items()))
    (destination / 'SHA256SUMS.txt').write_text(sums, encoding='ascii', newline='\n')


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    stage(Path(sys.argv[1]), Path(sys.argv[2]), root / 'assets/legacy/boot-9.1.firm')
