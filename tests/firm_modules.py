from pathlib import Path
import argparse
import importlib.util
import json
import struct

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('firm_modules',ROOT/'tools/firm_modules.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

def fixture(sizes=(0x6000,0x2400,0x4800,0x2400,0x3BC00)):
    total=sum(sizes)
    result=bytearray(0x200+total)
    result[:4]=b'FIRM'
    struct.pack_into('<4I',result,0x40,0x200,0x18180000,total,0)
    offset=0x200
    for name,size in zip(module.EXPECTED_MODULES,sizes):
        result[offset+0x100:offset+0x104]=b'NCCH'
        struct.pack_into('<I',result,offset+0x104,size//0x200)
        result[offset+0x200:offset+0x200+len(name)]=name.encode()
        offset+=size
    return result

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--firm',type=Path)
    parser.add_argument('--expect-reject',action='store_true')
    args=parser.parse_args()
    checks=0
    def accept(data):
        nonlocal checks
        result=module.bundled_native_modules(data);checks+=1;return result
    def reject(data):
        nonlocal checks
        try:module.bundled_native_modules(data)
        except (ValueError,UnicodeError):checks+=1;return
        raise AssertionError('Invalid FIRM module budget accepted')
    result=accept(fixture())
    assert result['total']==0x4AC00 and result['remaining_for_stock_modules']==0x35400
    assert [r['name'] for r in result['modules']]==list(module.EXPECTED_MODULES)
    accept(fixture((0x6000,0x2400,0x4800,0x2400,0x22E00)))
    reject(fixture((0x6000,0x2400,0x4800,0x2400,0x5EA00)))
    reject(fixture((0x6000,0x2400,0x4800,0x2400,0x3BE00)))
    good=fixture()
    for offset,value in ((0,0),(0x44,0x18000000),(0x40,0x201),(0x48,0xFFFFFFFF),(0x4C,3),(0x300,0),(0x304,0),(0x304,0xFFFFFFFF),(0x38E,1)):
        broken=bytearray(good);struct.pack_into('<I',broken,offset,value);reject(broken)
    for length in (0,0x100,0x200,len(good)-1):
        reject(good[:length])
    broken=bytearray(good);broken[0x400:0x408]=b'unknown\0';reject(broken)
    broken=bytearray(good);broken[0x6400:0x6408]=b'loader\0\0';reject(broken)
    broken=bytearray(good);struct.pack_into('<4I',broken,0x70,0x200,0x18180000,len(good)-0x200,0);reject(broken)
    for base in [ROOT]:
        ros=base/'Luma3DS_v13_4/sysmodules/rosalina'
        make=(ros/'Makefile').read_text(encoding='utf8')
        sources=next(line for line in make.splitlines() if line.startswith('SOURCES')).split(':=',1)[1].strip().split()
        assert not any('mbedtls' in source for source in sources)
        assert 'CFILES := $(filter-out updater%.c,$(CFILES))' in make
        assert 'MBEDTLS_CONFIG_FILE' not in make
        menu=(ros/'source/menus/miscellaneous.c').read_text(encoding='utf8')
        assert 'RosalinaMenu_Update' not in menu and 'menus/updater.h' not in menu
        checks+=4
    if args.firm:
        (reject if args.expect_reject else accept)(args.firm.read_bytes())
    print(json.dumps(dict(result='pass',checks=checks,bundled_limit=hex(module.BUNDLED_NATIVE_BASELINE),native_runtime_limit=hex(module.NATIVE_MODULE_LIMIT),basis='known-working previous release bundled CXI total; no runtime region expansion',firm=str(args.firm) if args.firm else None)))

if __name__=='__main__':main()
