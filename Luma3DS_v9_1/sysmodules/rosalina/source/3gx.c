#include <3ds.h>
#include "3gx.h"

static inline u32 invertEndianness(u32 val)
{
    return ((val & 0xFF) << 24) | ((val & 0xFF00) << 8) | ((val & 0xFF0000) >> 8) | ((val & 0xFF000000) >> 24);
}




Result  Check_3gx_Magic(IFile *file)
{
    u64     magic;
    u64     total;
    Result  res;
    int     verDif;

    file->pos = 0;
    if (R_FAILED((res = IFile_Read(file, &total, &magic, sizeof(u64)))))
        return res;

    file->pos = 0;

    if ((u32)magic != (u32)_3GX_MAGIC)
        return MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_LDR, 1);

    verDif = invertEndianness((u32)(magic >> 32)) - invertEndianness((u32)(_3GX_MAGIC >> 32));
    if (verDif != 0)
        return MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_LDR, (verDif < 0) ? 2 : 3);

    return 0;
}

Result  Read_3gx_Header(IFile *file, _3gx_Header *header)
{
    u64     total;
    char *  dst;
    Result  res = 0;

    res = IFile_Read(file, &total, header, sizeof(_3gx_Header));
    if (R_FAILED(res))
        return res;

    
    file->pos = (u32)header->infos.authorMsg;
    dst = (char *)header + sizeof(_3gx_Header);
    res = IFile_Read(file, &total, dst, header->infos.authorLen);
    if (R_FAILED(res))
        return res;

    
    header->infos.authorMsg = dst;

    
    file->pos = (u32)header->infos.titleMsg;
    dst += header->infos.authorLen;
    res = IFile_Read(file, &total, dst, header->infos.titleLen);
    if (R_FAILED(res))
        return res;

    
    header->infos.titleMsg = dst;

    
    header->infos.summaryLen = 0;
    header->infos.summaryMsg = NULL;
    header->infos.descriptionLen = 0;
    header->infos.descriptionMsg = NULL;

    
    file->pos = (u32)header->targets.titles;
    dst += header->infos.titleLen;
    dst += 4 - ((u32)dst & 3); 
    res = IFile_Read(file, &total, dst, header->targets.count * sizeof(u32));
    if (R_FAILED(res))
        return res;

    
    header->targets.titles = (u32 *)dst;
    return res;
}

Result  Read_3gx_Code(IFile *file, _3gx_Header *header, void *dst)
{
    u64     total;
    Result  res = 0;

    file->pos = (u32)header->code;
    res = IFile_Read(file, &total, dst, header->codeSize);

    return res;
}
