import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]
ROS = ROOT / 'Luma3DS_v13_4/sysmodules/rosalina'
OUT = ROOT / 'build/rosalina-135-tests'
OUT.mkdir(parents=True, exist_ok=True)


def function(text, name):
    start = text.rfind('\n', 0, text.index(name + '(')) + 1
    opened = text.index('{', start)
    end, depth = opened + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


utils = (ROS / 'source/utils.c').read_text(encoding='utf8')
utils_header = (ROS / 'include/utils.h').read_text(encoding='utf8')
filters = (ROS / 'source/menus/screen_filters.c').read_text(encoding='utf8')
filters_header = (ROS / 'include/menus/screen_filters.h').read_text(encoding='utf8')
system = (ROS / 'source/menus/sysconfig.c').read_text(encoding='utf8')
config_header = (ROS / 'include/luma_config.h').read_text(encoding='utf8')

calendar_type = re.search(r'typedef struct CalendarTime\s*\{.*?\} CalendarTime;', utils_header, re.S)[0]
filter_type = re.search(r'typedef struct ScreenFilter\s*\{.*?\} ScreenFilter;', filters_header, re.S)[0]
fields = re.search(r'enum\s*\{\s*SCREEN_FILTER_FIELD_TEMPERATURE.*?\};', filters, re.S)[0]
overrides = config_header[config_header.index('#define MCUCFG_FLAG_ENHANCE_TOP_SCREEN_COLORS'):config_header.index('extern LumaMcuConfig')]
lut = re.search(r'static const u8 stereoBalanceAttenuationLut\[100\] = \{.*?\};', system, re.S)[0]
dates = [datetime.datetime(1900, 1, 1), datetime.datetime(2000, 2, 29, 23, 59, 59, 999000),
         datetime.datetime(2038, 1, 19, 3, 14, 8), datetime.datetime(2100, 3, 1),
         datetime.datetime(2106, 2, 7, 6, 28, 16), datetime.datetime(9999, 12, 31, 23, 59, 59, 999000)]
dates += [datetime.datetime(year, month, 1, 12, 34, 56, 789000)
          for year in (1900, 2000, 2026, 2100) for month in range(1, 13)]
