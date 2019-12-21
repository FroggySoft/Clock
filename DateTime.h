// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!
 #include "Arduino.h"

#ifndef DATETIME_H
#define DATETIME_H

class DateTime;

byte GetMoonPhase(DateTime aDate);
DateTime GetSunRise(DateTime aDate);
DateTime GetSunSet(DateTime aDate);
DateTime HoursMinutes(float aTime);
int DiffinDays(DateTime aDate1, DateTime aDate2);
uint16_t date2days(uint16_t y, uint8_t m, uint8_t d);
uint16_t dayOfYear(uint16_t aYear, uint8_t aMonth, uint8_t aDay);
bool IsDst(DateTime aDate);
bool IsDst(uint16_t aY, uint8_t aM, uint8_t aD);

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime {
public:
    DateTime ();
    DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour =0, uint8_t min =0);
    DateTime (const char* date, const char* time);
    void Clear();
    bool IsValid();

    bool operator==(const DateTime theOther);
    bool operator!=(const DateTime theOther);

	// return time as string,
	// when bool aShortHour = true the tens of the hours will be a space for the 0 (e.g. 5:44 iso 05:44)
    const char* GetTimeStr(bool aShortHour = false);	
    const char* GetDateStr(bool aShort = false);
    
    uint8_t dayOfWeek() const;

    // 32-bit times as seconds since 1/1/2000
    long secondstime() const;   
    // 32-bit times as seconds since 1/1/1970
    uint32_t unixtime(void) const;

    uint16_t y;
    uint8_t  m, d, hh, mm;
};



#endif
