from pathlib import Path
import hashlib
import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
ROS = ROOT / 'Luma3DS_v13_4/sysmodules/rosalina'
OUT = ROOT / 'build/streaming-old3ds-tests'


EXTRA = r'''
typedef volatile u16 vu16;
static bool realJpeg,realDsp,deadlineTest;
static unsigned deadlineSleeps,dspInvalidates,audioPackets;
static u8 audioWire[1400];
static u32 audioWireSize;
static struct sockaddr_in audioDestination;
void RealRpJpeg_SetQuality(RpJpeg *,int);
uint32_t RealRpJpeg_BytesPerPixel(int);
uint32_t RealRpJpeg_Encode(const RpJpeg *,u8 *,u32,const u8 *,u32,u32,u32,u32,u32,int);
static Result svcInvalidateProcessDataCache(Handle process,u32 address,u32 bytes){CHECK(process==CUR_PROCESS_HANDLE&&!drawDepth&&settingsLock==1);CHECK((address==0x1ff57ffe||address==0x1ff77ffe)?bytes==2:(address==0x1ff50a80||address==0x1ff70a80)&&bytes==640);++dspInvalidates;return 0;}
static void svcSleepThread(s64 time){if(deadlineTest){CHECK(time>0&&time<=10000000LL);++deadlineSleeps;}baseSleepThread(time);}
'''