epoch = datetime.datetime(1900, 1, 1)
rows = []
for value in dates:
    delta = value - epoch
    milliseconds = delta.days * 86400000 + delta.seconds * 1000 + delta.microseconds // 1000
    rows.append('{%dull,%d,%d,%d,%d,%d,%d,%d,%d}' %
                (milliseconds, value.year, value.month, value.day, (value.weekday() + 1) % 7,
                 value.hour, value.minute, value.second, value.microsecond // 1000))

prefix = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;typedef int8_t s8;typedef uint16_t u16;typedef int16_t s16;
typedef unsigned long u32;typedef long s32;typedef uint64_t u64;typedef long Result;
#define BIT(n) (1u<<(n))
#define CLAMP(x,a,b) ((x)<(a)?(a):(x)>(b)?(b):(x))
#define R_SUCCEEDED(x) ((x)>=0)
#define MS_IN_A_DAY (1000ull*3600*24)
#define SCREEN_FILTER_MAX_GAMMA 4.0f
#define SCREEN_FILTER_MAX_CONTRAST 8.0f
static unsigned applied[2],mcuWrites,mcuCloses;
static u8 ledValue;
static Result mcuHwcInit(void){return 0;}
static void mcuHwcExit(void){++mcuCloses;}
static Result MCUHWC_WriteRegister(u32 reg,const void *value,u32 count)
{assert(reg==0x28&&count==1);ledValue=*(const u8*)value;++mcuWrites;return 0;}
static void ScreenFiltersMenu_ApplyColorSettings(bool top){++applied[top?0:1];}
'''
code = prefix + calendar_type + '\n' + filter_type + '\n' + overrides + 'LumaMcuConfig lumaMcuConfig;\n'
code += 'ScreenFilter topScreenFilter,bottomScreenFilter;\n' + fields + lut
for name in ('calendarTimeFromMsSince1900', 'calendarTimeToString', 'dateTimeToString'):
    code += function(utils, name)
for name in ('ScreenFiltersMenu_IsUnified', 'ScreenFiltersMenu_Unify', 'ScreenFiltersMenu_StepChannel',
             'ScreenFiltersMenu_AdvancedConfigurationChangeValue'):
    code += function(filters, name)
for name in ('SysConfigMenu_ApplyLedMasterBrightnessOverride', 'SysConfigMenu_ConvertSliderPercentageToVolume',
             'SysConfigMenu_AttenuateVolume'):
    code += function(system, name)
code += r'''
struct Expected {u64 ms;s32 year;int month,day,weekday,hour,minute,second,subsecond;};
static const struct Expected dates[]={
''' + ',\n'.join(rows) + r'''
};
int main(void)
{
    for(unsigned i=0;i<sizeof(dates)/sizeof(dates[0]);++i){
        CalendarTime t;calendarTimeFromMsSince1900(&t,dates[i].ms);
        const struct Expected *e=&dates[i];
        assert(t.year==e->year&&t.month==e->month&&t.day==e->day&&t.weekday==e->weekday);
        assert(t.hour==e->hour&&t.minute==e->minute&&t.second==e->second&&t.ms==e->subsecond);
        char value[64];assert(calendarTimeToString(value,&t)>0);
        assert(dateTimeToString(value,e->ms,true)>0);
    }
    for(u32 pct=0;pct<=100;++pct){
        s8 volume=SysConfigMenu_ConvertSliderPercentageToVolume(pct);
        assert(pct!=0||volume==-128);
        assert(pct!=100||volume==-20);
        if(pct!=0)assert(volume>=SysConfigMenu_ConvertSliderPercentageToVolume(pct-1));
        assert(SysConfigMenu_AttenuateVolume(volume,0)==volume);
        for(u32 balance=1;balance<=100;++balance)
            assert(SysConfigMenu_AttenuateVolume(volume,balance)<=SysConfigMenu_AttenuateVolume(volume,balance-1));
        assert(SysConfigMenu_AttenuateVolume(volume,100)==-128);
    }
    for(u32 value=0;value<256;++value){
        lumaMcuConfig.ledMasterBrightnessOverride=value;
        assert(SysConfigMenu_ApplyLedMasterBrightnessOverride()==0);
        assert(ledValue==((value<=100)?(255*value+50)/100:255));
    }
    assert(mcuWrites==256&&mcuCloses==256);
    topScreenFilter=(ScreenFilter){.cct=6500,.gamma={1,1,1},.contrast={1,1,1}};
    bottomScreenFilter=topScreenFilter;
    ScreenFiltersMenu_AdvancedConfigurationChangeValue(SCREEN_FILTER_FIELD_GAMMA,10,false,true,1);
    assert(topScreenFilter.gamma[1]>1.09f&&topScreenFilter.gamma[0]==1&&topScreenFilter.gamma[2]==1);
    assert(bottomScreenFilter.gamma[1]==1&&!ScreenFiltersMenu_IsUnified(&topScreenFilter));
    ScreenFiltersMenu_Unify(&topScreenFilter);assert(ScreenFiltersMenu_IsUnified(&topScreenFilter));
    ScreenFiltersMenu_AdvancedConfigurationChangeValue(SCREEN_FILTER_FIELD_GAMMA,10000,true,false,0);
    for(unsigned ch=0;ch<3;++ch)assert(topScreenFilter.gamma[ch]==4&&bottomScreenFilter.gamma[ch]==4);
    ScreenFiltersMenu_AdvancedConfigurationChangeValue(SCREEN_FILTER_FIELD_CONTRAST,-10000,true,false,0);
    for(unsigned ch=0;ch<3;++ch)assert(topScreenFilter.contrast[ch]==0&&bottomScreenFilter.contrast[ch]==0);
    ScreenFiltersMenu_AdvancedConfigurationChangeValue(SCREEN_FILTER_FIELD_CONTRAST,10000,true,true,2);
    assert(topScreenFilter.contrast[2]==8&&bottomScreenFilter.contrast[2]==8);
    assert(topScreenFilter.contrast[0]==0&&bottomScreenFilter.contrast[1]==0);
    ScreenFiltersMenu_AdvancedConfigurationChangeValue(SCREEN_FILTER_NUM_FIELDS_PER_SCREEN+SCREEN_FILTER_FIELD_INVERT,1,false,false,0);
    assert(!topScreenFilter.invert&&bottomScreenFilter.invert);
    assert(applied[0]==5&&applied[1]==5);
    puts("Calendar, RGB editing, stereo attenuation and LED override: PASS");
}
'''
target = OUT / 'rosalina_135.c'
target.write_text(code, encoding='utf8')
compiler = os.environ.get('CC', 'cc')
if not shutil.which(compiler) and os.name == 'nt':
    compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
environment = dict(os.environ, PATH=str(Path(compiler).parent) + os.pathsep + os.environ.get('PATH', ''))
binary = target.with_suffix('.exe' if os.name == 'nt' else '')
subprocess.run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
                str(target), '-o', str(binary)], check=True, env=environment)
subprocess.run([str(binary)], check=True, env=environment)
paths = [ROS / path for path in ('source/utils.c', 'source/menus/screen_filters.c', 'source/menus/sysconfig.c',
                                 'include/utils.h', 'include/menus/screen_filters.h', 'include/luma_config.h')]
report = dict(passed=True, calendar_cases=len(dates), volume_percentages=101, led_values=256,
              hardware_tested=False, source_sha256={str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                                                   for path in paths})
(OUT / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf8')
