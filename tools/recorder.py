import argparse
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import struct
import subprocess
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'third_party/recorder/manifest.json'
TITLE_ID = 0x0004013000F1D302


def digest(data):
    return hashlib.sha256(data).hexdigest()


def receipt(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': digest(data)}


def record(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def run(args, cwd=None, env=None):
    subprocess.run([str(value) for value in args], cwd=cwd, env=env, check=True)


def empty_directory(path):
    if path.exists() and (not path.is_dir() or any(path.iterdir())):
        raise ValueError('Destination is not empty: ' + str(path))
    path.mkdir(parents=True, exist_ok=True)


def fetch_source(spec, destination, cache):
    empty_directory(destination)
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / (spec['sha256'] + '.tar.gz')
    if archive.exists():
        data = archive.read_bytes()
    else:
        request = urllib.request.Request(spec['url'], headers={'User-Agent': 'Yu-Luma-recorder-build'})
        with urllib.request.urlopen(request, timeout=90) as response:
            data = response.read(32 * 1024 * 1024 + 1)
        if digest(data) != spec['sha256']:
            raise ValueError('Unexpected source archive: ' + spec['url'])
        archive.write_bytes(data)
    if digest(data) != spec['sha256']:
        raise ValueError('Unexpected cached source archive: ' + str(archive))
    with tarfile.open(fileobj=io.BytesIO(data), mode='r:gz') as source:
        members = source.getmembers()
        prefixes = {PurePosixPath(member.name).parts[0] for member in members}
        if len(prefixes) != 1:
            raise ValueError('Expected one archive root')
        total = 0
        names = set()
        for member in members:
            parts = PurePosixPath(member.name).parts
            if not parts or parts[0] in ('/', '..') or any(value in ('..', '') or ':' in value or '\\' in value for value in parts):
                raise ValueError('Unsafe source archive path')
            if not member.isfile() and not member.isdir():
                raise ValueError('Unsupported source archive entry')
            relative = PurePosixPath(*parts[1:])
            if not parts[1:]:
                continue
            if str(relative) in names:
                raise ValueError('Duplicate source archive entry')
            names.add(str(relative))
            target = destination.joinpath(*relative.parts)
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
                continue
            total += member.size
            if total > 128 * 1024 * 1024:
                raise ValueError('Source archive is too large')
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(source.extractfile(member).read())
            target.chmod(member.mode & 0o777)


def source_files(path):
    return {str(file.relative_to(path)).replace('\\', '/'): receipt(file)
            for file in sorted(path.rglob('*')) if file.is_file()}


def prepare(work):
    config = json.loads(MANIFEST.read_text(encoding='utf-8'))
    empty_directory(work)
    source = work / 'source'
    fetch_source(config['sources']['recorder'], source, work / 'cache')
    before = source_files(source)
    patches = []
    for item in config['patches']:
        patch = ROOT / item['path']
        if digest(patch.read_bytes()) != item['sha256']:
            raise ValueError('Unexpected recorder patch: ' + str(patch))
        run(['git', 'apply', '--check', patch], cwd=source)
        run(['git', 'apply', patch], cwd=source)
        patches.append({'path': item['path'], **receipt(patch)})
    after = source_files(source)
    result = {'schema': 1, 'source': config['sources']['recorder'], 'manifest': receipt(MANIFEST),
              'patches': patches, 'before': before, 'after': after,
              'changed': sorted(name for name in before.keys() | after.keys() if before.get(name) != after.get(name))}
    record(work / 'prepared.json', result)
    return result


def build_tools(work, jobs):
    config = json.loads(MANIFEST.read_text(encoding='utf-8'))
    tool_root = work / 'tools'
    empty_directory(tool_root)
    directories = {'libctru': tool_root / 'libctru', 'makerom': tool_root / 'makerom',
                   '3gxtool': tool_root / '3gxtool', 'yaml-cpp': tool_root / '3gxtool/extern/yaml-cpp',
                   'dynalo': tool_root / '3gxtool/extern/dynalo'}
    for name, path in directories.items():
        fetch_source(config['sources'][name], path, work / 'cache')
    environment = dict(os.environ)
    compiler = Path(environment['DEVKITARM']) / 'bin/arm-none-eabi-gcc'
    environment['PATH'] = str(compiler.parent) + os.pathsep + environment['PATH']
    ctru = directories['libctru'] / 'libctru'
    makefile = ctru / 'Makefile'
    makefile.write_bytes(makefile.read_bytes().replace(b'-Werror', b''))
    run(['make', '-j' + str(jobs)], cwd=ctru, env=environment)
    environment['CTRULIB'] = str(ctru)
    makerom = directories['makerom'] / 'makerom'
    run(['make', 'deps', '-j' + str(jobs)], cwd=makerom, env=environment)
    run(['make', 'program', '-j' + str(jobs)], cwd=makerom, env=environment)
    gx = directories['3gxtool']
    run(['cmake', '-S', gx, '-B', gx / 'build', '-DCMAKE_BUILD_TYPE=Release', '-DGIT_SUBMODULE=OFF'], env=environment)
    run(['cmake', '--build', gx / 'build', '-j' + str(jobs)], env=environment)
    environment['PATH'] = str(makerom / 'bin') + os.pathsep + str(gx / 'build') + os.pathsep + environment['PATH']
    versions = {}
    for name, args in {'arm_gcc': [compiler, '--version'], 'host_gcc': ['gcc', '--version'],
                       'make': ['make', '--version'], 'cmake': ['cmake', '--version']}.items():
        versions[name] = subprocess.check_output([str(arg) for arg in args], env=environment, text=True).splitlines()[0]
    record(work / 'toolchain.json', {'image': config['image'], 'sources': config['sources'], 'versions': versions,
                                    'libctru_makefile': receipt(makefile), 'makerom': receipt(makerom / 'bin/makerom'),
                                    '3gxtool': receipt(gx / 'build/3gxtool')})
    return environment


def check_prepared(work):
    result = json.loads((work / 'prepared.json').read_text(encoding='utf-8'))
    if result['manifest'] != receipt(MANIFEST) or result['after'] != source_files(work / 'source'):
        raise ValueError('Prepared recorder source changed')
    for item in result['patches']:
        if receipt(ROOT / item['path']) != {key: item[key] for key in ('bytes', 'sha256')}:
            raise ValueError('Prepared recorder patch changed')
    return result


def host_tests(work, compiler=None):
    tests = work / 'source/tests/host'
    environment = dict(os.environ)
    if compiler:
        environment['CC'] = compiler
    run(['make', 'test', '-j2'], cwd=tests, env=environment)
    binaries = {name: receipt(tests / name) for name in ('test_bench', 'test_convert', 'test_codec',
                'test_audio', 'test_huff', 'test_fused', 'test_unwrap', 'test_autostart', 'test_front',
                'test_plgring', 'test_appletguard', 'test_join')}
    record(work / 'host-tests.json', {'all_passed': True, 'suites': 12, 'binaries': binaries})


def read32(data, offset):
    if offset < 0 or offset + 4 > len(data):
        raise ValueError('Truncated binary')
    return struct.unpack_from('<I', data, offset)[0]


def ncch(data):
    if len(data) < 0xA00 or data[0x100:0x104] != b'NCCH' or read32(data, 0x104) * 512 != len(data):
        raise ValueError('Invalid recorder NCCH header')
    if struct.unpack_from('<Q', data, 0x118)[0] != TITLE_ID or data[0x200:0x208] != b'rec3ds\0\0':
        raise ValueError('Unexpected recorder title')
    if read32(data, 0x180) != 0x400 or digest(data[0x200:0x600]) != data[0x160:0x180].hex():
        raise ValueError('Invalid recorder extended header hash')
    if not data[0x18F] & 4:
        raise ValueError('Encrypted recorder is unsupported')
    exefs_offset, exefs_size = read32(data, 0x1A0) * 512, read32(data, 0x1A4) * 512
    if exefs_offset < 0xA00 or exefs_size < 512 or exefs_offset + exefs_size > len(data):
        raise ValueError('Invalid recorder ExeFS bounds')
    if digest(data[exefs_offset:exefs_offset + 512]) != data[0x1C0:0x1E0].hex():
        raise ValueError('Invalid recorder ExeFS header hash')
    entries = {}
    for index in range(10):
        position = exefs_offset + index * 16
        if not data[position]:
            continue
        name = data[position:position + 8].split(b'\0', 1)[0].decode('ascii')
        offset, size = read32(data, position + 8), read32(data, position + 12)
        if name in entries or offset + size > exefs_size - 512:
            raise ValueError('Invalid recorder ExeFS entry')
        content = data[exefs_offset + 512 + offset:exefs_offset + 512 + offset + size]
        expected = data[exefs_offset + 512 - 32 * (index + 1):exefs_offset + 512 - 32 * index]
        if hashlib.sha256(content).digest() != expected:
            raise ValueError('Invalid recorder ExeFS entry hash')
        entries[name] = {'bytes': size, 'sha256': digest(content)}
    if '.code' not in entries:
        raise ValueError('Recorder executable is missing')
    return {'title_id': f'{TITLE_ID:016X}', 'name': 'rec3ds', 'entries': entries,
            'text_bytes': read32(data, 0x218), 'ro_bytes': read32(data, 0x228),
            'data_bytes': read32(data, 0x238), 'bss_bytes': read32(data, 0x23C),
            'signature_verified': False}


def romfs(data):
    if len(data) < 44 or data[:4] != b'3DSX' or struct.unpack_from('<H', data, 4)[0] != 44:
        raise ValueError('Invalid recorder 3DSX header')
    start = read32(data, 0x28)
    if start < 44 or start + 40 > len(data):
        raise ValueError('Invalid recorder RomFS offset')
    fs = data[start:]
    header = struct.unpack_from('<10I', fs)
    if header[0] != 40:
        raise ValueError('Invalid recorder RomFS header')
    for offset, size in ((header[1], header[2]), (header[3], header[4]), (header[5], header[6]), (header[7], header[8])):
        if offset < 40 or offset + size > len(fs):
            raise ValueError('Invalid recorder RomFS metadata')
    if header[9] < max(header[index] + header[index + 1] for index in (1, 3, 5, 7)) or header[9] > len(fs):
        raise ValueError('Invalid recorder RomFS file data')
    directories = {}
    position = 0
    while position < header[4]:
        offset = header[3] + position
        if position + 24 > header[4]:
            raise ValueError('Truncated recorder RomFS directory')
        parent = read32(fs, offset)
        length = read32(fs, offset + 20)
        if length & 1 or position + 24 + length > header[4]:
            raise ValueError('Invalid recorder RomFS directory name')
        name = fs[offset + 24:offset + 24 + length].decode('utf-16le')
        directories[position] = (parent, name)
        position = (position + 24 + length + 3) & ~3
    def directory(offset):
        result, visited = [], set()
        while offset:
            if offset in visited or offset not in directories:
                raise ValueError('Invalid recorder RomFS directory graph')
            visited.add(offset)
            offset, name = directories[offset]
            result.append(name)
        return '/'.join(reversed(result))
    files = {}
    position = 0
    while position < header[8]:
        offset = header[7] + position
        if position + 32 > header[8]:
            raise ValueError('Truncated recorder RomFS file')
        parent = read32(fs, offset)
        data_offset, size = struct.unpack_from('<QQ', fs, offset + 8)
        length = read32(fs, offset + 28)
        if length & 1 or position + 32 + length > header[8] or header[9] + data_offset + size > len(fs):
            raise ValueError('Invalid recorder RomFS file bounds')
        name = fs[offset + 32:offset + 32 + length].decode('utf-16le')
        full = '/'.join(value for value in (directory(parent), name) if value)
        parts = PurePosixPath(full).parts
        if not name or full in files or full.startswith('/') or any(part in ('.', '..') or '\\' in part for part in parts):
            raise ValueError('Invalid recorder RomFS filename')
        files[full] = fs[header[9] + data_offset:header[9] + data_offset + size]
        position = (position + 32 + length + 3) & ~3
    return files


def licenses(source):
    files = {'3DS-Recorder.txt': source / 'LICENSE', 'minih264.txt': source / 'third_party/minih264/LICENSE',
             'vo-aacenc.txt': source / 'third_party/vo-aacenc/COPYING',
             'vo-aacenc-NOTICE.txt': source / 'third_party/vo-aacenc/NOTICE',
             'vo-aacenc-common-NOTICE.txt': source / 'third_party/vo-aacenc/common/NOTICE',
             'vo-aacenc-aacenc-NOTICE.txt': source / 'third_party/vo-aacenc/aacenc/NOTICE',
             'mp4-muxer.txt': source / 'web/vendor/mp4-muxer.LICENSE',
             'Inter-OFL.txt': source / 'app/assets/fonts/OFL.txt'}
    result = {name: path.read_bytes() for name, path in files.items()}
    for name, path in [('qrcodegen.txt', source / 'third_party/qrcodegen/qrcodegen.c')]:
        data = path.read_bytes()
        end = data.find(b'*/')
        if end < 0:
            raise ValueError('License block missing: ' + str(path))
        result[name] = data[:end + 2] + b'\n'
    data = (source / 'app/source/csvc.s').read_bytes()
    marker = data.find(b'\n.arm\n')
    if marker < 0 or b'This notice may not be removed' not in data[:marker]:
        raise ValueError('Luma3DS assembly license missing')
    result['Luma3DS-csvc.txt'] = data[:marker] + b'\n'
    return result


def package(work):
    config = json.loads(MANIFEST.read_text(encoding='utf-8'))
    source = work / 'source'
    cxi = (source / 'sysmod/rec3ds.cxi').read_bytes()
    helper = (source / 'plugin/rec3ds-launch.3gx').read_bytes()
    app = (source / 'app/3ds-recorder.3dsx').read_bytes()
    cxi_info = ncch(cxi)
    embedded = romfs(app)
    if embedded.get('rec3ds.cxi') != cxi or embedded.get('rec3ds-launch.3gx') != helper:
        raise ValueError('Recorder companion has different embedded module or launcher')
    if helper[:8] != b'3GX$0002' or read32(helper, 0x58) < 0x94:
        raise ValueError('Invalid recorder launcher header')
    runtime = work / 'runtime'
    empty_directory(runtime)
    files = {config['runtime']['module']: cxi, config['runtime']['launcher']: helper,
             config['runtime']['companion']: app}
    files.update({config['runtime']['licenses'] + '/' + name: data for name, data in licenses(source).items()})
    for name, data in files.items():
        target = runtime.joinpath(*PurePosixPath(name).parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    result = {'schema': 1, 'all_passed': True, 'upstream': config['sources']['recorder'],
              'prepared': receipt(work / 'prepared.json'), 'toolchain': receipt(work / 'toolchain.json'),
              'host_tests': receipt(work / 'host-tests.json'), 'files': source_files(runtime),
              'module': cxi_info, 'companion_files': {name: {'bytes': len(data), 'sha256': digest(data)} for name, data in embedded.items()},
              'physical_console_tested': False}
    record(work / 'runtime.json', result)
    diagnostic = work / 'diagnostics'
    empty_directory(diagnostic)
    for relative in ['sysmod/rec3ds.elf', 'sysmod/rec3ds.map', 'plugin/build/rec3ds-launch.elf',
                     'app/3ds-recorder.elf', 'app/3ds-recorder.map']:
        item = source / relative
        if item.is_file():
            target = diagnostic / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(item, target)
    for name in ('prepared.json', 'toolchain.json', 'host-tests.json', 'runtime.json'):
        shutil.copyfile(work / name, diagnostic / name)
    return result


def build(work, jobs):
    check_prepared(work)
    host_tests(work)
    environment = build_tools(work, jobs)
    for part in ('sysmod', 'plugin', 'app'):
        run(['make', '-j' + str(jobs)], cwd=work / 'source' / part, env=environment)
    return package(work)


def merge_runtime(firmware, runtime, destination):
    for source in (firmware, runtime):
        if not source.is_dir() or source.resolve() == destination.resolve() or source.resolve() in destination.resolve().parents:
            raise ValueError('Use a separate staging destination')
    files = {}
    for source in (firmware, runtime):
        for path in sorted(source.rglob('*')):
            if path.is_symlink():
                raise ValueError('Symlinks are not runtime files')
            if not path.is_file():
                continue
            name = path.relative_to(source).as_posix()
            if name == 'SHA256SUMS.txt':
                continue
            data = path.read_bytes()
            if name in files and data != files[name]:
                raise ValueError('Conflicting runtime file: ' + name)
            files[name] = data
    config = json.loads(MANIFEST.read_text(encoding='utf-8'))
    if not all(name in files for key, name in config['runtime'].items() if key != 'licenses'):
        raise ValueError('Recorder runtime is incomplete')
    ncch(files[config['runtime']['module']])
    embedded = romfs(files[config['runtime']['companion']])
    if embedded.get('rec3ds.cxi') != files[config['runtime']['module']] or embedded.get('rec3ds-launch.3gx') != files[config['runtime']['launcher']]:
        raise ValueError('Recorder runtime does not match the companion')
    empty_directory(destination)
    for name, data in files.items():
        target = destination.joinpath(*PurePosixPath(name).parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    sums = ''.join(digest(data) + '  ' + name + '\n' for name, data in sorted(files.items()))
    (destination / 'SHA256SUMS.txt').write_text(sums, encoding='ascii', newline='\n')
    return {'schema': 1, 'all_passed': True, 'files': source_files(destination)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('command', choices=('prepare', 'build', 'package', 'host-tests', 'merge'))
    parser.add_argument('--work', type=Path)
    parser.add_argument('--firmware', type=Path)
    parser.add_argument('--runtime', type=Path)
    parser.add_argument('--destination', type=Path)
    parser.add_argument('--jobs', type=int, default=2)
    args = parser.parse_args()
    if not 1 <= args.jobs <= 2:
        raise ValueError('Use one or two build jobs')
    if args.command != 'merge' and not args.work:
        parser.error('--work is required')
    if args.command == 'merge' and not all((args.firmware, args.runtime, args.destination)):
        parser.error('--firmware, --runtime and --destination are required')
    functions = {'prepare': lambda: prepare(args.work), 'build': lambda: build(args.work, args.jobs),
                 'package': lambda: package(args.work), 'host-tests': lambda: host_tests(args.work),
                 'merge': lambda: merge_runtime(args.firmware, args.runtime, args.destination)}
    result = functions[args.command]()
    if result:
        print(json.dumps({key: result[key] for key in ('schema', 'all_passed', 'changed') if key in result}))


if __name__ == '__main__':
    main()
