#include "plugin/legacy.h"
#include <string.h>

#define LEGACY_BASE 0x07000100u
#define LEGACY_EXTENSION 360u

typedef struct {
    uint32_t word;
    uint32_t ignore;
} LegacyPattern;

typedef struct {
    uint32_t code;
    uint32_t size;
    uint32_t output;
    uint32_t end;
    uint32_t loop;
    uint32_t kernel;
    uint32_t arbitrate;
    uint32_t connect;
} LegacyPlan;

static uint32_t legacyWord(const uint8_t *data, uint32_t offset)
{
    return (uint32_t)data[offset] | (uint32_t)data[offset + 1] << 8 |
           (uint32_t)data[offset + 2] << 16 | (uint32_t)data[offset + 3] << 24;
}

static void legacyPut(uint8_t *data, uint32_t offset, uint32_t value)
{
    for (uint32_t i = 0; i < 4; ++i) data[offset + i] = value >> (i * 8);
}

static bool legacySpan(uint32_t offset, uint32_t size, uint32_t fileSize)
{
    return offset >= PLUGIN_LEGACY_HEADER_SIZE && offset <= fileSize && size <= fileSize - offset;
}

static uint32_t legacyOutput(const uint8_t *data)
{
    uint32_t size = 0x94 + legacyWord(data, 0x14) + legacyWord(data, 0x1C);
    size = (size + 3) & ~3u;
    size += legacyWord(data, 0x34) * 4 + 12;
    return (size + 15) & ~15u;
}

bool PluginLegacy_ValidateHeader(const void *header, uint32_t fileSize, uint32_t memorySize)
{
    if (!header || fileSize < PLUGIN_LEGACY_HEADER_SIZE || fileSize > memorySize) return false;
    const uint8_t *data = header;
    if (legacyWord(data, 0) != 0x24584733 || legacyWord(data, 4) != 0x31303030) return false;
    uint32_t code = legacyWord(data, 0x10), size = legacyWord(data, 0xC);
    if (!size || (code | size) & 3 || !legacySpan(code, size, fileSize)) return false;
    for (uint32_t field = 0x14; field <= 0x2C; field += 8)
    {
        uint32_t length = legacyWord(data, field), offset = legacyWord(data, field + 4);
        if (length && !legacySpan(offset, length, fileSize)) return false;
        if (field < 0x24 && length > 256) return false;
    }
    uint32_t targets = legacyWord(data, 0x34);
    if (targets > 128 || (targets && !legacySpan(legacyWord(data, 0x38), targets * 4, fileSize))) return false;
    uint64_t image = (uint64_t)size + LEGACY_EXTENSION;
    uint64_t executable = (0x100 + image + 0x1000) & ~UINT64_C(0xFFF);
    return (uint64_t)legacyOutput(data) + image <= memorySize && executable < memorySize;
}

static bool legacyFind(const uint8_t *code, uint32_t size, const LegacyPattern *pattern, uint32_t count, uint32_t *offset)
{
    if (size < count * 4) return false;
    uint32_t matches = 0;
    for (uint32_t i = 0; i <= size - count * 4; i += 4)
    {
        bool same = true;
        for (uint32_t j = 0; j < count; ++j)
            if ((legacyWord(code, i + j * 4) & ~pattern[j].ignore) != (pattern[j].word & ~pattern[j].ignore))
            {
                same = false;
                break;
            }
        if (same)
        {
            *offset = i;
            if (++matches > 1) return false;
        }
    }
    return matches == 1;
}

static bool legacyBranchTarget(uint32_t offset, uint32_t instruction, uint32_t size, uint32_t *target)
{
    if ((instruction & 0xFF000000u) != 0xEB000000u) return false;
    int32_t displacement = (int32_t)(instruction << 8) >> 6;
    int64_t destination = (int64_t)offset + 8 + displacement;
    if (destination < 0 || destination >= size || destination & 3) return false;
    *target = destination;
    return true;
}

