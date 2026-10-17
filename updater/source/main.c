#include <3ds.h>
#include "app.h"
#include "ui.h"

int main(void)
{
    if(R_FAILED(fsInit()))return 1;
    gfxInit(GSP_BGR8_OES,GSP_BGR8_OES,false);
    gfxSet3D(false);
    bool japanese=true;
    if(R_SUCCEEDED(cfguInit()))
    {
        u8 language=CFG_LANGUAGE_JP;
        if(R_SUCCEEDED(CFGU_GetSystemLanguage(&language)))japanese=language==CFG_LANGUAGE_JP;
        cfguExit();
    }
    UpdaterApp_Init();
    UpdaterUi ui;
    UpdaterUi_Init(&ui,japanese);
    bool exitRequested=false;
    while(aptMainLoop()&&!exitRequested)
    {
        hidScanInput();
        UpdaterAppView view;
        UpdaterApp_Snapshot(&view);
        aptSetHomeAllowed(!view.busy);
        UpdaterUiAction action=UpdaterUi_Input(&ui,&view,hidKeysDown());
        switch(action)
        {
            case UPDATER_UI_CHECK:UpdaterApp_RequestCheck();break;
            case UPDATER_UI_INSTALL:UpdaterApp_RequestInstall();break;
            case UPDATER_UI_CANCEL:UpdaterApp_Cancel();break;
            case UPDATER_UI_EXIT:exitRequested=true;UpdaterApp_Cancel();break;
            default:break;
        }
        UpdaterApp_Snapshot(&view);
        u16 topWidth,topHeight,bottomWidth,bottomHeight;
        u8 *top=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,&topWidth,&topHeight);
        u8 *bottom=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&bottomWidth,&bottomHeight);
        UpdaterUiSurface topSurface={top,(size_t)topWidth*topHeight*3,topHeight,topWidth};
        UpdaterUiSurface bottomSurface={bottom,(size_t)bottomWidth*bottomHeight*3,bottomHeight,bottomWidth};
        UpdaterUi_Draw(&ui,&view,&topSurface,&bottomSurface);
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    UpdaterApp_Shutdown();
    aptSetHomeAllowed(true);
    gfxExit();
    fsExit();
    return 0;
}