TESTS = r'''
static void resetModel(bool newModel){reset();isN3DS=newModel;realJpeg=realDsp=deadlineTest=false;audioPackets=audioWireSize=0;}
static void failures(bool newModel){
for(unsigned stage=1;stage<=10;++stage){resetModel(newModel);failStage=stage;CHECK(RemotePlay_Start()<0&&!running&&!buffers&&!allocationLive);CHECK(frees==(stage!=1)&&socStops==(stage>=3));if(!newModel)CHECK(clockStateBefore==-1&&!clockRestores);}
for(unsigned stage=8;stage<=10;++stage){unsigned created=stage-7,limit=(1u<<(created+1))-2;for(unsigned mask=2;mask<=limit;mask+=2){resetModel(newModel);failStage=stage;joinMask=mask;CHECK(RemotePlay_Start()<0&&running&&buffers&&allocationLive&&!frees&&!socStops&&joinCalls==created);CHECK(RemotePlay_Start()<0);joinMask=0;CHECK(!RemotePlay_Stop()&&!running&&!buffers&&!allocationLive&&frees==1&&socStops==1&&joinCalls==created+(unsigned)__builtin_popcount(mask));}}
for(unsigned mask=2;mask<=30;mask+=2){resetModel(newModel);CHECK(!RemotePlay_Start()&&createCalls==4);joinMask=mask;CHECK(RemotePlay_Stop()<0&&running&&buffers&&allocationLive&&!frees&&!socStops&&joinCalls==4);joinMask=0;CHECK(!RemotePlay_Stop()&&!running&&!buffers&&!allocationLive&&frees==1&&socStops==1&&joinCalls==4+(unsigned)__builtin_popcount(mask));}
resetModel(newModel);CHECK(!RemotePlay_Start());socExitFailures=1;CHECK(RemotePlay_Stop()<0&&running&&socInitialized&&!frees&&buffers);CHECK(!RemotePlay_Stop()&&!running&&!socInitialized&&frees==1&&socStops==2);
}
static void profile(bool newModel){
resetModel(newModel);oldNextCaptureTick=123;CHECK(!RemotePlay_Start()&&oldNextCaptureTick==0&&running&&allocs==1&&createCalls==4);CHECK(status.clockMhz==(newModel?804:268)&&status.l2Cache==newModel);CHECK(clockStateBefore==(newModel?3:-1));
ControlConn connection={.sock=-1};unsigned clockBefore=clockRestores;CHECK(cpuIsFast()==newModel);raiseClock();checkLink(&connection);CHECK(clockRestores==clockBefore);if(!newModel)CHECK(clockStateBefore==-1&&status.clockMhz==268&&!status.l2Cache);
NtrHeader request={.magic=NTR_MAGIC,.type=0,.cmd=NTR_CMD_REMOTEPLAY};request.args[0]=0x102;request.args[1]=90;request.args[2]=QOS_MAX;handleRemotePlayRequest(&request,0x04030201);CHECK(takeSettings()&&enc.requestedQuality==90&&enc.screens[0].quality==90&&enc.screens[1].quality==90&&settingQuality==90&&status.quality==90);CHECK(settingMode==0x102&&settingQos==QOS_MAX&&enc.destIp==0x04030201&&status.priorityFactor==2&&status.topPriority);
Capture capture;copyTop=true;GPU_FB_TOP_LEFT_ADDR_1=0x28010000;CHECK(captureScreen(true,&capture)==newModel);GPU_FB_TOP_LEFT_ADDR_1=0x27fffffc;CHECK(captureScreen(true,&capture)==newModel);GPU_FB_TOP_LEFT_ADDR_1=0x20010000;CHECK(captureScreen(true,&capture));GPU_FB_TOP_LEFT_ADDR_1=0x18010000;
u64 from=svcGetSystemTick(),to=from+(u64)10000*TICKS_PER_US;encoderSpent(ENC_BUSY,from,to);CHECK(oldNextCaptureTick==(newModel?0:from+(u64)OLD_CAPTURE_INTERVAL_US*TICKS_PER_US));
if(!newModel){const u32 costs[]={0,1000,50000,100000,1000000};for(unsigned i=0;i<sizeof(costs)/sizeof(*costs);++i){from=to+(u64)1000000*TICKS_PER_US;to=from+(u64)costs[i]*TICKS_PER_US;encoderSpent(ENC_BUSY,from,to);u64 interval=from+(u64)OLD_CAPTURE_INTERVAL_US*TICKS_PER_US,rested=to+(to-from)*OLD_ENCODER_REST_FACTOR;CHECK(oldNextCaptureTick==(interval>rested?interval:rested));CHECK(oldNextCaptureTick-from>=(u64)OLD_CAPTURE_INTERVAL_US*TICKS_PER_US&&(oldNextCaptureTick-from)>=(to-from)*4);}u64 saved=oldNextCaptureTick;encoderSpent(ENC_WAIT_GAME,from,to);CHECK(oldNextCaptureTick==saved);encoderSpent(ENC_BUSY,to,from);CHECK(oldNextCaptureTick==saved);oldNextCaptureTick=UINT64_MAX-1;enc.lowCredit=2;deadlineTest=true;stopAfterSleeps=1;CHECK(pickScreen()==-1&&stopRequested&&deadlineSleeps>0);deadlineTest=false;stopRequested=false;}
CHECK(!RemotePlay_Stop()&&!buffers&&!allocationLive&&!running);if(!newModel)CHECK(clockStateBefore==-1&&!clockRestores);
}
static void encoderPaths(void){
for(unsigned mode=0;mode<4;++mode){resetModel(false);CHECK(!RemotePlay_Start());applySettings(0x04030201,0x100,90,QOS_DEFAULT,false);CHECK(takeSettings());copyTop=true;if(mode==0){GPU_FB_TOP_FMT=7|BIT(6);stopAfterSleeps=1;}else if(mode==1){jpegResult=0;stopOnEncode=true;}else if(mode==2){ScreenState *screen=&enc.screens[1];screen->haveHash=true;screen->lastHash=123;screen->lastFrameTick=svcGetSystemTick();screen->lastPa=0;stopOnCopy=true;}else{jpegResult=4;stopOnEncode=true;}encoderThreadMain();CHECK(stopRequested&&oldNextCaptureTick>0);if(mode<2)CHECK(status.skippedFrames==1);if(mode==2)CHECK(status.sameFrames==1);if(mode==3){CHECK(buffers->slots[0].size==4&&status.quality==90);stopOnEncode=false;stopRequested=false;stopOnSlotFreed=true;senderThreadMain();CHECK(stopRequested&&status.framesTop==1);}stopOnCopy=stopOnEncode=stopOnSlotFreed=false;CHECK(!RemotePlay_Stop());}
}
static void jpeg(bool newModel,bool top){
resetModel(newModel);CHECK(!RemotePlay_Start());realJpeg=true;applySettings(0x04030201,top?0x100:0,90,QOS_DEFAULT,false);CHECK(takeSettings());u32 rows=top?400:320;u8 *pixels=(u8*)(uintptr_t)(top?0x1f010000:0x1f210000);for(u32 row=0;row<rows;++row)for(u32 col=0;col<240;++col){u32 offset=row*720+col*3;pixels[offset]=(u8)(((col+row)/40)*10);pixels[offset+1]=(u8)((row/40)*20);pixels[offset+2]=(u8)((col/24)*20);}if(top){GPU_FB_TOP_FMT=RPJPEG_FMT_BGR8|BIT(6);GPU_FB_TOP_STRIDE=720;}else{GPU_FB_BOTTOM_FMT=RPJPEG_FMT_BGR8;GPU_FB_BOTTOM_STRIDE=720;}Capture capture;copyTop=top;CHECK(captureScreen(top,&capture)&&capture.rows==rows&&capture.pitch==720&&capture.format==RPJPEG_FMT_BGR8);CHECK(!memcmp(capture.pixels,pixels,rows*720));JpegSlot *slot=&buffers->slots[0];u32 bytes=encodeCapture(slot,&capture,&enc.screens[top]);CHECK(bytes>1000&&bytes<JPEG_SLOT_CAP&&enc.jpeg.quality==90&&enc.screens[top].quality==90&&enc.requestedQuality==90);char name[40];snprintf(name,sizeof(name),"%s-%s-q90.jpg",newModel?"new":"old",top?"top":"bottom");FILE *file=fopen(name,"wb");CHECK(file&&fwrite(slot->data,1,bytes,file)==bytes&&!fclose(file));realJpeg=false;CHECK(!RemotePlay_Stop());
}
static void audio(bool newModel){
resetModel(newModel);CHECK(!RemotePlay_Start());realDsp=true;*(vu16*)(uintptr_t)0x1ff57ffe=100;*(vu16*)(uintptr_t)0x1ff77ffe=101;memset((void*)(uintptr_t)0x1ff50a80,0x12,640);memset((void*)(uintptr_t)0x1ff70a80,0x34,640);audioSock=openAudioSocket();CHECK(audioSock==42);startPlayer(0x04030201,htons(8002));aud.lastHelloTick=svcGetSystemTick();*(vu16*)(uintptr_t)0x1ff57ffe=102;*(vu16*)(uintptr_t)0x1ff77ffe=103;stopAfterSleeps=1;audioThreadMain();CHECK(stopRequested&&audioPackets==1&&status.audioFrames==2&&audioWireSize==1292&&dspInvalidates>0);CHECK(!memcmp(audioWire,"L2AU\x01\x00\x02\x00\x66\x00\x00\x00",12));CHECK(audioDestination.sin_addr.s_addr==0x04030201&&audioDestination.sin_port==htons(8002));for(u32 i=0;i<640;++i)CHECK(audioWire[12+i]==0x12&&audioWire[652+i]==0x34);realDsp=false;CHECK(!RemotePlay_Stop());
}
int main(void){CHECK(OLD_CAPTURE_INTERVAL_US==200000&&OLD_ENCODER_REST_FACTOR==3&&OLD_ENCODER_THREAD_PRIO==0x3F&&OLD_SENDER_THREAD_PRIO==0x3E&&AUDIO_THREAD_PRIO==0x2E&&VIDEO_THREAD_CORE==3);CHECK(commonRegressionMain()==0);mapAt(0x1ff50000,0x30000);for(unsigned newModel=0;newModel<2;++newModel){failures(newModel!=0);profile(newModel!=0);jpeg(newModel!=0,true);jpeg(newModel!=0,false);audio(newModel!=0);}encoderPaths();printf("old3ds %u %u %u %u %u\n",checks,statusAccesses,historyAccesses,dspInvalidates,deadlineSleeps);return 0;}
'''


