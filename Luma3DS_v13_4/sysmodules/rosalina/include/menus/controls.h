#pragma once

#include <3ds/types.h>

extern u32 g_menuRepeatDelay;
extern u32 g_menuRepeatInterval;
extern bool g_menuTransitions;

void MenuControls_Load(void);
void MenuControls_Apply(void);
void RosalinaMenu_Controls(void);