static bool legacyPlan(const uint8_t *data, uint32_t fileSize, uint32_t memorySize, LegacyPlan *plan)
{
    if (!PluginLegacy_ValidateHeader(data, fileSize, memorySize)) return false;
    for (uint32_t field = 0x14; field <= 0x2C; field += 8)
    {
        uint32_t length = legacyWord(data, field), offset = legacyWord(data, field + 4);
        if (length && !memchr(data + offset, 0, length)) return false;
    }
    static const LegacyPattern loop[] = {
        {0xE59D3014, 0}, {0xE1CD80F0, 0}, {0xE58D3038, 0}, {0xE59D3018, 0},
        {0xE3A02002, 0}, {0xE58D303C, 0}, {0xE1A01004, 0}, {0xE3A03000, 0},
        {0xE28D0020, 0}, {0xE58D5020, 0}, {0xEB000000, 0xFFFFFF}, {0xE59D3020, 0},
        {0xE3530000, 0}, {0x1A00000B, 0}, {0xEB000000, 0xFFFFFF}, {0xEAFFFFEF, 0}
    };
    static const LegacyPattern arbitrate[] = {
        {0xE92D0030, 0}, {0xE59D4008, 0}, {0xE59D500C, 0},
        {0xEF000022, 0}, {0xE8BD0030, 0}, {0xE12FFF1E, 0}
    };
    static const LegacyPattern connect[] = {
        {0xE52D0004, 0}, {0xEF00002D, 0}, {0xE49D3004, 0},
        {0xE5831000, 0}, {0xE12FFF1E, 0}
    };
    plan->code = legacyWord(data, 0x10);
    plan->size = legacyWord(data, 0xC);
    plan->output = legacyOutput(data);
    plan->end = plan->output + plan->size + LEGACY_EXTENSION;
    const uint8_t *code = data + plan->code;
    if (!legacyFind(code, plan->size, loop, sizeof(loop) / sizeof(*loop), &plan->loop) ||
        !legacyFind(code, plan->size, arbitrate, sizeof(arbitrate) / sizeof(*arbitrate), &plan->arbitrate) ||
        !legacyFind(code, plan->size, connect, sizeof(connect) / sizeof(*connect), &plan->connect)) return false;
    if (plan->loop < 181 * 4 || plan->loop > plan->size - 48 * 4) return false;
    uint32_t ignored;
    if (!legacyBranchTarget(plan->loop - 181 * 4, legacyWord(code, plan->loop - 181 * 4), plan->size, &plan->kernel) ||
        !legacyBranchTarget(plan->loop + 14 * 4, legacyWord(code, plan->loop + 14 * 4), plan->size, &ignored)) return false;
    return legacyWord(code, plan->loop + 44 * 4) == 0xE59D001C &&
           (legacyWord(code, plan->loop + 45 * 4) & 0xFF000000u) == 0xEB000000u &&
           legacyWord(code, plan->loop + 46 * 4) == 0xE5960008 &&
           (legacyWord(code, plan->loop + 47 * 4) & 0xFF000000u) == 0xEB000000u;
}

bool PluginLegacy_Validate(const void *file, uint32_t fileSize, uint32_t memorySize)
{
    LegacyPlan plan;
    return legacyPlan(file, fileSize, memorySize, &plan);
}

static uint32_t legacyBranch(uint32_t from, uint32_t to)
{
    return 0xEB000000u | (((to - from - 8) >> 2) & 0xFFFFFFu);
}

static void legacyWords(uint8_t *image, uint32_t offset, const uint32_t *words, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) legacyPut(image, offset + i * 4, words[i]);
}

