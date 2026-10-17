#include <3ds.h>
#include <string.h>
#include "plugin.h"
#include "ifile.h"
#include "utils.h"
#include "plugin/payload_vm.h"

u32 g_loadExeArgs[4];
static PluginPayload loadProgram;
static char authorText[257], titleText[257];
static u32 supportedTitles[128];

static Result invalidPlugin(void)
{
    return MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_LDR, RD_INVALID_ADDRESS);
}

static inline u32 invertEndianness(u32 value)
{
    return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
}

Result Check_3gx_Magic(IFile *file)
{
    u64 magic = 0, total = 0;
    file->pos = 0;
    Result result = IFile_Read(file, &total, &magic, sizeof(magic));
    if(R_FAILED(result)) return result;
    if(total != sizeof(magic)) return invalidPlugin();
    if((u32)magic != (u32)_3GX_MAGIC) return MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_LDR, 1);
    u32 version = invertEndianness((u32)(magic >> 32));
    u32 expected = invertEndianness((u32)(_3GX_MAGIC >> 32));
    if(version != expected) return MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_LDR, version < expected ? 2 : 3);
    return 0;
}

Result Read_3gx_Header(IFile *file, _3gx_Header *header)
{
    u64 total = 0;
    memset(header, 0, sizeof(*header));
    file->pos = 0;
    Result result = IFile_Read(file, &total, header, sizeof(*header));
    return R_FAILED(result) ? result : total == sizeof(*header) ? 0 : invalidPlugin();
}

Result Read_3gx_ParseHeader(const u8 *file, _3gx_Header *header)
{
    memset(authorText, 0, sizeof(authorText));
    memset(titleText, 0, sizeof(titleText));
    if(header->infos.authorLen) memcpy(authorText, file + (u32)header->infos.authorMsg, header->infos.authorLen);
    if(header->infos.titleLen) memcpy(titleText, file + (u32)header->infos.titleMsg, header->infos.titleLen);
    if(header->targets.count) memcpy(supportedTitles, file + (u32)header->targets.titles, header->targets.count * sizeof(u32));
    header->infos.authorMsg = authorText;
    header->infos.titleMsg = titleText;
    header->infos.summaryLen = header->infos.descriptionLen = 0;
    header->infos.summaryMsg = header->infos.descriptionMsg = NULL;
    header->targets.titles = supportedTitles;
    return 0;
}

Result Read_3gx_LoadSegments(const u8 *file, _3gx_Header *header, void *destination)
{
    _3gx_Executable *executable = &header->executable;
    PluginLoaderContext *context = &PluginLoaderCtx;
    u32 size = executable->codeSize + executable->rodataSize + executable->dataSize;
    if(!context->isExeLoadFunctionset) return MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_LDR, RD_NO_DATA);
    if(destination != file + executable->codeOffset) memmove(destination, file + executable->codeOffset, size);
    uint32_t checksum = 0;
    bool valid = PluginPayload_Run(&loadProgram, destination, size, (const uint32_t *)g_loadExeArgs, &checksum);
    Reset_3gx_LoadParams();
    return valid && checksum == context->exeLoadChecksum ? 0 : invalidPlugin();
}

static void readPayload(const u8 *file, u32 offset, u32 codeOffset, u32 output[32])
{
    memset(output, 0, 128);
    u32 length = codeOffset - offset;
    if(length > 128) length = 128;
    memcpy(output, file + offset, length);
}

Result Read_3gx_EmbeddedPayloads(const u8 *file, _3gx_Header *header)
{
    u32 program[32], arguments[4];
    PluginLoaderContext *context = &PluginLoaderCtx;
    Result result = 0;
    if(header->infos.embeddedExeLoadFunc)
    {
        readPayload(file, header->executable.exeLoadFuncOffset, header->executable.codeOffset, program);
        memcpy(arguments, header->infos.builtInLoadExeArgs, sizeof(arguments));
        result = Set_3gx_LoadParams(program, arguments);
        if(!result) context->isExeLoadFunctionset = true;
    }
    if(!result && header->infos.embeddedSwapSaveLoadFunc)
    {
        readPayload(file, header->executable.swapSaveFuncOffset, header->executable.codeOffset, program);
        memcpy(arguments, header->infos.builtInSwapSaveLoadArgs, sizeof(arguments));
        result = MemoryBlock__SetSwapSettings(program, false, arguments);
        if(!result)
        {
            readPayload(file, header->executable.swapLoadFuncOffset, header->executable.codeOffset, program);
            result = MemoryBlock__SetSwapSettings(program, true, arguments);
        }
        if(!result) context->isSwapFunctionset = true;
    }
    return result;
}

Result Set_3gx_LoadParams(u32 *code, u32 *arguments)
{
    PluginPayload candidate;
    if(!PluginPayload_Set(&candidate, (const uint32_t *)code)) return invalidPlugin();
    loadProgram = candidate;
    memcpy(g_loadExeArgs, arguments, sizeof(g_loadExeArgs));
    return 0;
}

void Reset_3gx_LoadParams(void)
{
    memset(&loadProgram, 0, sizeof(loadProgram));
    memset(g_loadExeArgs, 0, sizeof(g_loadExeArgs));
    PluginLoaderCtx.isExeLoadFunctionset = false;
}
