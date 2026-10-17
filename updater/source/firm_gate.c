#include "firm_gate.h"
#include <string.h>

static uint32_t word(const uint8_t *data)
{
    return (uint32_t)data[0]|(uint32_t)data[1]<<8|(uint32_t)data[2]<<16|(uint32_t)data[3]<<24;
}

UpdateResult UpdaterFirm_Gate(UpdaterFirmRead read,void *context,uint32_t size)
{
    uint8_t header[512];
    UpdateSection sections[4];
    uint32_t count=0;
    if(!read||!read(context,0,header,sizeof(header)))return UPDATE_ERR_SD;
    if(!Update_FirmHeader(header,size,sections,&count))return UPDATE_ERR_FIRM;
    const UpdateSection *bundled=NULL;
    for(uint32_t index=0;index<count;++index)
        if(sections[index].address==0x18180000)
        {
            if(bundled)return UPDATE_ERR_FIRM;
            bundled=sections+index;
        }
    if(!bundled)return UPDATE_ERR_FIRM;
    if(bundled->size>0x4AC00)return UPDATE_ERR_SIZE;
    static const char expected[5][9]={"loader","sm","pm","pxi","rosalina"};
    uint32_t position=0;
    for(unsigned index=0;index<5;++index)
    {
        uint8_t cxi[0x208];
        if(position>bundled->size||sizeof(cxi)>bundled->size-position)return UPDATE_ERR_FIRM;
        if(!read(context,bundled->offset+position,cxi,sizeof(cxi)))return UPDATE_ERR_SD;
        if(memcmp(cxi+0x100,"NCCH",4)||cxi[0x18E]||memcmp(cxi+0x200,expected[index],8))return UPDATE_ERR_FIRM;
        uint32_t units=word(cxi+0x104);
        if(units<4||units>(bundled->size-position)/512)return UPDATE_ERR_FIRM;
        position+=units*512;
    }
    return position==bundled->size?UPDATE_OK:UPDATE_ERR_FIRM;
}
