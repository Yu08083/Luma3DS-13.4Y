#include "updater_clock.h"
#include <string.h>

static bool leap(int year)
{
    return year%4==0&&(year%100!=0||year%400==0);
}

int64_t UpdateClock_FromDate(int year,int month,int day,int hour,int minute,int second)
{
    static const uint16_t before[]={0,31,59,90,120,151,181,212,243,273,304,334};
    static const uint8_t days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(year<1970||year>9999||month<1||month>12||day<1||hour<0||hour>23||minute<0||minute>59||second<0||second>59)return -1;
    if(day>days[month-1]+(month==2&&leap(year)))return -1;
    int64_t prior=year-1;
    int64_t count=(int64_t)(year-1970)*365+prior/4-prior/100+prior/400-(1969/4-1969/100+1969/400);
    count+=before[month-1]+(month>2&&leap(year))+day-1;
    return ((count*24+hour)*60+minute)*60+second;
}

static int decimal(const char *text,unsigned length)
{
    int result=0;
    for(unsigned index=0;index<length;++index)
    {
        if(text[index]<'0'||text[index]>'9')return -1;
        result=result*10+text[index]-'0';
    }
    return result;
}

bool UpdateClock_ParseHttpDate(const char *text,int64_t *seconds)
{
    static const char weekdays[][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static const char months[][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    if(!text||!seconds)return false;
    unsigned length=0;
    while(length<30&&text[length])++length;
    if(length!=29||memcmp(text+3,", ",2)||text[7]!=' '||text[11]!=' '||text[16]!=' '||text[19]!=':'||text[22]!=':'||memcmp(text+25," GMT",4))return false;
    unsigned weekday=0,month=0;
    while(weekday<7&&memcmp(text,weekdays[weekday],3))++weekday;
    while(month<12&&memcmp(text+8,months[month],3))++month;
    if(weekday==7||month==12)return false;
    int64_t value=UpdateClock_FromDate(decimal(text+12,4),(int)month+1,decimal(text+5,2),decimal(text+17,2),decimal(text+20,2),decimal(text+23,2));
    if(value<0||(value/86400+4)%7!=(int64_t)weekday)return false;
    *seconds=value;
    return true;
}
