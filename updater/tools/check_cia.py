from pathlib import Path
import argparse
import hashlib
import json
import re
import struct
import sys


class InvalidCia(ValueError):
    pass


def require(value, message):
    if not value:
        raise InvalidCia(message)


def part(data, offset, size):
    require(offset >= 0 and size >= 0 and offset <= len(data) and size <= len(data) - offset,
            'Section is outside its container')
    return data[offset:offset + size]


def le32(data, offset):
    return int.from_bytes(part(data, offset, 4), 'little')


def le64(data, offset):
    return int.from_bytes(part(data, offset, 8), 'little')


def be16(data, offset):
    return int.from_bytes(part(data, offset, 2), 'big')


def be64(data, offset):
    return int.from_bytes(part(data, offset, 8), 'big')


def digest(data):
    return hashlib.sha256(data).digest()


def align64(value):
    return (value + 63) & ~63


def signed_body(data):
    kind = int.from_bytes(part(data, 0, 4), 'big')
    sizes = {0x10000: 0x240, 0x10001: 0x140, 0x10002: 0x80,
             0x10003: 0x240, 0x10004: 0x140, 0x10005: 0x80}
    require(kind in sizes, 'Unknown signature block type')
    part(data, 0, sizes[kind])
    return sizes[kind]


def rsf_section(text, name):
    match = re.search(r'^' + re.escape(name) + r':\s*\n((?:[ \t]+[^\n]*\n|[ \t]*\n)*)', text, re.M)
    require(match is not None, 'Missing RSF section: ' + name)
    return match.group(1)


def rsf_number(text, name):
    values = re.findall(r'^  ' + re.escape(name) + r':\s*([^\s#]+)', text, re.M)
    require(len(values) == 1, 'Missing or repeated RSF field: ' + name)
    value = values[0].strip('"\'')
    require(re.fullmatch(r'0[xX][0-9a-fA-F]+|[0-9]+', value) is not None,
            'Invalid RSF number: ' + name)
    return int(value, 16 if value.lower().startswith('0x') else 10)


def configuration(path):
    text = path.read_text(encoding='utf-8')
    title = rsf_section(text, 'TitleInfo')
    require(re.search(r'^  Category:\s*Application\s*$', title, re.M), 'Application category is required')
    unique = rsf_number(title, 'UniqueId')
    variation = rsf_number(title, 'Version')
    require(unique <= 0xFFFFF and variation <= 255, 'Title ID fields are out of range')
    require(variation == 1, 'Keep TitleInfo.Version fixed at 1; it is the title variation, not release version')
    access = rsf_section(text, 'AccessControlInfo')
    system = rsf_section(text, 'SystemControlInfo')
    dependency_block = re.search(r'^  Dependency:\s*\n((?:    [^\n]*\n?)*)', system, re.M)
    require(dependency_block is not None, 'Missing dependency list')
    dependencies = [int(value, 16) for value in re.findall(r'^    \S+:\s*(0x[0-9a-fA-F]+)\s*$', dependency_block.group(1), re.M)]
    return {'title_id': 0x0004000000000000 | (unique << 8) | variation,
            'priority': rsf_number(access, 'Priority') + 32,
            'dependencies': dependencies,
            'stack': rsf_number(system, 'StackSize')}


