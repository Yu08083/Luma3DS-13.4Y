#include "updater_format.h"
#include <string.h>
#include <stdio.h>

typedef struct JsonCursor {
    const unsigned char *text;
    size_t length,pos;
} JsonCursor;

static void jsonSpace(JsonCursor *j) {
    while(j->pos<j->length && (j->text[j->pos]==' ' || j->text[j->pos]=='\t' || j->text[j->pos]=='\r' || j->text[j->pos]=='\n')) j->pos++;
}

static int jsonHex(unsigned char c) {
    if(c>='0' && c<='9') return c-'0';
    if(c>='a' && c<='f') return c-'a'+10;
    if(c>='A' && c<='F') return c-'A'+10;
    return -1;
}

static bool jsonUnicode(JsonCursor *j,uint32_t *value) {
    if(j->length-j->pos<4) return false;
    *value=0;
    for(unsigned int i=0;i<4;i++) {
        int h=jsonHex(j->text[j->pos++]);
        if(h<0) return false;
        *value=(*value<<4)|(unsigned int)h;
    }
    return true;
}

static bool jsonAppend(char *out,size_t capacity,size_t *used,uint32_t cp) {
    unsigned char b[4];size_t n;
    if(!cp || cp>0x10FFFF || (cp>=0xD800 && cp<=0xDFFF)) return false;
    if(cp<0x80) { b[0]=(unsigned char)cp;n=1; }
    else if(cp<0x800) { b[0]=0xC0|(cp>>6);b[1]=0x80|(cp&63);n=2; }
    else if(cp<0x10000) { b[0]=0xE0|(cp>>12);b[1]=0x80|((cp>>6)&63);b[2]=0x80|(cp&63);n=3; }
    else { b[0]=0xF0|(cp>>18);b[1]=0x80|((cp>>12)&63);b[2]=0x80|((cp>>6)&63);b[3]=0x80|(cp&63);n=4; }
    if(out) {
        if(*used+n>=capacity) return false;
        memcpy(out+*used,b,n);
    }
    *used+=n;
    return true;
}

static bool jsonString(JsonCursor *j,char *out,size_t capacity) {
    jsonSpace(j);
    if(j->pos>=j->length || j->text[j->pos++]!='"') return false;
    size_t used=0;
    while(j->pos<j->length) {
        uint32_t cp=j->text[j->pos++];
        if(cp=='"') { if(out) out[used]=0;return true; }
        if(cp<32) return false;
        if(cp=='\\') {
            if(j->pos==j->length) return false;
            cp=j->text[j->pos++];
            if(cp=='u') {
                if(!jsonUnicode(j,&cp)) return false;
                if(cp>=0xD800 && cp<=0xDBFF) {
                    if(j->length-j->pos<6 || j->text[j->pos++]!='\\' || j->text[j->pos++]!='u') return false;
                    uint32_t low;
                    if(!jsonUnicode(j,&low) || low<0xDC00 || low>0xDFFF) return false;
                    cp=0x10000+((cp-0xD800)<<10)+low-0xDC00;
                }
            }
            else if(cp=='b') cp=8;
            else if(cp=='f') cp=12;
            else if(cp=='n') cp=10;
            else if(cp=='r') cp=13;
            else if(cp=='t') cp=9;
            else if(cp!='"' && cp!='\\' && cp!='/') return false;
        }
        else if(cp>=0x80) {
            unsigned int trailing;
            uint32_t minimum;
            if(cp>=0xC2 && cp<=0xDF) {trailing=1;minimum=0x80;cp&=31;}
            else if(cp>=0xE0 && cp<=0xEF) {trailing=2;minimum=0x800;cp&=15;}
            else if(cp>=0xF0 && cp<=0xF4) {trailing=3;minimum=0x10000;cp&=7;}
            else return false;
            if(j->length-j->pos<trailing) return false;
            while(trailing--) {
                unsigned char c=j->text[j->pos++];
                if((c&0xC0)!=0x80) return false;
                cp=(cp<<6)|(c&63);
            }
            if(cp<minimum) return false;
        }
        if(!jsonAppend(out,capacity,&used,cp)) return false;
    }
    return false;
}

static bool jsonLiteral(JsonCursor *j,const char *word) {
    size_t n=strlen(word);
    if(j->length-j->pos<n || memcmp(j->text+j->pos,word,n)) return false;
    j->pos+=n;
    return true;
}

