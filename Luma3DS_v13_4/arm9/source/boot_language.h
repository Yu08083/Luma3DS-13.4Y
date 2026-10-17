#pragma once

#include "types.h"

void BootLanguage_Load(void);
bool BootLanguage_IsJapanese(void);
bool BootLanguage_Toggle(void);
const char *BootLanguage_Selector(void);
const char *BootLanguage_Text(const char *text);