def replace_function(module, source, name, replacement):
    old = module.extract(source, name)
    assert source.count(old) == 1, name
    return source.replace(old, replacement + '\n')


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / '3ds').mkdir(exist_ok=True)
    (OUT / '3ds/types.h').write_text('#pragma once\n', encoding='utf-8')
    spec = importlib.util.spec_from_file_location('streaming_platform_shared', ROOT / 'tests/streaming_platform.py')
    shared = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(shared)
    source = (ROS / 'source/remote_play.c').read_text(encoding='utf-8')
    draw = (ROS / 'source/draw.c').read_text(encoding='utf-8')
    dsp = (ROS / 'source/dsp_audio.c').read_text(encoding='utf-8')
    actual = re.sub(r'^#include[^\n]*\n', '', source, flags=re.M)
    actual = actual.replace('static RemotePlayStatus status;', 'static RemotePlayStatus rawStatus;\n#define status (*statusAccess())')
    actual = actual.replace('statusIncrement(&status.', 'statusIncrement(&rawStatus.')
    actual = actual.replace('++*counter;', '++*statusCounterAccess(counter);')
    actual = actual.replace('buffers->encHistory', '((EncSecond *)historyAccess(buffers->encHistory))')
    actual = actual.replace('buffers->sndHistory', '((SndSecond *)historyAccess(buffers->sndHistory))')
    guards = '\n#undef status\nstatic RemotePlayStatus *statusAccess(void){CHECK(settingsLock==0);++statusAccesses;return &rawStatus;}\nstatic u32 *statusCounterAccess(u32 *counter){CHECK(settingsLock==0&&(uintptr_t)counter>=(uintptr_t)&rawStatus&&(uintptr_t)counter+sizeof(*counter)<=(uintptr_t)&rawStatus+sizeof(rawStatus));++statusAccesses;return counter;}\nstatic void *historyAccess(void *history){CHECK(settingsLock==0);++historyAccesses;return history;}\n#define status rawStatus\n'
    selected = '\n'.join(shared.extract(draw, name) for name in ['Draw_IsFramebufferActive', 'Draw_SetupFramebuffer', 'Draw_RestoreFramebuffer', 'Draw_GetCurrentFramebufferAddress'])
    prefix = shared.PREFIX.replace('static void svcSleepThread(', 'static void baseSleepThread(')
    suffix = shared.SUFFIX.replace('int main(void)', 'static int commonRegressionMain(void)')
    suffix = replace_function(shared, suffix, 'RpJpeg_SetQuality', 'void RpJpeg_SetQuality(RpJpeg *jpeg,int quality){RealRpJpeg_SetQuality(jpeg,quality);}')
    encode = shared.extract(suffix, 'RpJpeg_Encode').replace('return jpegResult;', 'return realJpeg?RealRpJpeg_Encode(jpeg,out,outCap,src,srcCap,width,rows,stride,rowStep,format):jpegResult;')
    suffix = replace_function(shared, suffix, 'RpJpeg_Encode', encode.rstrip())
    copy = shared.extract(suffix, 'RpCopy_Rows').replace('++copyCalls;', '++copyCalls;if(realJpeg)for(u32 row=0;row<rows;++row)memcpy((u8*)dst+row*rowBytes,(const u8*)src+row*stride*rowStep,rowBytes);')
    suffix = replace_function(shared, suffix, 'RpCopy_Rows', copy.rstrip())
    suffix = replace_function(shared, suffix, 'DspAudio_ReaderInit', 'void DspAudio_ReaderInit(DspAudioReader *reader){CHECK(!drawDepth&&settingsLock==1&&reader);++dspInits;if(realDsp)RealDspAudio_ReaderInit(reader);else memset(reader,0,sizeof(*reader));}')
    suffix = replace_function(shared, suffix, 'DspAudio_Poll', 'u32 DspAudio_Poll(DspAudioReader *reader,DspAudioBlock out[2]){CHECK(!drawDepth&&settingsLock==1&&allocationLive&&reader==&aud.reader&&out);++dspPolls;return realDsp?RealDspAudio_Poll(reader,out):0;}')
    send = shared.extract(suffix, 'socSendto').replace('(void)sock;', 'if(realDsp&&sock==audioSock){CHECK(bytes<=sizeof(audioWire));memcpy(audioWire,data,bytes);audioWireSize=(u32)bytes;memcpy(&audioDestination,address,sizeof(audioDestination));++audioPackets;}')
    suffix = replace_function(shared, suffix, 'socSendto', send.rstrip())
    actual_dsp = re.sub(r'^#include[^\n]*\n', '', dsp, flags=re.M)
    actual_dsp = re.sub(r'\bDspAudio_ReaderInit\b', 'RealDspAudio_ReaderInit', actual_dsp)
    actual_dsp = re.sub(r'\bDspAudio_Poll\b', 'RealDspAudio_Poll', actual_dsp)
    fixture = OUT / 'fixture.c'
    fixture.write_text(prefix + EXTRA + actual_dsp + selected + actual + guards + suffix + TESTS, encoding='utf-8')
    compiler = os.environ.get('CC', 'cc')
    if not shutil.which(compiler) and os.name == 'nt':
        compiler = 'C:/msys64/ucrt64/bin/gcc.exe'
    environment = dict(os.environ)
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    jpeg_object = OUT / 'jpeg.o'
    command = [compiler, '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Werror', '-DRpJpeg_SetQuality=RealRpJpeg_SetQuality', '-DRpJpeg_BytesPerPixel=RealRpJpeg_BytesPerPixel', '-DRpJpeg_Encode=RealRpJpeg_Encode', '-I' + str(ROS / 'include'), '-c', str(ROS / 'source/rp_jpeg.c'), '-o', str(jpeg_object)]
    result = subprocess.run(command, env=environment, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    executable = OUT / ('old3ds.exe' if os.name == 'nt' else 'old3ds')
    command = [compiler, '-std=gnu11', '-O1', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-unused-variable', '-Wno-pointer-to-int-cast', '-Wno-int-to-pointer-cast', '-I' + str(OUT), '-I' + str(ROS / 'include'), str(fixture), str(jpeg_object), '-o', str(executable)]
    result = subprocess.run(command, env=environment, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    result = subprocess.run([str(executable)], cwd=OUT, env=environment, capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr
    marker, checks, status_accesses, history_accesses, dsp_invalidates, deadline_sleeps = result.stdout.splitlines()[-1].split()
    assert marker == 'old3ds'
    images = []
    for screen, rows in [('top', 400), ('bottom', 320)]:
        old = (OUT / ('old-' + screen + '-q90.jpg')).read_bytes()
        new = (OUT / ('new-' + screen + '-q90.jpg')).read_bytes()
        assert old == new, screen + ' differs by hardware model'
        image = Image.open(OUT / ('old-' + screen + '-q90.jpg'))
        assert image.size == (240, rows) and image.layer[0][1:3] == (1, 1)
        pixels = image.convert('RGB').tobytes()
        reference = ((col // 24 * 20, row // 40 * 20, (col + row) // 40 * 10) for row in range(rows) for col in range(240))
        error = sum(abs(value - expected) for value, expected in zip(pixels, (value for pixel in reference for value in pixel))) / (240 * rows * 3)
        assert error < 3, (screen, error)
        images.append(dict(screen=screen, native_width=240, native_height=rows, quality=90, sampling='444', old_new_identical=True, jpeg_sha256=hashlib.sha256(old).hexdigest(), mean_absolute_rgb_error=error))
    result = subprocess.run([sys.executable, str(ROOT / 'tests/streaming_audio.py')], cwd=ROOT, env=environment, capture_output=True, text=True, timeout=45)
    assert result.returncode == 0, result.stdout + result.stderr
    audio_report = json.loads((ROOT / 'build/streaming-audio-tests/proof.json').read_text(encoding='utf-8'))
    assert audio_report['result'] == 'pass'
    report = dict(result='pass', checks=int(checks), common_new_regression_executed=True, models=['Old3DS', 'New3DS'], worker_threads=4, failure_stages_per_model=10, all_join_failure_masks_per_model=True, old_core=1, old_encoder_priority=63, old_sender_priority=62, old_audio_priority=46, old_max_aggregate_fps=5, old_encoder_rest_factor=3, bounded_cancellable_deadline_sleeps=int(deadline_sleeps), old_clock_queries_or_changes_allowed=False, original_quality90_and_native_pixels_preserved=images, actual_dsp_reader_model_runs=2, actual_dsp_cache_invalidations=int(dsp_invalidates), actual_l2au_model_runs=2, locked_status_accesses=int(status_accesses), locked_history_accesses=int(history_accesses), shared_audio_checks=audio_report['checks'], source_sha256={name: hashlib.sha256((ROS / name).read_bytes()).hexdigest() for name in ['source/remote_play.c', 'source/dsp_audio.c', 'source/rp_jpeg.c', 'source/draw.c', 'rosalina.rsf']}, hardware_tested=False, boundary='Actual candidate capture backend, Old profile, JPEG encoder, DSP reader and audio packet output execute with shared platform fixture providers; native host JPEG decode validates both original-size screens. OS allocation, thread scheduler, cache services, VRAM/DSP memory, sockets and wireless throughput are host simulations. No ARM/full firmware build or physical Old/New3DS performance claim.')
    (OUT / 'proof.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