static bool jsonNumber(JsonCursor *j,uint32_t *out) {
    size_t start=j->pos;
    if(!out && j->pos<j->length && j->text[j->pos]=='-') j->pos++;
    if(j->pos==j->length || j->text[j->pos]<'0' || j->text[j->pos]>'9') return false;
    bool zero=j->text[j->pos]=='0';uint32_t value=0;unsigned int digits=0;
    do {
        unsigned int digit=j->text[j->pos++]-'0';digits++;
        if(out && value>(UINT32_MAX-digit)/10) return false;
        value=value*10+digit;
    } while(j->pos<j->length && j->text[j->pos]>='0' && j->text[j->pos]<='9');
    if(zero && digits>1) return false;
    if(!out && j->pos<j->length && j->text[j->pos]=='.') {
        j->pos++;size_t begin=j->pos;
        while(j->pos<j->length && j->text[j->pos]>='0' && j->text[j->pos]<='9') j->pos++;
        if(begin==j->pos) return false;
    }
    if(!out && j->pos<j->length && (j->text[j->pos]=='e' || j->text[j->pos]=='E')) {
        j->pos++;
        if(j->pos<j->length && (j->text[j->pos]=='+' || j->text[j->pos]=='-')) j->pos++;
        size_t begin=j->pos;
        while(j->pos<j->length && j->text[j->pos]>='0' && j->text[j->pos]<='9') j->pos++;
        if(begin==j->pos) return false;
    }
    if(out) *out=value;
    return j->pos>start;
}

static bool jsonSkip(JsonCursor *j,unsigned int depth) {
    jsonSpace(j);
    if(depth>16 || j->pos==j->length) return false;
    unsigned char c=j->text[j->pos];
    if(c=='"') return jsonString(j,NULL,0);
    if(c=='t') return jsonLiteral(j,"true");
    if(c=='f') return jsonLiteral(j,"false");
    if(c=='n') return jsonLiteral(j,"null");
    if(c=='{' || c=='[') {
        unsigned char close=c=='{'?'}':']';j->pos++;jsonSpace(j);
        if(j->pos<j->length && j->text[j->pos]==close) {j->pos++;return true;}
        for(;;) {
            if(c=='{') {
                if(!jsonString(j,NULL,0)) return false;
                jsonSpace(j);
                if(j->pos==j->length || j->text[j->pos++]!=':') return false;
            }
            if(!jsonSkip(j,depth+1)) return false;
            jsonSpace(j);
            if(j->pos==j->length) return false;
            unsigned char separator=j->text[j->pos++];
            if(separator==close) return true;
            if(separator!=',') return false;
        }
    }
    return jsonNumber(j,NULL);
}

