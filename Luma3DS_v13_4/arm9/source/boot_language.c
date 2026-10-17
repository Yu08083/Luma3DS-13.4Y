#include "boot_language.h"
#include "fs.h"
#include "memory.h"

static bool japanese = true;

void BootLanguage_Load(void)
{
    japanese = true;
    char value[4] = {0};
    if (fileRead(value, "lang.cfg", sizeof(value)) >= 1)
    {
        if (value[0] == '0') japanese = false;
        if (value[0] == '1') japanese = true;
    }
}

bool BootLanguage_IsJapanese(void)
{
    return japanese;
}

bool BootLanguage_Toggle(void)
{
    const char value[2] = { japanese ? '0' : '1', '\n' };
    if (!fileWrite(value, "lang.cfg", sizeof(value)))
        return false;
    japanese = !japanese;
    return true;
}

const char *BootLanguage_Selector(void)
{
    return japanese ? "X: 言語  English / [日本語]" : "X: Language  [English] / 日本語";
}

const char *BootLanguage_Text(const char *text)
{
    if (!japanese)
        return text;
    static const struct { const char *english; const char *translated; } labels[] = {
        { "Unable to save language", "言語を保存できません" },
        { "Booted from %s via %s", "%sから%s経由で起動" },
        { "Press A to select, START to save", "十字: 選択  A: 切替  START: 保存して終了" },
        { "Use the DPAD and A to change settings", "十字: 選択  A: 切替  START: 保存して終了" },
    };
    for (u32 i = 0; i < sizeof(labels) / sizeof(labels[0]); ++i)
    {
        const char *a = text, *b = labels[i].english;
        while (*a && *a == *b) { ++a; ++b; }
        if (*a == *b)
            return labels[i].translated;
    }
    return text;
}
