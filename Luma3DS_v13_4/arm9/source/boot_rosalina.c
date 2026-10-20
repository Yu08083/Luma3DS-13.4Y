#include "boot_rosalina.h"
#include "boot_language.h"
#include "buttons.h"
#include "config.h"
#include "draw.h"
#include "fmt.h"
#include "screen.h"
#include "utils.h"

void BootRosalina_Configure(void)
{
    static const char *names[] = {"A", "B", "SELECT", "START", "RIGHT", "LEFT", "UP", "DOWN", "R", "L", "X", "Y", "ZL", "ZR"};
    static const u32 masks[] = {1,2,4,8,16,32,64,128,256,512,1024,2048,16384,32768};
    u32 selected=0, combo=configData.rosalinaMenuCombo;
    bool redraw=true, failed=false;
    while(true)
    {
        if(redraw)
        {
            clearScreens(false);
            drawString(true,10,10,COLOR_TITLE,BootLanguage_IsJapanese()?"Rosalinaを開くボタン":"Rosalina menu buttons");
            for(u32 i=0;i<14;i++)
                drawFormattedString(true,20+(i/7)*180,45+(i%7)*20,i==selected?COLOR_RED:COLOR_TEXT,"(%c) %s",combo&masks[i]?'x':' ',names[i]);
            drawString(false,10,10,COLOR_TEXT,BootLanguage_IsJapanese()?"十字キー: 選択   A: 切り替え":"DPAD: Select    A: Toggle");
            drawString(false,10,30,COLOR_TEXT,BootLanguage_IsJapanese()?"START: 保存   B: 戻る":"START: Save    B: Back");
            drawString(false,10,50,COLOR_TEXT,BootLanguage_IsJapanese()?"X: 標準に戻す":"X: Restore default");
            if(failed)drawString(false,10,90,COLOR_RED,BootLanguage_IsJapanese()?"保存できません。ボタンとSDを確認してください。":"Cannot save. Check the buttons and SD card.");
            redraw=false;
        }
        u32 pressed=waitInput(true);
        if(pressed&BUTTON_B)break;
        if(pressed&BUTTON_START)
        {
            if(configSetRosalinaMenuCombo(combo))break;
            failed=redraw=true;
            continue;
        }
        if(pressed&BUTTON_UP)selected=selected?selected-1:13;
        else if(pressed&BUTTON_DOWN)selected=(selected+1)%14;
        else if(pressed&(BUTTON_LEFT|BUTTON_RIGHT))selected=(selected+7)%14;
        else if(pressed&BUTTON_A)combo^=masks[selected];
        else if(pressed&BUTTON_X)combo=BUTTON_L1|BUTTON_DOWN|BUTTON_SELECT;
        else continue;
        failed=false;redraw=true;
    }
    while(HID_PAD&PIN_BUTTONS);
}