bool PluginLegacy_Convert(void *file, uint32_t fileSize, uint32_t memorySize, uint32_t *convertedSize)
{
    LegacyPlan plan;
    if (!convertedSize || !legacyPlan(file, fileSize, memorySize, &plan)) return false;
    uint8_t *data = file;
    uint8_t metadata[1024], header[0x94] = {0};
    uint32_t author = legacyWord(data, 0x14), title = legacyWord(data, 0x1C), targets = legacyWord(data, 0x34);
    uint32_t targetOffset = (author + title + 3) & ~3u;
    memset(metadata, 0, sizeof(metadata));
    if (author) memcpy(metadata, data + legacyWord(data, 0x18), author);
    if (title) memcpy(metadata + author, data + legacyWord(data, 0x20), title);
    if (targets) memcpy(metadata + targetOffset, data + legacyWord(data, 0x38), targets * 4);
    legacyPut(header, 0, 0x24584733);
    legacyPut(header, 4, 0x32303030);
    legacyPut(header, 8, legacyWord(data, 8));
    legacyPut(header, 0x10, author);
    legacyPut(header, 0x14, 0x94);
    legacyPut(header, 0x18, title);
    legacyPut(header, 0x1C, 0x94 + author);
    legacyPut(header, 0x30, 3);
    legacyPut(header, 0x58, plan.output);
    legacyPut(header, 0x5C, plan.end);
    legacyPut(header, 0x60, plan.end);
    legacyPut(header, 0x64, plan.size + LEGACY_EXTENSION);
    uint32_t payload = 0x94 + targetOffset + targets * 4;
    legacyPut(header, 0x74, payload);
    legacyPut(header, 0x78, payload);
    legacyPut(header, 0x7C, payload);
    legacyPut(header, 0x80, targets);
    legacyPut(header, 0x84, 0x94 + targetOffset);
    memmove(data + plan.output, data + plan.code, plan.size);
    memcpy(data, header, sizeof(header));
    memset(data + sizeof(header), 0, plan.output - sizeof(header));
    memcpy(data + sizeof(header), metadata, targetOffset + targets * 4);
    legacyPut(data, payload, 0xE3A00000);
    legacyPut(data, payload + 4, 0xE12FFF1E);
    legacyPut(data, payload + 8, 0xE320F000);
    uint8_t *image = data + plan.output;
    uint32_t storage = plan.size, fetch = storage + 16, reply = fetch + 36;
    uint32_t init = reply + 184, initEx = init + 88, convert = initEx + 20;
    static const uint32_t fetchCode[] = {
        0xE59F3018, 0xE5932008, 0xE1920F9F, 0xE1823F90, 0xE6EF3073,
        0xE3530000, 0x1AFFFFFA, 0xE12FFF1E, 0
    };
    static const uint32_t replyCode[] = {
        0xE92D40F7, 0xE3A03000, 0xE1A04000, 0xE59F50A0, 0xE595100C, 0xE1912F9F,
        0xE1812F93, 0xE6EF2072, 0xE3520000, 0x1AFFFFFA, 0xE3540002, 0xCA000007,
        0xE5951008, 0xE1913F9F, 0xE1813F92, 0xE6EF3073, 0xE3530000, 0x1AFFFFFA,
        0xE28DD00C, 0xE8BD80F0, 0xE3A06000, 0xE3A07000, 0xE3A03001, 0xE1CD60F0,
        0xE5950004, 0, 0xE3540003, 0x1A00000D, 0xE3E02000, 0xE5951008,
        0xE1913F9F, 0xE1813F92, 0xE6EF3073, 0xE3530000, 0x1AFFFFFA, 0xE3A06000,
        0xE3A07000, 0xE3A02001, 0xE1CD60F0, 0xE5950004, 0, 0xEAFFFFE7,
        0xE3540004, 0x1AFFFFE5, 0xEF000009, 0
    };
    static const uint32_t initCode[] = {
        0xE92D4070, 0xEE1D5F70, 0xE59F403C, 0xE28F103C, 0xE1A00004, 0,
        0xE3A03809, 0xE5940000, 0xE5853080, 0xEF000032, 0xE3500000,
        0xA595308C, 0xA5843004, 0xE3A03407, 0xE5932018, 0xE5842008,
        0xE593301C, 0xE584300C, 0xE8BD8070, 0, 0x3A676C70, 0x0072646C
    };
    static const uint32_t initExCode[] = {0xE52DE004, 0, 0, 0, 0xE49DF004};
    static const uint32_t convertCode[] = {0xE3A03407, 0xE5932014, 0xE5832018, 0xE12FFF1E};
    memset(image + storage, 0, 16);
    legacyWords(image, fetch, fetchCode, sizeof(fetchCode) / 4);
    legacyWords(image, reply, replyCode, sizeof(replyCode) / 4);
    legacyWords(image, init, initCode, sizeof(initCode) / 4);
    legacyWords(image, initEx, initExCode, sizeof(initExCode) / 4);
    legacyWords(image, convert, convertCode, sizeof(convertCode) / 4);
    legacyPut(image, fetch + 8 * 4, LEGACY_BASE + storage);
    legacyPut(image, reply + 25 * 4, legacyBranch(reply + 25 * 4, plan.arbitrate));
    legacyPut(image, reply + 40 * 4, legacyBranch(reply + 40 * 4, plan.arbitrate));
    legacyPut(image, reply + 45 * 4, LEGACY_BASE + storage);
    legacyPut(image, init + 5 * 4, legacyBranch(init + 5 * 4, plan.connect));
    legacyPut(image, init + 19 * 4, LEGACY_BASE + storage);
    legacyPut(image, initEx + 4, legacyBranch(initEx + 4, plan.kernel));
    legacyPut(image, initEx + 8, legacyBranch(initEx + 8, init));
    legacyPut(image, initEx + 12, legacyBranch(initEx + 12, convert));
    legacyPut(image, plan.loop - 181 * 4, legacyBranch(plan.loop - 181 * 4, initEx));
    uint32_t replacement[] = {
        0xE59D0014, 0xE59F20AC, 0xE3A03000, 0xEF000024, 0xE59F50A4,
        0xE1500005, 0x1A000006, legacyBranch(plan.loop + 7 * 4, fetch),
        0xE3500004, 0x0A00000F, legacyBranch(plan.loop + 10 * 4, reply),
        0xEAFFFFF3, 0xE320F000, 0xE320F000
    };
    legacyWords(image, plan.loop, replacement, sizeof(replacement) / 4);
    uint32_t exitOffset = plan.loop + 44 * 4;
    const uint32_t exitCode[] = {0xE3A00004, legacyBranch(exitOffset + 4, reply), 0x05F5E100, 0x09401BFE};
    legacyWords(image, exitOffset, exitCode, sizeof(exitCode) / 4);
    *convertedSize = plan.end;
    return true;
}
