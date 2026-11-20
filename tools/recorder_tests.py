import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import tarfile
import tempfile
import urllib.request

import recorder


def rejected(function, *args):
    try:
        function(*args)
    except (ValueError, UnicodeDecodeError, struct.error):
        return
    raise AssertionError('Malformed input was accepted')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--reference', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    manifest = json.loads(recorder.MANIFEST.read_text(encoding='utf-8'))
    reference = manifest['upstream_reference']
    if args.reference:
        app = args.reference.read_bytes()
    else:
        with urllib.request.urlopen(reference['url'], timeout=90) as response:
            app = response.read(reference['bytes'] + 1)
    assert len(app) == reference['bytes'] and recorder.digest(app) == reference['sha256']
    embedded = recorder.romfs(app)
    assert set(embedded) == {'rec3ds.cxi', 'rec3ds-launch.3gx', 'web/index.html.gz',
                             'font/inter_500.bcfnt', 'font/inter_700.bcfnt'}
    module = embedded['rec3ds.cxi']
    assert len(module) == reference['embedded_module_bytes']
    assert recorder.digest(module) == reference['embedded_module_sha256']
    info = recorder.ncch(module)
    assert info['title_id'] == '0004013000F1D302' and info['entries']['.code']['bytes'] > 100000
    cases = 3
    for length in (0, 43, 511, 0x9FF, len(module) - 1):
        rejected(recorder.ncch, module[:length])
        cases += 1
    for offset in (0x100, 0x104, 0x118, 0x160, 0x180, 0x18F, 0x1A0, 0x1A4, 0x1C0, 0x200, 0xA00, len(module) - 513):
        altered = bytearray(module)
        altered[offset] ^= 0x7F
        rejected(recorder.ncch, altered)
        cases += 1
    for length in (0, 43, 511, len(app) - 1):
        rejected(recorder.romfs, app[:length])
        cases += 1
    start = recorder.read32(app, 0x28)
    for offset, value in ((0, 0), (4, 0), (0x28, 0), (0x28, 0xFFFFFFFF), (start, 0),
                          (start + 4, 0xFFFFFFF0), (start + 8, 0xFFFFFFFF),
                          (start + 28, 0xFFFFFFF0), (start + 32, 0xFFFFFFFF),
                          (start + 36, 0xFFFFFFFF)):
        altered = bytearray(app)
        struct.pack_into('<I', altered, offset, value)
        rejected(recorder.romfs, altered)
        cases += 1
    with tempfile.TemporaryDirectory(prefix='recorder-package-') as temporary:
        base = Path(temporary)
        runtime, firmware = base / 'runtime', base / 'firmware'
        runtime.mkdir()
        firmware.mkdir()
        original_firmware = b'firmware sentinel'
        (firmware / 'boot.firm').write_bytes(original_firmware)
        (firmware / 'SHA256SUMS.txt').write_text('old checksums', encoding='ascii')
        files = {manifest['runtime']['module']: module, manifest['runtime']['launcher']: embedded['rec3ds-launch.3gx'],
                 manifest['runtime']['companion']: app}
        for name, data in files.items():
            target = runtime / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        for name in recorder.LICENSE_NAMES:
            target = runtime / manifest['runtime']['licenses'] / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text('license fixture', encoding='ascii')
        merged = base / 'merged'
        result = recorder.merge_runtime(firmware, runtime, merged)
        assert result['all_passed'] and (merged / 'boot.firm').read_bytes() == original_firmware
        for name, data in files.items():
            assert (merged / name).read_bytes() == data
        sums = (merged / 'SHA256SUMS.txt').read_text(encoding='ascii').splitlines()
        assert len(sums) == 4 + len(recorder.LICENSE_NAMES)
        for line in sums:
            expected, name = line.split('  ', 1)
            assert recorder.digest((merged / name).read_bytes()) == expected
        assert (firmware / 'SHA256SUMS.txt').read_text(encoding='ascii') == 'old checksums'
        cases += 7
        sentinel = merged / 'keep.txt'
        sentinel.write_text('keep', encoding='ascii')
        rejected(recorder.merge_runtime, firmware, runtime, merged)
        assert sentinel.read_text(encoding='ascii') == 'keep'
        cases += 1
        (runtime / 'boot.firm').write_bytes(b'conflicting firmware')
        conflict = base / 'conflict'
        rejected(recorder.merge_runtime, firmware, runtime, conflict)
        assert not conflict.exists()
        cases += 1
        (runtime / 'boot.firm').unlink()
        collision = firmware / manifest['runtime']['module']
        collision.parent.mkdir(parents=True, exist_ok=True)
        collision.write_bytes(b'other recorder')
        rejected(recorder.merge_runtime, firmware, runtime, conflict)
        assert not conflict.exists() and collision.read_bytes() == b'other recorder'
        cases += 1
        for index, name in enumerate(('root/../../outside', '/root/outside', 'root/a:b', 'root/a\\b')):
            packed = io.BytesIO()
            with tarfile.open(fileobj=packed, mode='w:gz') as archive:
                item = tarfile.TarInfo(name)
                item.size = 4
                archive.addfile(item, io.BytesIO(b'bad!'))
            raw = packed.getvalue()
            cache = base / ('cache' + str(index))
            cache.mkdir()
            sha = hashlib.sha256(raw).hexdigest()
            (cache / (sha + '.tar.gz')).write_bytes(raw)
            rejected(recorder.fetch_source, {'url': 'https://invalid.example/', 'sha256': sha}, base / ('bad' + str(index)), cache)
            assert not (base / 'outside').exists()
            cases += 1
        packed = io.BytesIO()
        with tarfile.open(fileobj=packed, mode='w:gz') as archive:
            item = tarfile.TarInfo('root/link')
            item.type = tarfile.SYMTYPE
            item.linkname = '../../outside'
            archive.addfile(item)
        raw = packed.getvalue()
        cache = base / 'symlink-cache'
        cache.mkdir()
        sha = recorder.digest(raw)
        (cache / (sha + '.tar.gz')).write_bytes(raw)
        rejected(recorder.fetch_source, {'url': 'https://invalid.example/', 'sha256': sha}, base / 'bad-link', cache)
        cases += 1
    result = {'all_passed': True, 'checks': cases, 'upstream_3dsx': {'bytes': len(app), 'sha256': recorder.digest(app)},
              'upstream_cxi': {'bytes': len(module), 'sha256': recorder.digest(module)},
              'scope': 'Native binary parsing, corrupt input rejection, safe archive extraction and conflict-preserving staging',
              'physical_console_tested': False}
    if args.output:
        recorder.record(args.output, result)
    print(json.dumps(result))


if __name__ == '__main__':
    main()
