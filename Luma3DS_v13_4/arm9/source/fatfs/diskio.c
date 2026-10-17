








#include "ff.h"
#include "diskio.h"
#include "sdmmc/sdmmc.h"
#include "../crypto.h"
#include "../i2c.h"


#define SDCARD        0
#define CTRNAND       1





DSTATUS disk_status (
    BYTE pdrv
)
{
    (void)pdrv;
    return RES_OK;
}







DSTATUS disk_initialize (
    BYTE pdrv
)
{
    static u32 sdmmcInitResult = 4;
    DSTATUS res = 0;

    if(sdmmcInitResult == 4)
        sdmmcInitResult = sdmmc_sdcard_init();


    switch (pdrv)
    {
        case SDCARD:
            res = (sdmmcInitResult & 2) == 0 ? 0 : STA_NOINIT;
            break;
        case CTRNAND:

            res = (sdmmcInitResult & 1) == 0 && ctrNandInit() == 0 ? 0 : STA_NOINIT;
            break;
        default:
            res = STA_NODISK;
            break;
    }

    return res;
}







DRESULT disk_read (
    BYTE pdrv,
    BYTE *buff,
    LBA_t sector,
    UINT count
)
{
    DRESULT res = RES_OK;

    switch (pdrv)
    {
        case SDCARD:
            res = sdmmc_sdcard_readsectors(sector, count, buff) == 0 ? RES_OK : RES_PARERR;
            break;
        case CTRNAND:
            res = ctrNandRead(sector, count, buff) == 0 ? RES_OK : RES_PARERR;
            break;
        default:
            res = RES_NOTRDY;
            break;
    }

    return res;
}







#if FF_FS_READONLY == 0

DRESULT disk_write (
    BYTE pdrv,
    const BYTE *buff,
    LBA_t sector,
    UINT count
)
{
    DRESULT res = RES_OK;

    switch (pdrv)
    {
        case SDCARD:
        {
            if ((*(vu16 *)(SDMMC_BASE + REG_SDSTATUS0) & TMIO_STAT0_WRPROTECT) == 0)
                res = RES_WRPRT;
            else
                res = sdmmc_sdcard_writesectors(sector, count, buff) == 0 ? RES_OK : RES_PARERR;
            break;
        }
        case CTRNAND:
            res = ctrNandWrite(sector, count, buff) == 0 ? RES_OK : RES_PARERR;
            break;
        default:
            res = RES_NOTRDY;
            break;
    }

    return res;
}
#endif






DRESULT disk_ioctl (
    BYTE pdrv,
    BYTE cmd,
    void *buff
)
{
    (void)pdrv;
    (void)buff;
    return cmd == CTRL_SYNC ? RES_OK : RES_PARERR;
}


#define BCDVALID(b) (((b)<=0x99)&&(((b)&0xF)<=0x9)&&((((b)>>4)&0xF)<=0x9))
#define BCD2NUM(b)  (BCDVALID(b) ? (((b)&0xF)+((((b)>>4)&0xF)*10)) : 0xFF)
#define NUM2BCD(n)  ((n<99) ? (((n/10)*0x10)|(n%10)) : 0x99)
#define DSTIMEGET(bcd,n) (BCD2NUM((bcd)->n))


typedef struct DsTime {
    u8 bcd_s;
    u8 bcd_m;
    u8 bcd_h;
    u8 weekday;
    u8 bcd_D;
    u8 bcd_M;
    u8 bcd_Y;
    u8 leap_count;
} DsTime;





DWORD get_fattime( void ) {
    DsTime dstime;
    I2C_readRegBuf(I2C_DEV_MCU, 0x30, (u8 *)&dstime, sizeof(DsTime));
    DWORD fattime =
        ((DSTIMEGET(&dstime, bcd_s)&0x3F) >> 1 ) |
        ((DSTIMEGET(&dstime, bcd_m)&0x3F) << 5 ) |
        ((DSTIMEGET(&dstime, bcd_h)&0x3F) << 11) |
        ((DSTIMEGET(&dstime, bcd_D)&0x1F) << 16) |
        ((DSTIMEGET(&dstime, bcd_M)&0x0F) << 21) |
        (((DSTIMEGET(&dstime, bcd_Y)+(2000-1980))&0x7F) << 25);

    return fattime;
}