def check_ncch(data, config):
    require(part(data, 0x100, 4) == b'NCCH', 'Missing NCCH header')
    require(data[0x18E] == 0 and le32(data, 0x104) * 512 == len(data), 'Invalid NCCH size or unit')
    require(data[0x18F] & 4, 'Encrypted NCCH is unsupported by this gate')
    require(le64(data, 0x108) == le64(data, 0x118) == config['title_id'], 'NCCH title ID mismatch')
    require(le32(data, 0x180) == 0x400, 'Unexpected exheader size')
    exhdr = part(data, 0x200, 0x400)
    acex = part(data, 0x600, 0x400)
    require(digest(exhdr) == part(data, 0x160, 32), 'Exheader SHA-256 mismatch')
    require(exhdr[:8] == b'LumaYUpd' and le64(exhdr, 0x200) == config['title_id'], 'Exheader identity mismatch')
    require(le64(acex, 0x200) == config['title_id'], 'Access descriptor title ID mismatch')
    require(exhdr[0x20C:0x20F] == bytes([0, 0, 4]), 'Old3DS CPU, memory mode or affinity mismatch')
    require(le32(exhdr, 0x1C) == config['stack'] == 0x10000 and le64(exhdr, 0x1C0) == 0,
            'Unexpected stack or savedata size')
    require(exhdr[13] & 2, 'SD application flag is missing')
    require(int.from_bytes(exhdr[0x248:0x24F], 'little') == 0x80, 'SDMC-only filesystem access is required')
    require(exhdr[0x3F0:0x400] == bytes.fromhex('00030000000000000000000000000002'),
            'Unexpected ARM9 filesystem capabilities')
    services = [part(exhdr, 0x250 + i * 8, 8).rstrip(b'\0') for i in range(34)]
    services = [name.decode('ascii') for name in services if name]
    required_services = {'APT:U', 'ac:u', 'cfg:u', 'fs:USER', 'gsp::Gpu', 'hid:USER', 'ndm:u', 'ps:ps', 'soc:U'}
    require(set(services) == required_services and len(services) == len(required_services), 'Service ACL mismatch')
    require(acex[0x230:0x370] == exhdr[0x230:0x370], 'Access descriptor filesystem or service ACL mismatch')
    require(acex[0x370:0x400] == exhdr[0x370:0x400], 'Access descriptor kernel capabilities mismatch')
    descriptors = struct.unpack('<28I', part(exhdr, 0x370, 112))
    syscalls = set()
    for descriptor in descriptors:
        if descriptor >> 27 == 0x1E:
            group = (descriptor >> 24) & 7
            syscalls.update(group * 24 + bit for bit in range(24) if descriptor & (1 << bit))
    required_syscalls = {1, 2, 3, 8, 9, 10, 11, 12, 19, 20, 21, 22, 23, 24, 25, 30, 31, 32,
                         33, 34, 35, 36, 37, 39, 40, 42, 43, 45, 50, 53, 56, 57, 58, 60, 61, 82, 83, 84}
    require(syscalls == required_syscalls, 'SVC permission mask mismatch')
    require({0xFE000200, 0xFF00112C, 0xFC00022E} <= set(descriptors), 'Handle, memory, CPU or kernel flags mismatch')
    dependencies = [le64(exhdr, 0x40 + i * 8) for i in range(48)]
    require([value for value in dependencies if value] == config['dependencies'], 'Dependency list mismatch')
    exefs = part(data, le32(data, 0x1A0) * 512, le32(data, 0x1A4) * 512)
    require(digest(part(exefs, 0, le32(data, 0x1A8) * 512)) == part(data, 0x1C0, 32), 'ExeFS header SHA-256 mismatch')
    files = {}
    ranges = []
    for index in range(8):
        row = part(exefs, index * 16, 16)
        if not row[:8].strip(b'\0'):
            continue
        name = row[:8].rstrip(b'\0').decode('ascii')
        offset, size = struct.unpack('<II', row[8:])
        require(name not in files, 'Repeated ExeFS entry')
        require(all(offset >= end or offset + size <= begin for begin, end in ranges), 'Overlapping ExeFS entries')
        ranges.append((offset, offset + size))
        payload = part(exefs, 512 + offset, size)
        require(digest(payload) == part(exefs, 0x100 + (7 - index) * 32, 32), 'ExeFS SHA-256 mismatch: ' + name)
        files[name] = {'bytes': size, 'sha256': digest(payload).hex()}
        if name == 'icon':
            require(size == 0x36C0 and payload[:4] == b'SMDH', 'Invalid SMDH icon')
            require(le32(payload, 0x2028) & 1 and le32(payload, 0x2018) == 0x7FFFFFFF, 'HOME visibility or region flags mismatch')
            require(any(payload[0x2040:0x24C0]) and any(payload[0x24C0:0x36C0]), 'Empty HOME icon')
            for language in (0, 1):
                start = 8 + language * 512
                short = payload[start:start + 128].decode('utf-16le').split('\0')[0]
                author = payload[start + 384:start + 512].decode('utf-16le').split('\0')[0]
                require(short == 'Luma3DS 13.4Y Updater' and author == '@yu_', 'HOME title or author mismatch')
        if name == 'banner':
            require(payload[:4] == b'CBMD' and size > 0x88, 'Invalid HOME banner')
    require(set(files) == {'.code', 'banner', 'icon', 'logo'}, 'Unexpected ExeFS entries')
    logo_size = le32(data, 0x19C) * 512
    if logo_size:
        require(digest(part(data, le32(data, 0x198) * 512, logo_size)) == part(data, 0x130, 32), 'Logo SHA-256 mismatch')
    priority = exhdr[0x20F]
    require(priority <= 63, f'Main priority {priority} exceeds 63; makerom adds 32 to Application RSF Priority')
    require(priority == config['priority'] == 0x30, 'Main priority must be 0x30; set RSF Priority to 16')
    require(priority >= acex[0x20F], 'Main priority violates its access descriptor')
    return {'title_id': f'{config["title_id"]:016X}', 'main_priority': priority,
            'old3ds_legacy': True, 'services': services, 'syscalls': sorted(syscalls), 'files': files}


