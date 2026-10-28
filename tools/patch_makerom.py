import argparse
import hashlib
import json
from pathlib import Path

UPSTREAM_COMMIT = 'e8f5f529c54ff9b22a2491a480ffa69206bf7b19'
UPSTREAM_URL = 'https://github.com/3DSGuy/Project_CTR/blob/' + UPSTREAM_COMMIT + '/makerom/src/code.c'
ORIGINAL_SHA256 = 'ce7f2b0c783e4ee644a4043c6ee685abde89f6345ca6033bb246db3498d256c3'
MODE_EXPRESSION = '(getenv("MAKEROM_BLZ_BEST") && strcmp(getenv("MAKEROM_BLZ_BEST"), "1") == 0 ? BLZ_BEST : BLZ_NORMAL)'


def patch(raw):
    source = raw.replace(b'\r\n', b'\n')
    replacement = MODE_EXPRESSION.encode()
    reverse = source.replace(replacement, b'BLZ_NORMAL')
    if hashlib.sha256(source).hexdigest() == ORIGINAL_SHA256:
        if source.count(b', BLZ_NORMAL);') != 2:
            raise ValueError('Unexpected makerom compression call count')
        return source.replace(b', BLZ_NORMAL);', b', ' + replacement + b');')
    if source.count(replacement) == 2 and hashlib.sha256(reverse).hexdigest() == ORIGINAL_SHA256:
        return source
    raise ValueError('makerom source does not match pinned ' + UPSTREAM_COMMIT)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    args = parser.parse_args()
    raw = args.source.read_bytes()
    patched = patch(raw)
    if raw != patched:
        args.source.write_bytes(patched)
    print(json.dumps({'source': str(args.source), 'upstream_commit': UPSTREAM_COMMIT, 'sha256': hashlib.sha256(patched).hexdigest(), 'best_mode_env': 'MAKEROM_BLZ_BEST=1'}))


if __name__ == '__main__':
    main()
