#include "DateTime.h"

#define DS1307_ADDRESS 0x68
#define SECONDS_PER_DAY 86400L

#define SECONDS_FROM_1970_TO_2000 946684800

////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

#define LEAP_YEAR(_year) ((_year%4)==0)
const uint8_t daysInMonth [] = { 31,28,31,30,31,30,31,31,30,31,30,31 }; //has to be const or compiler compaints
const float longitude =  5.73;      // Lengtegraad in decimale graden, <0 voor OL, >0 voor WL
const float latitude = 51.81;     // Breedtegraad in decimale graden, >0 voor NB, <0 voor ZB

// number of days since 2000/01/01, valid for 2001..2099

uint16_t date2days(uint16_t y, uint8_t m, uint8_t d)
{
    if (y >= 2000)
        y -= 2000;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)		// m is counted from 1..12
        days += daysInMonth[i-1];
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}
static uint16_t date2days(DateTime aDate)
{
  return date2days(aDate.y, aDate.m, aDate.d);
}
static uint16_t dayOfYear(uint16_t aYear, uint8_t aMonth, uint8_t aDay)
{
    uint16_t days = aDay;
    for (uint8_t i = 1; i < aMonth; ++i)		// m is counted from 1..12
        days += daysInMonth[i-1];
    if (aMonth > 2 && aYear % 4 == 0)
        ++days;
    return days;  
}

static unsigned long makeTime(DateTime aDate)
{
// converts time components to time_t
// note year argument is full four digit year (or digits since 2000), i.e.1975, (year 8 is 2008)

   int i;
   int year = aDate.y;
   unsigned long seconds;

    // seconds from 1970 till 1 jan 00:00:00 this year
    seconds= (year-1970)*(60*60*24L*365);

    // add extra days for leap years
    for (i=1970; i<year; i++)
    {
        if (LEAP_YEAR(i))
        {
            seconds+= 60*60*24L;
        }
    }
    // add days for this year
    for (i=0; i<aDate.m; i++)
    {
      if (i==1 && LEAP_YEAR(year))
      {
        seconds+= 60*60*24L*29;
      }
      else
      {
        seconds+= 60*60*24L*daysInMonth[i];
      }
    }

    seconds+= (aDate.d-1)*3600*24L;
    seconds+= aDate.hh*3600L;
    seconds+= aDate.mm*60L;
    return seconds;
}
  
static int DiffinDays(DateTime aDate1, DateTime aDate2)
{
  uint16_t days1 = date2days(aDate1);
  uint16_t days2 = date2days(aDate2);

  if(days1<days2)
    return days2-days1;
  return days1-days2;
}

byte GetMoonPhase(DateTime aDate)
{
  // Note that new moon is 1 i.s.o. zero
	float MaanCyclus = 29.530589;
	DateTime NieuweMaan(2005,5,7,0,0);  //"June 7, 2005 00:00:00");
	float aantalDagen = DiffinDays(aDate,NieuweMaan);
	float CyclusDeel = aantalDagen/MaanCyclus - int(aantalDagen/MaanCyclus); // Alleen de decimalen
	return int(MaanCyclus*CyclusDeel+0.5);
}