def check_cia(path, rsf):
    require(0x2020 <= path.stat().st_size <= 128 * 1024 * 1024, 'CIA file size is out of range')
    data = path.read_bytes()
    header, kind, version, cert_size, ticket_size, tmd_size, meta_size, content_size = struct.unpack('<IHHIIIIQ', part(data, 0, 32))
    require(header == 0x2020 and kind == 0 and version == 0, 'Unexpected CIA header')
    config = configuration(rsf)
    sections = {}
    offsets = {}
    offset = align64(header)
    for name, size in [('cert', cert_size), ('ticket', ticket_size), ('tmd', tmd_size), ('content', content_size), ('meta', meta_size)]:
        require(size > 0, 'Missing CIA section: ' + name)
        offsets[name] = offset
        sections[name] = part(data, offset, size)
        offset = align64(offset + size)
    require(len(data) == offsets['meta'] + meta_size, 'Trailing or truncated CIA data')
    tmd = sections['tmd']
    body = signed_body(tmd)
    require(be64(tmd, body + 0x4C) == config['title_id'] and be16(tmd, body + 0x9E) == 1, 'TMD title ID or content count mismatch')
    infos = part(tmd, body + 0xC4, 0x900)
    require(digest(infos) == part(tmd, body + 0xA4, 32), 'TMD content-info SHA-256 mismatch')
    records = part(tmd, body + 0x9C4, 48)
    covered = set()
    for index in range(64):
        info = part(infos, index * 36, 36)
        first, count = struct.unpack('>HH', info[:4])
        if count:
            require(first + count <= 1 and not covered, 'Invalid TMD content-info coverage')
            require(digest(part(records, first * 48, count * 48)) == info[4:], 'TMD content-record SHA-256 mismatch')
            covered.update(range(first, first + count))
    require(covered == {0}, 'Missing TMD content-info record')
    content_id, index, flags, size = struct.unpack('>IHHQ', records[:16])
    require(index == 0 and flags & 1 == 0 and size == content_size, 'TMD content flags or size mismatch')
    require(data[0x20] == 0x80 and not any(data[0x21:0x2020]), 'CIA content-index mismatch')
    require(digest(sections['content']) == records[16:], 'TMD content SHA-256 mismatch')
    ticket = sections['ticket']
    require(be64(ticket, signed_body(ticket) + 0x9C) == config['title_id'], 'Ticket title ID mismatch')
    ncch = check_ncch(sections['content'], config)
    exhdr = part(sections['content'], 0x200, 0x400)
    require(meta_size == 0x3AC0, 'Unexpected CIA metadata size')
    require(part(sections['meta'], 0, 0x180) == part(exhdr, 0x40, 0x180), 'CIA metadata dependencies mismatch')
    require(le32(sections['meta'], 0x300) == le32(exhdr, 0x208) == 2, 'CIA metadata core version mismatch')
    require(digest(part(sections['meta'], 0x400, 0x36C0)).hex() == ncch['files']['icon']['sha256'], 'CIA metadata icon mismatch')
    require(be16(tmd, body + 0x9C) == 1025, 'TMD version must be 1.0.1 (1025)')
    return {'result': 'pass', 'bytes': len(data), 'sha256': digest(data).hex(), 'content_id': content_id,
            'tmd_version': 1025, 'ncch': ncch, 'hardware_tested': False, 'rsa_signatures_verified': False}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('cia', type=Path)
    parser.add_argument('--rsf', type=Path, default=Path(__file__).resolve().parents[1] / 'metadata.rsf')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    try:
        report = check_cia(args.cia, args.rsf)
    except (InvalidCia, OSError, ValueError, UnicodeError, struct.error, IndexError) as error:
        print('CIA rejected: ' + str(error), file=sys.stderr)
        return 1
    if args.report:
        args.report.write_bytes((json.dumps(report, indent=2) + '\n').encode('utf-8'))
    print(json.dumps(report))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
