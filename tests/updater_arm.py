from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import concurrent.futures

ROOT=Path(__file__).resolve().parents[1]
ROS=ROOT/'Luma3DS_v13_4/sysmodules/rosalina'
OUT=ROOT/'build/updater-tests/arm'

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--compiler',default=str(ROOT.parent/'tools/bin/arm-none-eabi-gcc.exe'))
    parser.add_argument('--ctru',default=str(ROOT/'.libctru/v2.7.0'))
    args=parser.parse_args()
    OUT.mkdir(parents=True,exist_ok=True)
    compiler=Path(args.compiler).resolve()
    base=[str(compiler),'-c','-std=gnu11','-Os','-Wall','-Wextra','-Werror','-march=armv6k','-mtune=mpcore','-mfpu=vfp','-mfloat-abi=hard','-mtp=soft','-D__3DS__','-DMBEDTLS_CONFIG_FILE="updater_tls_config.h"','-I'+str(ROS/'include'),'-I'+str(ROS/'third_party/mbedtls/include'),'-I'+str(Path(args.ctru).resolve()/'include')]
    sources=sorted((ROS/'source').glob('updater*.c'))+[ROS/'source/menus/updater.c',ROS/'source/minisoc.c']
    sources+=sorted((ROS/'third_party/mbedtls/library').glob('*.c'))
    def compile_one(source):
        target=OUT/('menu_'+source.stem+'.o' if source.parent.name=='menus' else source.stem+'.o')
        result=subprocess.run(base+[str(source),'-o',str(target)],capture_output=True,text=True)
        return source,result
    failures=[]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        for source,result in pool.map(compile_one,sources):
            if result.returncode:failures.append(dict(source=str(source.relative_to(ROS)),output=result.stdout+result.stderr))
    if failures:
        for item in failures:print(json.dumps(item))
        raise AssertionError(str(len(failures))+' ARM compile failures')
    report=dict(result='pass',sources=len(sources),compiler=subprocess.check_output([str(compiler),'--version'],text=True).splitlines()[0],target='ARMv6K/VFP/hard, -Os -Wall -Wextra -Werror',source_sha256={str(x.relative_to(ROS)):hashlib.sha256(x.read_bytes()).hexdigest() for x in sources},hardware_tested=False,boundary='translation units only; production link requires devkitARM/libctru rules')
    (OUT.parent/'arm-proof.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print('ARM strict compile PASS:',len(sources),'translation units')

if __name__=='__main__':main()
