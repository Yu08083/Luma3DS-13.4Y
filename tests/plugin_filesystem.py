import argparse
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--output', type=Path)
args = parser.parse_args()
compiler = shutil.which('cc') or shutil.which('gcc')
if not compiler:
    fallback = Path('C:/msys64/ucrt64/bin/gcc.exe')
    if fallback.exists():
        compiler = str(fallback)
assert compiler
environment = dict(os.environ)
environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
kernel_path = ROOT / 'Luma3DS_v13_4/k11_extension/source/svc/SendSyncRequest.c'
kernel = kernel_path.read_text(encoding='utf8')
prefix = (ROOT / 'tests/plguds_kernel_prefix.c').read_text(encoding='utf8')
prefix = prefix.replace('static Result SendSyncRequest(Handle handle)', 'static Result BaseSendSyncRequest(Handle handle)')
suffix = (ROOT / 'tests/plguds_kernel_suffix.c').read_text(encoding='utf8')
suffix = suffix.replace('int main(void)', 'static void baseline_main(void)')
adapter = r'''
static bool fs_case;
static u32 fs_expected[96], fs_reply[96];
static Result fs_result;
static unsigned fs_forwarded;
static Result SendSyncRequest(Handle handle)
{
    if (!fs_case) return BaseSendSyncRequest(handle);
    assert(handle == 5 || handle == 8);
    assert(!locks && !memcmp(tls + 32, fs_expected, sizeof(fs_expected)));
    fs_forwarded++;
    memcpy(tls + 32, fs_reply, sizeof(fs_reply));
    return fs_result;
}
'''
cases = r'''
int main(void)
{
    baseline_main();
    unsigned cases = 0;
    const char *services[] = {"fs:USER", "fs:LDR", "fs:REG", ""};
    const u32 commands[] = {
        0x080201C2, 0x08030204, 0x080200C2, 0x08030102,
        0x08040000, 0x08050080, 0x08080000, 0x08090000,
        0x080B0102, 0x080C00C2, 0x080E0080, 0x080F0180,
        0x08170000, 0x082A0082, 0x08610042,
        0x10042, 0x10082, 0x20000, 0x20002, 0x50100,
        0x80040, 0x90000, 0x4010082, 0x4020082, 0x8010082,
        0x8020082, 0x4060000, 0x8160000, 0x00C0080, 0x00D0080
    };
    const Result results[] = {0, (Result)0xC8804478, -13};
    for (unsigned state = 0; state < 5; state++)
    {
        fs_case = false;
        setup();
        if (state == 3)
        {
            fail_initialize = true;
            request(100, 2, 0); tls[33] = 8;
            assert(SendSyncRequestHook(1) == 0 && (s32)tls[33] < 0);
            fail_initialize = false;
        }
        else if (state)
        {
            initialize();
            if (state == 2)
            {
                request(105, 1, 0); tls[33] = 1;
                assert(SendSyncRequestHook(1) == 0);
            }
            if (state == 4) core.objectContext.currentProcess = &processes[1];
        }
        for (unsigned service = 0; service < sizeof(services) / sizeof(*services); service++)
        {
            memset(infos[4].name, 0, sizeof(infos[4].name));
            memset(infos[7].name, 0, sizeof(infos[7].name));
            strcpy(infos[4].name, services[service]);
            strcpy(infos[7].name, services[service]);
            for (unsigned client = 0; client < 2; client++)
            for (unsigned command = 0; command < sizeof(commands) / sizeof(*commands); command++)
            for (unsigned result = 0; result < sizeof(results) / sizeof(*results); result++)
            {
                for (unsigned index = 0; index < 96; index++)
                    fs_expected[index] = 0xA5F00000u ^ (index * 0x01010101u);
                fs_expected[0] = commands[command];
                memcpy(fs_reply, fs_expected, sizeof(fs_reply));
                fs_reply[0] = (commands[command] & 0xFFFF0000u) | 0x40;
                fs_reply[1] = 0xD9004676;
                memcpy(tls + 32, fs_expected, sizeof(fs_expected));
                fs_result = results[result];
                fs_forwarded = 0;
                unsigned refs = sessions[client ? 7 : 4].clientSession.syncObject.autoObject.references;
                fs_case = true;
                assert(SendSyncRequestHook(client ? 8 : 5) == fs_result);
                fs_case = false;
                assert(fs_forwarded == 1 && !memcmp(tls + 32, fs_reply, sizeof(fs_reply)));
                assert(!locks && sessions[client ? 7 : 4].clientSession.syncObject.autoObject.references == refs);
                cases++;
            }
        }
    }
    printf("FS passthrough: %u exact request/response/TLS/result cases passed\n", cases);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as temporary:
    folder = Path(temporary)
    source = folder / 'filesystem.c'
    source.write_text(prefix + adapter + '\n' + '\n'.join(line for line in kernel.splitlines() if not line.startswith('#include')) + '\n' + suffix + cases, encoding='utf8')
    target = folder / ('filesystem.exe' if os.name == 'nt' else 'filesystem')
    subprocess.run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'Luma3DS_v13_4/k11_extension/include'), str(source), '-o', str(target)], check=True, env=environment)
    result = subprocess.run([str(target)], check=True, env=environment, capture_output=True, text=True)
    print(result.stdout, end='')
count = int(next(line for line in result.stdout.splitlines() if line.startswith('FS passthrough: ')).split()[2])
assert count == 3600
report = {'all_passed': True, 'case_count': count,
          'source_sha256': hashlib.sha256(kernel_path.read_bytes()).hexdigest(),
          'hardware_tested': False,
          'boundaries': ['The actual SendSyncRequestHook C executes against the existing host kernel/session fixture.',
                         'FS service results, OS handles, mapped user buffers and real SD reads are supplied by the fixture.',
                         'This proves IPC passthrough across UDS lifecycle states, not the reported physical TXT failure or all plugin path/encoding behavior.']}
if args.output:
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
