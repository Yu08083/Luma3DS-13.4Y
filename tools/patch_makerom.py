import argparse
import hashlib
import json
from pathlib import Path

UPSTREAM_COMMIT = 'e8f5f529c54ff9b22a2491a480ffa69206bf7b19'
UPSTREAM_URL = 'https://github.com/3DSGuy/Project_CTR/blob/' + UPSTREAM_COMMIT + '/makerom/src/code.c'
ORIGINAL_SHA256 = 'ce7f2b0c783e4ee644a4043c6ee685abde89f6345ca6033bb246db3498d256c3'
MODE_EXPRESSION = '(getenv("MAKEROM_BLZ_BEST") && strcmp(getenv("MAKEROM_BLZ_BEST"), "1") == 0 ? BLZ_BEST : BLZ_NORMAL)'
OPTIMAL_HEADER = Path(__file__).with_name('blz_optimal.h').read_bytes().replace(b'\r\n', b'\n')
OPTIMAL_PREFIX = OPTIMAL_HEADER + b'\n'


def patch(raw):
    source = raw.replace(b'\r\n', b'\n')
    replacement = MODE_EXPRESSION.encode()
    body = source[len(OPTIMAL_PREFIX):] if source.startswith(OPTIMAL_PREFIX) else source
    reverse = body.replace(replacement, b'BLZ_NORMAL').replace(b'YuBlz_Code(', b'BLZ_Code(')
    if hashlib.sha256(reverse).hexdigest() != ORIGINAL_SHA256:
        raise ValueError('makerom source does not match pinned ' + UPSTREAM_COMMIT)
    if reverse.count(b', BLZ_NORMAL);') != 2 or reverse.count(b'BLZ_Code(') != 2:
        raise ValueError('Unexpected makerom compression call count')
    previous = reverse.replace(b', BLZ_NORMAL);', b', ' + replacement + b');')
    patched = OPTIMAL_PREFIX + previous.replace(b'BLZ_Code(', b'YuBlz_Code(')
    if source not in (reverse, previous, patched):
        raise ValueError('Unknown makerom patch state')
    return patched


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    args = parser.parse_args()
    raw = args.source.read_bytes()
    patched = patch(raw)
    if raw != patched:
        args.source.write_bytes(patched)
    print(json.dumps({'source': str(args.source), 'upstream_commit': UPSTREAM_COMMIT, 'sha256': hashlib.sha256(patched).hexdigest(), 'best_mode_env': 'MAKEROM_BLZ_BEST=1', 'optimal_mode_env': 'MAKEROM_BLZ_OPTIMAL=1', 'optimal_header_sha256': hashlib.sha256(OPTIMAL_HEADER).hexdigest()}))


if __name__ == '__main__':
    main()