DateTime getSunTime(DateTime aDate, bool aRiseNotSet)
{
	float eqtime,hars;
	float doy = dayOfYear(aDate.y,aDate.m,aDate.d);

	doy -= 0.5;

	// equation of time (in minutes)
	float x = doy*2*PI/365; // fractional year in radians
	eqtime = 229.18*(0.000075+0.001868*cos(x) - 0.032077*sin(x) - 0.014615*cos(2*x) - 0.040849*sin(2*x));

	// declination (in degrees)
	float declin = 0.006918-0.399912*cos(x) + 0.070257*sin(x) - 0.006758*cos(2*x);
	declin = declin + 0.000907*sin(2*x) - 0.002697*cos(3*x) + 0.00148*sin(3*x);
	declin = declin * 180/PI;

	// solar azimuth angle for sunrise and sunset corrected for atmospheric refraction (in degrees),
	x = PI/180;
	hars = cos(x*90.833) / (cos(x*latitude) * cos(x*declin));
	hars = hars - tan(x*latitude)*tan(x*declin);
	hars = acos(hars)/x;

	// sunrise
	x = 720 - eqtime;
	if (aRiseNotSet)  // Sunrise
	  x += 4*(-longitude-hars);
	else    // sunset
	  x += 4*(-longitude+hars);

	x = x/60 + 1;  // from UTC to CET
  //if (aDate.dst) // extra daylight saving time correction
	if (IsDst(aDate))
	  x++;

  DateTime lSunTime;

  int hrs = (int)(x);
  x -= hrs; // remove integral part
  x *= 60;  
  int min = (int)x;

  if (min == 60)        // Voorkom dat er 16:60 verschijnt ipv 17:00
  {
    min = 0;
    hrs += 1;
    if (hrs > 24 )
      hrs = 0;
  }

  lSunTime.hh = hrs;
  lSunTime.mm = min;
  return lSunTime;
}

DateTime GetSunRise(DateTime aDate)
{
  return getSunTime(aDate, true);
}
DateTime GetSunSet(DateTime aDate)
{
  return getSunTime(aDate, false);
}

bool IsDst(DateTime aDate)
{
  return IsDst(aDate.y, aDate.m, aDate.d);
}
bool IsDst(uint16_t aY, uint8_t aM, uint8_t aD)
{
  uint16_t firstDay =  31 - ((5 * aY / 4) - (aY / 100) + (aY / 400) + 5) % 7;
  if(firstDay > 31)
    firstDay-=7;
  uint16_t lastDay = 31 - ((5 * aY / 4) - (aY / 100) + (aY / 400) + 2) % 7;
  if(lastDay > 31)
    lastDay-=7;
  bool dst = false;
  if ((aM>3) && (aM<9))   // between april and september always dst
    dst = true;
  else if((aM==3) && (aD>=firstDay))
    dst = true;
  else if((aM==9) && (aD<=lastDay))
    dst = true;
  
  return dst;
}

////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime ()
{
  Clear();
}

DateTime::DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min)
{
  Clear();
  if((year >= 1970) && (year<=2036) &&
     (month>0) && (month<=12) &&
     (day>0) && (day<=31) &&
     (hour<=24) &&
     (min<60))
  {
    y = year;
    m = month;
    d = day;
    hh = hour;
    mm = min;
  }
}

void DateTime::Clear()
{
    y = 0;
    m = 0;
    d = 0;
    hh = 0;
    mm = 0;
}

bool DateTime::IsValid()
{
  return (d != 0);
}

static char mStr[12];
const char* DateTime::GetTimeStr(bool aShortHour)
{
	if (aShortHour && hh<10)
  		sprintf(mStr," %01d:%02d",hh,mm);
	else
  		sprintf(mStr,"%02d:%02d",hh,mm);	
  	return mStr;
}
const char* DateTime::GetDateStr(bool aShort)
{
	if (aShort)
  		sprintf(mStr," %d-%d-%4d",d,m,y);
  	else
  		sprintf(mStr," %02d-%02d-%4d",d,m,y);
	return mStr;
}
bool DateTime::operator==(const DateTime theOther)
{
  return ((y == theOther.y) &&
          (m == theOther.m) &&
          (d == theOther.d) &&
          (hh == theOther.hh) &&
          (mm == theOther.mm));
}
bool DateTime::operator!=(const DateTime theOther)
{
  return !(*this==theOther);
}

static uint8_t conv2d(const char* p)
{
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

uint8_t DateTime::dayOfWeek() const
{
  uint16_t day = date2days(y, m, d);
  day = (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6

  if( day==0)
    day=6;
  else
    day--;
  return day;
}

 
