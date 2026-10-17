#include "updater_http.h"
#include <string.h>

typedef struct Reader {
    const UpdateHttpIo *io;
    unsigned char buffer[4096];
    size_t position,size;
    int error;
} Reader;

static int byte(Reader *reader)
{
    if(reader->position==reader->size)
    {
        int result=reader->io->read(reader->io->context,reader->buffer,sizeof(reader->buffer));
        if(result<=0){reader->error=result;return -1;}
        if((unsigned)result>sizeof(reader->buffer)){reader->error=UPDATE_ERR_HTTP;return -1;}
        reader->position=0;
        reader->size=(size_t)result;
    }
    return reader->buffer[reader->position++];
}

static int line(Reader *reader,char *out,size_t capacity,uint32_t *budget)
{
    size_t used=0;
    for(;;)
    {
        int value=byte(reader);
        if(value<0)return reader->error<0?reader->error:UPDATE_ERR_HTTP;
        if(!*budget)return UPDATE_ERR_HTTP;
        --*budget;
        if(value=='\r')
        {
            value=byte(reader);
            if(value<0)return reader->error<0?reader->error:UPDATE_ERR_HTTP;
            if(value!='\n'||!*budget)return UPDATE_ERR_HTTP;
            --*budget;
            out[used]=0;
            return UPDATE_OK;
        }
        if(value<32||value>126||used+1>=capacity)return UPDATE_ERR_HTTP;
        out[used++]=(char)value;
    }
}

static bool same(const char *a,const char *b)
{
    while(*a&&*b)
    {
        unsigned x=(unsigned char)*a++,y=(unsigned char)*b++;
        if(x>='A'&&x<='Z')x+=32;
        if(y>='A'&&y<='Z')y+=32;
        if(x!=y)return false;
    }
    return !*a&&!*b;
}

static bool token(unsigned value)
{
    if((value>='a'&&value<='z')||(value>='A'&&value<='Z')||(value>='0'&&value<='9'))return true;
    return value&&strchr("!#$%&'*+-.^_" "\x60" "|~",(int)value);
}

static bool field(char *input,char **name,char **value)
{
    char *separator=strchr(input,':');
    if(!separator||separator==input)return false;
    for(char *p=input;p<separator;++p)if(!token((unsigned char)*p))return false;
    *separator=0;
    *name=input;
    *value=separator+1;
    while(**value==' ')++*value;
    char *end=*value+strlen(*value);
    while(end>*value&&end[-1]==' ')*--end=0;
    return true;
}

static bool decimal(const char *input,uint32_t *value)
{
    uint32_t number=0;
    if(!*input)return false;
    for(;*input;++input)
    {
        unsigned digit=(unsigned char)*input-'0';
        if(digit>9||number>(UINT32_MAX-digit)/10)return false;
        number=number*10+digit;
    }
    *value=number;
    return true;
}

static int copy(Reader *reader,uint32_t count,uint32_t limit,UpdateHttpSink sink,void *context,uint32_t *received)
{
    if(count>limit-*received)return UPDATE_ERR_SIZE;
    while(count)
    {
        if(reader->position==reader->size)
        {
            int value=byte(reader);
            if(value<0)return reader->error<0?reader->error:UPDATE_ERR_HTTP;
            --reader->position;
        }
        size_t amount=reader->size-reader->position;
        if(amount>count)amount=count;
        int result=sink(context,reader->buffer+reader->position,(uint32_t)amount);
        if(result)return result;
        reader->position+=amount;
        count-=(uint32_t)amount;
        *received+=(uint32_t)amount;
    }
    return UPDATE_OK;
}

