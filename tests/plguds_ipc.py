import os
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
compiler = shutil.which('cc') or shutil.which('gcc')
if not compiler:
    fallback = Path('C:/msys64/ucrt64/bin/gcc.exe')
    if fallback.exists(): compiler = str(fallback)
assert compiler, 'A native C compiler is required'
environment = dict(os.environ)
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
include = ROOT / 'Luma3DS_v13_4/k11_extension/include'
with tempfile.TemporaryDirectory() as temporary:
    directory = Path(temporary)
    kernel = (ROOT / 'Luma3DS_v13_4/k11_extension/source/svc/SendSyncRequest.c').read_text(encoding='utf-8')
    lines = [line for line in kernel.splitlines() if not line.startswith('#include')]
    combined = (ROOT / 'tests/plguds_kernel_prefix.c').read_text(encoding='utf-8') + '\n' + '\n'.join(lines) + '\n' + (ROOT / 'tests/plguds_kernel_suffix.c').read_text(encoding='utf-8')
    generated = directory / 'kernel.c'
    generated.write_text(combined, encoding='utf-8')
    ipc = (ROOT / 'Luma3DS_v13_4/k11_extension/source/ipc.c').read_text(encoding='utf-8')
    def function(name):
        first = ipc.index(name + '(')
        start = ipc.rfind('\n', 0, first) + 1
        opening = ipc.index('{', first)
        depth = 1
        end = opening + 1
        while depth:
            depth += (ipc[end] == '{') - (ipc[end] == '}')
            end += 1
        return ipc[start:end]
    registry = directory / 'registry.c'
    registry.write_text((ROOT / 'tests/plguds_session_prefix.c').read_text(encoding='utf-8') + '\n' + '\n'.join(function(name) for name in ['SessionInfo_FindClosestSlot', 'SessionInfo_Lookup', 'SessionInfo_Remove']) + '\n' + (ROOT / 'tests/plguds_session_suffix.c').read_text(encoding='utf-8'), encoding='utf-8')
    for name, source in [('parser', ROOT / 'tests/plguds_parser_test.c'), ('kernel', generated), ('registry', registry)]:
        output = directory / (name + ('.exe' if os.name == 'nt' else ''))
        subprocess.run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-I' + str(include), str(source), '-o', str(output)], check=True, env=environment)
        subprocess.run([str(output)], check=True, env=environment)
