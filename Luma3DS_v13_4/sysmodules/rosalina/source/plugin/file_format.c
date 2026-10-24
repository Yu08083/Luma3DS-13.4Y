#include "plugin/file_format.h"

static uint32_t formatWord(const uint8_t *data, uint32_t offset)
{
    return (uint32_t)data[offset] | (uint32_t)data[offset + 1] << 8 | (uint32_t)data[offset + 2] << 16 | (uint32_t)data[offset + 3] << 24;
}

static bool formatSpan(uint32_t offset, uint32_t length, uint32_t fileSize)
{
    return offset >= 0x94 && offset <= fileSize && length <= fileSize - offset;
}

bool PluginFile_Validate(const void *header, uint32_t fileSize, uint32_t memorySize, uint32_t pluginHeaderSize, PluginFileLayout *layout)
{
    if(!header || !layout || fileSize < 0x94 || pluginHeaderSize > 0x1000) return false;
    const uint8_t *data = header;
    if(formatWord(data, 0) != 0x24584733 || formatWord(data, 4) != 0x32303030 || formatWord(data, 12)) return false;
    uint32_t flags = formatWord(data, 0x30);
    if(flags & ~0x3FFu || ((flags >> 4) & 3) == 3) return false;
    uint32_t codeOffset = formatWord(data, 0x58), codeSize = formatWord(data, 0x64);
    uint32_t rodataOffset = formatWord(data, 0x5C), rodataSize = formatWord(data, 0x68);
    uint32_t dataOffset = formatWord(data, 0x60), dataSize = formatWord(data, 0x6C);
    uint32_t bssSize = formatWord(data, 0x70);
    if(!codeSize || (codeOffset | codeSize | rodataSize | dataSize) & 3) return false;
    if(!formatSpan(codeOffset, codeSize, fileSize)) return false;
    uint32_t rodataStart = codeOffset + codeSize;
    if(rodataSize ? rodataOffset != rodataStart || !formatSpan(rodataOffset, rodataSize, fileSize) :
       rodataOffset && rodataOffset != rodataStart) return false;
    uint32_t dataStart = rodataStart + rodataSize;
    if(dataSize ? dataOffset != dataStart || !formatSpan(dataOffset, dataSize, fileSize) :
       dataOffset && dataOffset != dataStart) return false;
    uint64_t image = (uint64_t)codeSize + rodataSize + dataSize;
    uint64_t executable = (pluginHeaderSize + image + bssSize + 0x1000) & ~0xFFFULL;
    uint64_t extent = (uint64_t)codeOffset + image;
    if(executable >= memorySize) return false;
    for(uint32_t field = 0x10; field < 0x30; field += 8)
    {
        uint32_t length = formatWord(data, field), offset = formatWord(data, field + 4);
        if(length && !formatSpan(offset, length, fileSize)) return false;
        if(field < 0x20 && length > 256) return false;
        if(length && (uint64_t)offset + length > extent) extent = (uint64_t)offset + length;
    }
    uint32_t targets = formatWord(data, 0x80), targetOffset = formatWord(data, 0x84);
    if(targets > 128 || (targets && !formatSpan(targetOffset, targets * 4, fileSize))) return false;
    if(targets && (uint64_t)targetOffset + targets * 4 > extent) extent = (uint64_t)targetOffset + targets * 4;
    if(flags & 1)
    {
        uint32_t offset = formatWord(data, 0x74);
        if(!formatSpan(offset, 4, fileSize) || offset >= codeOffset) return false;
    }
    if(flags & 2)
        for(uint32_t field = 0x78; field <= 0x7C; field += 4)
        {
            uint32_t offset = formatWord(data, field);
            if(!formatSpan(offset, 4, fileSize) || offset >= codeOffset) return false;
        }
    layout->imageSize = image;
    layout->executableSize = executable;
    uint64_t snapshot = (extent + 15) & ~15ULL;
    if(snapshot > memorySize) return false;
    layout->snapshotSize = snapshot;
    return true;
}

bool PluginFile_ValidateStrings(const void *file, uint32_t fileSize)
{
    if(!file || fileSize < 0x94) return false;
    const uint8_t *data = file;
    for(uint32_t field = 0x10; field < 0x30; field += 8)
    {
        uint32_t length = formatWord(data, field), offset = formatWord(data, field + 4);
        if(!length) continue;
        if(!formatSpan(offset, length, fileSize)) return false;
        bool terminated = false;
        for(uint32_t i = 0; i < length; ++i) if(!data[offset + i]) { terminated = true; break; }
        if(!terminated) return false;
    }
    uint32_t flags = formatWord(data, 0x30), codeOffset = formatWord(data, 0x58);
    if(codeOffset < 0x94 || codeOffset > fileSize) return false;
    for(uint32_t field = 0x74; field <= 0x7C; field += 4)
    {
        if(field == 0x74 ? !(flags & 1) : !(flags & 2)) continue;
        uint32_t offset = formatWord(data, field);
        if(!formatSpan(offset, 4, fileSize) || offset >= codeOffset) return false;
        bool terminated = false;
        for(uint32_t i = 0; i < 32 && offset <= codeOffset && codeOffset - offset >= 4; ++i, offset += 4)
            if(formatWord(data, offset) == 0xE320F000u) { terminated = i != 0; break; }
        if(!terminated) return false;
    }
    return true;
}
