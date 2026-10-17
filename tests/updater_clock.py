import ctypes as c
from datetime import datetime,timezone
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess

root=Path(__file__).resolve().parents[1]
module=root/'Luma3DS_v13_4/sysmodules/rosalina'
generated=root/'build/updater-clock-tests'
generated.mkdir(parents=True,exist_ok=True)
compiler='C:/msys64/ucrt64/bin/gcc.exe' if os.name=='nt' else os.environ.get('CC','cc')
target=generated/('clock.dll' if os.name=='nt' else 'clock.so')
source=module/'source/updater_clock.c'
header=module/'include/updater_clock.h'
environment=dict(os.environ)
environment['PATH']=str(Path(compiler).parent)+os.pathsep+environment.get('PATH','')
subprocess.run([compiler,'-std=c11','-Wall','-Wextra','-Werror','-O2','-shared','-fPIC','-I'+str(module/'include'),str(source),'-o',str(target)],check=True,env=environment)
library=c.CDLL(str(target))
library.UpdateClock_FromDate.argtypes=[c.c_int]*6
library.UpdateClock_FromDate.restype=c.c_int64
library.UpdateClock_ParseHttpDate.argtypes=[c.c_char_p,c.POINTER(c.c_int64)]
library.UpdateClock_ParseHttpDate.restype=c.c_bool
cases=0
rng=random.Random(0x134C10C)
values=[datetime(1970,1,1,tzinfo=timezone.utc),datetime(2000,2,29,23,59,59,tzinfo=timezone.utc),datetime(2038,1,19,3,14,8,tzinfo=timezone.utc),datetime(9999,12,31,23,59,59,tzinfo=timezone.utc)]
for _ in range(20000):
    year=rng.randrange(1970,10000)
    month=rng.randrange(1,13)
    day=rng.randrange(1,29)
    values.append(datetime(year,month,day,rng.randrange(24),rng.randrange(60),rng.randrange(60),tzinfo=timezone.utc))
for value in values:
    expected=int((value-datetime(1970,1,1,tzinfo=timezone.utc)).total_seconds())
    assert library.UpdateClock_FromDate(value.year,value.month,value.day,value.hour,value.minute,value.second)==expected
    text=value.strftime('%a, %d %b %Y %H:%M:%S GMT').encode()
    output=c.c_int64(-123)
    assert library.UpdateClock_ParseHttpDate(text,c.byref(output)) and output.value==expected
    cases+=2
for fields in [(1969,12,31,23,59,59),(10000,1,1,0,0,0),(2026,0,1,0,0,0),(2026,13,1,0,0,0),(2026,1,0,0,0,0),(2026,1,32,0,0,0),(2026,2,29,0,0,0),(2100,2,29,0,0,0),(2000,2,30,0,0,0),(2026,1,1,-1,0,0),(2026,1,1,24,0,0),(2026,1,1,0,60,0),(2026,1,1,0,0,60),(-2147483648,1,1,0,0,0),(2147483647,1,1,0,0,0)]:
    assert library.UpdateClock_FromDate(*fields)==-1
    cases+=1
valid=b'Sat, 17 Oct 2026 15:00:00 GMT'
assert len(valid)==29
invalid=[b'',valid[:-1],valid+b' ',valid.replace(b'GMT',b'UTC'),valid.replace(b'GMT',b'gmt'),valid.replace(b'Sat',b'Sun'),valid.replace(b'2026',b'0026'),valid.replace(b'15:00:00',b'24:00:00'),valid.replace(b'Oct',b'Foo'),b'Sun, 29 Feb 2026 00:00:00 GMT',b'Saturday, 17-Oct-26 15:00:00 GMT',b'Sat Oct 17 15:00:00 2026']
for position in [3,4,7,11,16,19,22,25]:
    altered=bytearray(valid)
    altered[position]=ord('_')
    invalid.append(bytes(altered))
for position in [5,6,12,13,14,15,17,18,20,21,23,24]:
    altered=bytearray(valid)
    altered[position]=ord('x')
    invalid.append(bytes(altered))
for text in invalid:
    output=c.c_int64(123)
    assert not library.UpdateClock_ParseHttpDate(text,c.byref(output)) and output.value==123
    cases+=1
output=c.c_int64(123)
assert not library.UpdateClock_ParseHttpDate(None,c.byref(output)) and output.value==123
assert not library.UpdateClock_ParseHttpDate(valid,None)
assert library.UpdateClock_ParseHttpDate(valid,c.byref(output)) and output.value==1792249200
cases+=3
proof={'all_passed':True,'cases':cases,'random_date_cases':20000,'strict_imf_fixdate_gmt':True,'utc_build_floor':output.value,
       'source_sha256':{str(path.relative_to(root)):hashlib.sha256(path.read_bytes()).hexdigest() for path in [source,header]},'hardware_tested':False}
(generated/'report.json').write_bytes((json.dumps(proof,indent=2)+'\n').encode())
print(json.dumps(proof))
