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

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / 'tools/blz_optimal.h'
compiler = shutil.which(os.environ.get('CC', 'cc')) or shutil.which('gcc')
if not compiler and Path('C:/msys64/ucrt64/bin/gcc.exe').exists():
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
assert compiler
environment = dict(os.environ)
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')


def decode(encoded):
    increment = int.from_bytes(encoded[-4:], 'little')
    if increment == 0:
        return encoded[:-4], []
    section = int.from_bytes(encoded[-8:-5], 'little')
    header = encoded[-5]
    assert 8 <= header <= 11 and header <= section <= len(encoded)
    prefix = len(encoded) - section
    output = bytearray(encoded + bytes(increment))
    source, destination = len(encoded) - header, len(output)
    matches = []
    while destination > prefix:
        assert source > prefix
        source -= 1
        flags = output[source]
        for bit in range(8):
            if destination == prefix:
                break
            if flags & (128 >> bit):
                assert source - prefix >= 2
                source -= 2
                value = output[source + 1] * 256 + output[source]
                length, distance = (value >> 12) + 3, (value & 4095) + 3
                assert 3 <= length <= 18 and 3 <= distance <= 4098
                assert destination - prefix >= length and distance <= len(output) - destination
                matches.append((length, distance))
                for _ in range(length):
                    destination -= 1
                    assert destination >= source
                    output[destination] = output[destination + distance]
            else:
                assert source > prefix
                source -= 1
                destination -= 1
                assert destination >= source
                output[destination] = output[source]
    assert source == destination == prefix
    return bytes(output), matches


def oracle(raw):
    reverse = raw[::-1]
    costs = {(0, 0): 0}
    best = len(raw)
    for position in range(len(raw) + 1):
        lengths = {1}
        for distance in range(3, min(4098, position) + 1):
            length = 0
            while length < min(18, len(raw) - position) and reverse[position + length] == reverse[position + length - distance]:
                length += 1
            lengths.update(range(3, length + 1))
        for state in range(8):
            cost = costs.get((position, state))
            if cost is None:
                continue
            best = min(best, len(raw) - position + cost)
            for length in lengths:
                if position + length > len(raw):
                    continue
                target = (position + length, (state + 1) % 8)
                value = cost + int(state == 0) + (1 if length == 1 else 2)
                costs[target] = min(costs.get(target, value), value)
    compressed = ((best + 3) & ~3) + 8
    return compressed if compressed < len(raw) else ((len(raw) + 3) & ~3) + 4


rng = random.Random(0x13520)
cases = [b'', b'a', b'ab', b'abc', bytes(range(256)), bytes(range(255)), b'abc' * 100, b'A' * 4099]
for size in [3, 4, 7, 8, 9, 17, 18, 19, 31, 63, 64, 65, 127, 128, 255, 256, 4097, 4098, 4099, 8192]:
    cases.extend([rng.randbytes(size), bytes(rng.randrange(4) for _ in range(size)), (b'abcde' * ((size + 4) // 5))[:size]])
for distance in [3, 4, 17, 18, 4097, 4098]:
    sequence = rng.randbytes(distance)
    cases.append((sequence + sequence[:18] + b'xyz' * 80)[::-1])
    if distance >= 4097:
        cases.append((sequence * 3)[::-1])
cases.extend(bytes(rng.randrange(3) for _ in range(rng.randrange(20, 70))) for _ in range(45))
with tempfile.TemporaryDirectory(prefix='blz-optimal-', ignore_cleanup_errors=True) as directory:
    stage = Path(directory)
    (stage / 'blz.h').write_text('#include <stdint.h>\nuint8_t *BLZ_Code(uint8_t *, int, uint32_t *, int);\n')
    source = stage / 'test.c'
    source.write_text('#include "' + HEADER.as_posix() + '"\nuint8_t *encode(const uint8_t *p,int n,uint32_t *s){return YuBlz_Optimal(p,n,s,1);}\nint verify(const uint8_t *p,uint32_t n,const uint8_t *r,uint32_t s){return YuBlz_Verify(p,n,r,s);}\nvoid release(void *p){free(p);}\n')
    target = stage / ('test.dll' if os.name == 'nt' else 'test.so')
    subprocess.run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-shared', '-fPIC', '-I' + str(stage), str(source), '-o', str(target)], check=True, env=environment)
    if os.name == 'nt':
        runtime = os.add_dll_directory(str(Path(compiler).parent))
    library = c.CDLL(str(target))
    library.encode.argtypes = [c.c_void_p, c.c_int, c.POINTER(c.c_uint32)]
    library.encode.restype = c.c_void_p
    library.verify.argtypes = [c.c_void_p, c.c_uint32, c.c_void_p, c.c_uint32]
    library.release.argtypes = [c.c_void_p]
    compressed_cases = plain_cases = oracle_cases = rejected_mutations = 0
    observed = []
    for raw in cases:
        data = c.create_string_buffer(raw)
        size = c.c_uint32()
        pointer = library.encode(data, len(raw), c.byref(size))
        assert pointer and data.raw[:len(raw)] == raw
        encoded = c.string_at(pointer, size.value)
        library.release(pointer)
        actual, matches = decode(encoded)
        assert actual[:len(raw)] == raw
        if len(encoded) < len(raw):
            compressed_cases += 1
            assert len(actual) == len(raw) and library.verify(encoded, len(encoded), raw, len(raw)) == 1
            for offset in [len(encoded) - 1, len(encoded) - 5, len(encoded) - 8]:
                changed = bytearray(encoded)
                changed[offset] ^= 128
                assert library.verify(bytes(changed), len(changed), raw, len(raw)) == 0
                rejected_mutations += 1
        else:
            plain_cases += 1
            assert actual[len(raw):] == bytes((-len(raw)) % 4)
            assert library.verify(encoded, len(encoded), raw, len(raw)) == 0
        if len(raw) <= 70:
            assert len(encoded) == oracle(raw), (len(raw), len(encoded), oracle(raw))
            oracle_cases += 1
        observed.extend(matches)
    for length in [-1, 0x200001, 0x7FFFFFFF]:
        size = c.c_uint32()
        assert not library.encode(None, length, c.byref(size))
    assert not library.encode(None, 1, c.pointer(c.c_uint32()))
    assert not library.encode(None, 0, None)
    assert any(length == 18 for length, _ in observed)
    assert any(distance == 3 for _, distance in observed)
    assert any(distance == 4098 for _, distance in observed)
report = {'all_passed': True, 'header_sha256': hashlib.sha256(HEADER.read_bytes()).hexdigest(), 'roundtrip_cases': len(cases), 'compressed_cases': compressed_cases, 'unaligned_or_plain_cases': plain_cases, 'exhaustive_parse_oracle_cases': oracle_cases, 'invalid_arguments': 5, 'malformed_streams_rejected': rejected_mutations, 'input_unchanged': True, 'maximum_match_and_distance_observed': True}
output = ROOT / 'build/blz-optimal-tests'
output.mkdir(parents=True, exist_ok=True)
(output / 'proof.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report))