static bool updateTag(const char *tag) {
    size_t length=0;
    if(!tag || !*tag) return false;
    while(tag[length]) {
        unsigned char c=(unsigned char)tag[length++];
        if(length>=UPDATE_MAX_TAG || !((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='.' || c=='_' || c=='-')) return false;
    }
    return strcmp(tag,".") && strcmp(tag,"..");
}

static bool jsonRoot(const char *text,size_t length,const char *tag,UpdateManifest *manifest,char *releaseTag) {
    JsonCursor j={(const unsigned char*)text,length,0};unsigned int fields=0;
    char key[64],value[UPDATE_MAX_TAG+1];
    jsonSpace(&j);
    if(j.pos==j.length || j.text[j.pos++]!='{') return false;
    for(;;) {
        if(!jsonString(&j,key,sizeof(key))) return false;
        jsonSpace(&j);
        if(j.pos==j.length || j.text[j.pos++]!=':') return false;
        jsonSpace(&j);
        unsigned int bit=0;
        if(releaseTag) {
            if(!strcmp(key,"tag_name")) {
                bit=1;
                if(!jsonString(&j,releaseTag,UPDATE_MAX_TAG) || !updateTag(releaseTag)) return false;
            }
            else if(!strcmp(key,"draft") || !strcmp(key,"prerelease")) {
                bit=!strcmp(key,"draft")?2:4;
                if(!jsonLiteral(&j,"false")) return false;
            }
        }
        else {
            if(!strcmp(key,"schema")) {
                uint32_t schema;bit=1;
                if(!jsonNumber(&j,&schema) || schema!=1) return false;
            }
            else if(!strcmp(key,"version")) {
                bit=2;
                if(!jsonString(&j,manifest->version,sizeof(manifest->version)) || strcmp(manifest->version,tag)) return false;
            }
            else if(!strcmp(key,"asset")) {
                bit=4;
                if(!jsonString(&j,value,sizeof(value)) || strcmp(value,"boot.firm")) return false;
            }
            else if(!strcmp(key,"size")) {
                bit=8;
                if(!jsonNumber(&j,&manifest->size) || manifest->size<1024 || manifest->size>UPDATE_MAX_FIRM) return false;
            }
            else if(!strcmp(key,"sha256")) {
                bit=16;
                if(!jsonString(&j,value,sizeof(value)) || strlen(value)!=64) return false;
                for(unsigned int i=0;i<32;i++) {
                    unsigned char a=value[i*2],b=value[i*2+1];
                    if(!((a>='0' && a<='9') || (a>='a' && a<='f')) || !((b>='0' && b<='9') || (b>='a' && b<='f'))) return false;
                    manifest->sha256[i]=(uint8_t)((jsonHex(a)<<4)|jsonHex(b));
                }
            }
        }
        if(bit) {
            if(fields&bit) return false;
            fields|=bit;
        }
        else if(!jsonSkip(&j,0)) return false;
        jsonSpace(&j);
        if(j.pos==j.length) return false;
        unsigned char separator=j.text[j.pos++];
        if(separator=='}') break;
        if(separator!=',') return false;
    }
    jsonSpace(&j);
    return j.pos==j.length && fields==(releaseTag?7u:31u);
}

bool Update_ParseRelease(const char *json,size_t length,char tag[UPDATE_MAX_TAG]) {
    if(!json || !tag || !length || length>UPDATE_MAX_JSON) return false;
    tag[0]=0;
    return jsonRoot(json,length,NULL,NULL,tag);
}

bool Update_ParseManifest(const char *json,size_t length,const char *tag,UpdateManifest *manifest) {
    if(!json || !manifest || !updateTag(tag) || !length || length>UPDATE_MAX_MANIFEST) return false;
    memset(manifest,0,sizeof(*manifest));
    return jsonRoot(json,length,tag,manifest,NULL);
}

bool Update_TrustedUrl(const char *url) {
    if(!url || strncmp(url,"https://",8)) return false;
    size_t length=strlen(url);
    if(length>=UPDATE_MAX_URL) return false;
    const char *host=url+8,*end=strchr(host,'/');
    if(!end || end==host) return false;
    size_t hostLength=(size_t)(end-host);
    static const char *const hosts[]={"api.github.com","github.com","release-assets.githubusercontent.com","objects.githubusercontent.com"};
    bool allowed=false;
    for(unsigned int i=0;i<sizeof(hosts)/sizeof(hosts[0]);i++)
        if(strlen(hosts[i])==hostLength && !memcmp(host,hosts[i],hostLength)) allowed=true;
    if(!allowed) return false;
    for(size_t i=0;i<length;i++)
        if((unsigned char)url[i]<=32 || (unsigned char)url[i]>=127 || url[i]=='\\' || url[i]=='#') return false;
    return true;
}

bool Update_AssetUrl(char out[UPDATE_MAX_URL],const char *tag,const char *asset) {
    if(!out || !updateTag(tag) || !asset || (strcmp(asset,"boot.firm") && strcmp(asset,"update.json"))) return false;
    int length=snprintf(out,UPDATE_MAX_URL,"https://github.com/Yu08083/Luma3DS-13.4Y/releases/download/%s/%s",tag,asset);
    return length>0 && (unsigned int)length<UPDATE_MAX_URL && Update_TrustedUrl(out);
}

static uint32_t firmWord(const uint8_t *b) {
    return (uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);
}

static bool firmRange(uint32_t address,uint32_t size) {
    static const uint32_t ranges[][2]={{0x08000000,0x08100000},{0x18000000,0x18600000},{0x1FF00000,0x1FFFFC00},{0x20000000,0x28000000}};
    uint64_t end=(uint64_t)address+size;
    for(unsigned int i=0;i<sizeof(ranges)/sizeof(ranges[0]);i++)
        if(address>=ranges[i][0] && end<=ranges[i][1]) return true;
    return false;
}

bool Update_FirmHeader(const uint8_t header[512],uint32_t fileSize,UpdateSection sections[4],uint32_t *count) {
    if(!header || !sections || !count || fileSize<1024 || fileSize>UPDATE_MAX_FIRM || memcmp(header,"FIRM",4)) return false;
    *count=0;
    uint32_t arm11=firmWord(header+8),arm9=firmWord(header+12);
    if(!arm9 || (arm9&3) || (arm11&3)) return false;
    bool found9=false,found11=arm11==0;
    for(unsigned int i=0;i<4;i++) {
        const uint8_t *row=header+0x40+i*0x30;
        uint32_t offset=firmWord(row),address=firmWord(row+4),size=firmWord(row+8),method=firmWord(row+12);
        if(!size) continue;
        if(offset<512 || (offset&511) || (size&511) || (address&3) || method>2 || (uint64_t)offset+size>fileSize || !firmRange(address,size)) return false;
        for(unsigned int k=0;k<*count;k++) {
            const UpdateSection *p=sections+k;
            if((uint64_t)offset<(uint64_t)p->offset+p->size && (uint64_t)p->offset<(uint64_t)offset+size) return false;
            if((uint64_t)address<(uint64_t)p->address+p->size && (uint64_t)p->address<(uint64_t)address+size) return false;
        }
        UpdateSection *out=sections+(*count)++;
        out->offset=offset;out->address=address;out->size=size;out->method=method;
        memcpy(out->hash,row+16,32);
        if(arm9>=address && (uint64_t)arm9+4<=(uint64_t)address+size) found9=true;
        if(arm11>=address && (uint64_t)arm11+4<=(uint64_t)address+size) found11=true;
    }
    return *count && found9 && found11;
}
