import struct

NATIVE_MODULE_LIMIT=0x80000
BUNDLED_NATIVE_BASELINE=0x4AC00
EXPECTED_MODULES=('loader','sm','pm','pxi','rosalina')

def bundled_native_modules(data,limit=BUNDLED_NATIVE_BASELINE):
    if len(data)<0x200 or data[:4]!=b'FIRM':
        raise ValueError('Invalid FIRM header')
    found=[]
    for index in range(4):
        offset,address,size,method=struct.unpack_from('<4I',data,0x40+index*0x30)
        if size and address==0x18180000:
            if offset<0x200 or offset%0x200 or offset+size>len(data) or method>2:
                raise ValueError('Invalid bundled-module section')
            found.append((offset,size))
    if len(found)!=1:
        raise ValueError('Expected one bundled-module section')
    offset,size=found[0]
    position=offset
    rows=[]
    while position<offset+size:
        if position+0x400>offset+size or data[position+0x100:position+0x104]!=b'NCCH':
            raise ValueError('Invalid bundled NCCH header')
        shift=9+data[position+0x18E]
        if shift!=9:
            raise ValueError('Unsupported bundled NCCH media unit')
        length=struct.unpack_from('<I',data,position+0x104)[0]<<shift
        if length<0x400 or position+length>offset+size:
            raise ValueError('Invalid bundled NCCH size')
        raw_name=data[position+0x200:position+0x208]
        name=raw_name.split(b'\0',1)[0].decode('ascii')
        if not name or any(row['name']==name for row in rows):
            raise ValueError('Invalid or duplicate bundled-module name')
        rows.append(dict(name=name,size=length))
        position+=length
    if tuple(row['name'] for row in rows)!=EXPECTED_MODULES:
        raise ValueError('Unexpected bundled-module list')
    if size>limit:
        raise ValueError(f'Bundled modules {size:#x} exceed known-working baseline {limit:#x}; native runtime budget is {NATIVE_MODULE_LIMIT:#x}')
    return dict(total=size,limit=limit,native_limit=NATIVE_MODULE_LIMIT,remaining_for_stock_modules=NATIVE_MODULE_LIMIT-size,modules=rows)