int UpdateHttp_Read(const UpdateHttpIo *io,uint32_t limit,UpdateHttpSink sink,void *context,UpdateHttpResponse *response)
{
    if(!io||!io->read||!sink||!response)return UPDATE_ERR_HTTP;
    Reader reader={.io=io};
    char text[UPDATE_MAX_URL+32];
    uint32_t budget=8192,length=0;
    bool hasLength=false,chunked=false,hasEncoding=false,hasLocation=false;
    memset(response,0,sizeof(*response));
    int result=line(&reader,text,sizeof(text),&budget);
    if(result)return result;
    size_t initial=strlen(text);
    if(initial<12||(memcmp(text,"HTTP/1.1 ",9)&&memcmp(text,"HTTP/1.0 ",9)))return UPDATE_ERR_HTTP;
    for(unsigned i=9;i<12;++i)if(text[i]<'0'||text[i]>'9')return UPDATE_ERR_HTTP;
    if(text[12]&&text[12]!=' ')return UPDATE_ERR_HTTP;
    response->status=(text[9]-'0')*100+(text[10]-'0')*10+text[11]-'0';
    for(;;)
    {
        result=line(&reader,text,sizeof(text),&budget);
        if(result)return result;
        if(!*text)break;
        char *name,*value;
        if(!field(text,&name,&value))return UPDATE_ERR_HTTP;
        if(same(name,"Content-Length"))
        {
            if(hasLength||!decimal(value,&length))return UPDATE_ERR_HTTP;
            hasLength=true;
        }
        else if(same(name,"Transfer-Encoding"))
        {
            if(chunked||!same(value,"chunked"))return UPDATE_ERR_HTTP;
            chunked=true;
        }
        else if(same(name,"Content-Encoding"))
        {
            if(hasEncoding||!same(value,"identity"))return UPDATE_ERR_HTTP;
            hasEncoding=true;
        }
        else if(same(name,"Location"))
        {
            if(hasLocation||strlen(value)>=sizeof(response->location))return UPDATE_ERR_HTTP;
            strcpy(response->location,value);
            hasLocation=true;
        }
    }
    if(hasLength&&chunked)return UPDATE_ERR_HTTP;
    if(response->status==301||response->status==302||response->status==303||response->status==307||response->status==308)
        return hasLocation?UPDATE_OK:UPDATE_ERR_HTTP;
    if(response->status!=200)return UPDATE_ERR_HTTP;
    if(hasLength)
    {
        result=copy(&reader,length,limit,sink,context,&response->size);
        if(result)return result;
        if(reader.position!=reader.size)return UPDATE_ERR_HTTP;
        return UPDATE_OK;
    }
    if(chunked)
    {
        for(;;)
        {
            budget=128;
            result=line(&reader,text,sizeof(text),&budget);
            if(result)return result;
            uint32_t chunk=0;
            unsigned digits=0;
            char *p=text;
            for(;*p&&*p!=';';++p)
            {
                unsigned digit=(unsigned char)*p;
                if(digit>='0'&&digit<='9')digit-='0';
                else if(digit>='a'&&digit<='f')digit-=('a'-10);
                else if(digit>='A'&&digit<='F')digit-=('A'-10);
                else return UPDATE_ERR_HTTP;
                if(++digits>8||chunk>(UINT32_MAX-digit)/16)return UPDATE_ERR_HTTP;
                chunk=chunk*16+digit;
            }
            if(!digits)return UPDATE_ERR_HTTP;
            if(!chunk)
            {
                budget=2048;
                do
                {
                    result=line(&reader,text,sizeof(text),&budget);
                    if(result)return result;
                    if(*text)
                    {
                        char *name,*value;
                        if(!field(text,&name,&value)||same(name,"Content-Length")||same(name,"Transfer-Encoding")||same(name,"Location"))return UPDATE_ERR_HTTP;
                    }
                }while(*text);
                return reader.position==reader.size?UPDATE_OK:UPDATE_ERR_HTTP;
            }
            result=copy(&reader,chunk,limit,sink,context,&response->size);
            if(result)return result;
            int first=byte(&reader);
            if(first<0)return reader.error<0?reader.error:UPDATE_ERR_HTTP;
            int second=byte(&reader);
            if(second<0)return reader.error<0?reader.error:UPDATE_ERR_HTTP;
            if(first!='\r'||second!='\n')return UPDATE_ERR_HTTP;
        }
    }
    for(;;)
    {
        int value=byte(&reader);
        if(value<0)return reader.error<0?reader.error:UPDATE_OK;
        --reader.position;
        result=copy(&reader,(uint32_t)(reader.size-reader.position),limit,sink,context,&response->size);
        if(result)return result;
    }
}
