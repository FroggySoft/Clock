#include "Adafruit_GFX.h"    // Core graphics library
#include "TftSpfd5408.h" // Hardware-specific library
#include <Wire.h>
#include "DateTime.h"
#include "RTClib.h"
#include "Bmp180.h"
#include "timer.h"
#include "images/zon.h"

//#define ADJUST_TIME_AT_STARTUP

#include <Arduino.h> // capital A so it is error prone on case-sensitive filesystems

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define PIN_PIR     13  // also buildin led
#define PIR_ACTIVE  HIGH

#define PIN_RX  0   // used for debug
#define PIN_TX  1

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7


// Assign human-readable names to some common 16-bit color values:
// 5 bits red , 6 bits green, 5 bits blue
#define	BLACK   0x0000
#define	BLUE    0x001F
#define LIGHTBLUE    0x187F
#define	RED     0xF800
#define LIGHTRED 0xFACB
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GRAY1   0x38E7
#define GRAY    0xD69A
#define LIGHTGRAY 0xE71C

#define TFT_WIDTH  320
#define TFT_HEIGHT 240

#define TFT_CHAR_WIDTH  6
#define TFT_CHAR_HEIGHT 8


typedef enum
{
  DISPLAY_TIME = 0,
  DISPLAY_PRESSURE,
  MAX_DISPLAYS
 } DISPLAYS;

/*
 * 1. temp/press sun/moon
 *      time
 *   date-string
 */
#define WEATHERICON_WIDTH      48
#define WEATHERICON_HEIGHT     48

#define XPOS_SUNMOON     (TFT_WIDTH-WIDTH_SUNMOON)
#define YPOS_SUNMOON     0
#define XPOS_MOON        (XPOS_SUNMOON+MOON_RADIUS)
#define XPOS_SUN         (XPOS_MOON+MOON_RADIUS*2+10)
#define YPOS_SUN         0
#define WIDTH_SUNMOON    (MOON_RADIUS*3+10+TFT_CHAR_WIDTH*DATE_SIZE*5)
#define HEIGHT_SUNMOON   (WEATHERICON_HEIGHT+2*TFT_CHAR_HEIGHT*DATE_SIZE)
#define MOON_RADIUS 16

#define TIME_SIZE   10
#define STRLEN_TIME 5
#define WIDTH_TIME  (TFT_CHAR_WIDTH*TIME_SIZE*STRLEN_TIME)
#define HEIGHT_TIME (TFT_CHAR_HEIGHT*TIME_SIZE)
#define XPOS_TIME   ((TFT_WIDTH-WIDTH_TIME)/2)
#define YPOS_TIME   ((YPOS_SUNMOON+HEIGHT_SUNMOON) + ((TFT_HEIGHT-(YPOS_SUNMOON+HEIGHT_SUNMOON)-HEIGHT_TIME)/2))

#define DATE_SIZE   2
#define STRLEN_DATE (10+12)
#define WIDTH_DATE  (TFT_CHAR_WIDTH*DATE_SIZE*STRLEN_DATE)
#define HEIGHT_DATE (TFT_CHAR_HEIGHT*DATE_SIZE)
#define XPOS_DATE   ((TFT_WIDTH-WIDTH_DATE)/2)
#define YPOS_DATE   (YPOS_TIME+HEIGHT_TIME) // 300
#define XPOS_ACTIVE_ALARMS  XPOS_DATE
#define YPOS_ACTIVE_ALARMS  (YPOS_DATE+HEIGHT_DATE+14)

#define TEMPPRES_SIZE   2
#define XPOS_PRES       10
#define YPOS_PRES       10
#define XPOS_TEMP       XPOS_PRES
#define YPOS_TEMP       (YPOS_PRES+TFT_CHAR_HEIGHT*TEMPPRES_SIZE+4)
#define WIDTH_TEMPPRES  (TFT_CHAR_WIDTH*TEMPPRES_SIZE*8)        //1040 mBar
#define HEIGHT_TEMPPRES (YPOS_TEMP+TFT_CHAR_HEIGHT*TEMPPRES_SIZE+4)


/*
 * 1. temp/press sun/moon
 * 4.  barometric chart
 */
#define MIN_PRESSURE  960
#define MAX_PRESSURE  1040
#define MIN_PRESSURE_STR  "960"
#define MAX_PRESSURE_STR  "1040"
//#define NR_OF_PRESSURES (3*24*3)     // last 3 days, 3 values/hour
#define NR_OF_PRESSURES (3*24*2)     // last 3 days, 2 values/hour
#define XPOS_PRES_GRAPH ((TFT_WIDTH-WIDTH_PRES_GRAPH)/2)
#define YPOS_PRES_GRAPH ((TFT_HEIGHT-HEIGHT_PRES_GRAPH)/2)
#define HEIGHT_PRES_GRAPH (MAX_PRESSURE-MIN_PRESSURE)
#define WIDTH_PRES_GRAPH NR_OF_PRESSURES
//#define WIDTH_PRES_GRAPH 220
#define GRAPHCOLOR   WHITE

char mPrevMinute=0;
DateTime mNow;
DateTime mPrevDate;
const char maandag[] PROGMEM = "maandag";
const char dinsdag[] PROGMEM = "dinsdag";
const char woensdag[] PROGMEM = "woensdag";
const char donderdag[] PROGMEM = "donderdag";
const char vrijdag[] PROGMEM = "vrijdag";
const char zaterdag[] PROGMEM = "zaterdag";
const char zondag[] PROGMEM = "zondag";
const char* const days_table[] PROGMEM = {maandag,dinsdag,woensdag,donderdag,vrijdag,zaterdag,zondag};

bool	mPirState = false;
Timer	mPirTimer;

uint8_t mActiveDisplay = 0;

byte mHistory[NR_OF_PRESSURES];
int  mHistoryIndex = 0;
byte mPrevSampleTime = 0;

double mCurrentTemp=0;
double mCurrentPress=0;


TftSpfd5408 mTft(LCD_CS, LCD_CD, LCD_WR, LCD_RD);
RTC_DS1307 mRtc;
Bmp180     mBmp180;

// defines to set the RTC at the build-time of the executable
#define BUILDTM_YEAR (\
    __DATE__[7] == '?' ? 1900 \
    : (((__DATE__[7] - '0') * 1000 ) \
    + (__DATE__[8] - '0') * 100 \
    + (__DATE__[9] - '0') * 10 \
    + __DATE__[10] - '0'))

#define BUILDTM_MONTH (\
    __DATE__ [2] == '?' ? 1 \
    : __DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 1 : 6) \
    : __DATE__ [2] == 'b' ? 2 \
    : __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 3 : 4) \
    : __DATE__ [2] == 'y' ? 5 \
    : __DATE__ [2] == 'l' ? 7 \
    : __DATE__ [2] == 'g' ? 8 \
    : __DATE__ [2] == 'p' ? 9 \
    : __DATE__ [2] == 't' ? 10 \
    : __DATE__ [2] == 'v' ? 11 \
    : 12)

#define BUILDTM_DAY (\
    __DATE__[4] == '?' ? 1 \
    : ((__DATE__[4] == ' ' ? 0 : \
    ((__DATE__[4] - '0') * 10)) + __DATE__[5] - '0'))

#define BUILDTM_HOUR (\
    __TIME__[0] == '?' ? 1 \
    : ((__TIME__[0] == ' ' ? 0 : \
    ((__TIME__[0] - '0') * 10)) + __TIME__[1] - '0'))

#define BUILDTM_MIN (\
    __TIME__[3] == '?' ? 1 \
    : ((__TIME__[3] == ' ' ? 0 : \
    ((__TIME__[3] - '0') * 10)) + __TIME__[4] - '0'))

void setup(void)
{
	Serial.begin(9600);
	delay(100);
	
	pinMode(PIN_PIR,INPUT);
	
	Wire.begin();
	mRtc.begin();
	mTft.init();
	mBmp180.Init();
	
	for( int i=0; i<NR_OF_PRESSURES; i++)
	{
		mHistory[i++]=0;
		mHistory[i]=HEIGHT_PRES_GRAPH-1;
	}
	mHistoryIndex = 0;
	
	mNow = mRtc.now();
	
	#ifdef ADJUST_TIME_AT_STARTUP
	Serial.print("Adjust RTC to ");
	Serial.print(__DATE__); Serial.print(__TIME__);
	DateTime nu(BUILDTM_YEAR,BUILDTM_MONTH,BUILDTM_DAY,BUILDTM_HOUR,BUILDTM_MIN);
	mRtc.adjust(nu);
	#endif
	
	ActivateDisplay(DISPLAY_TIME);
}


void loop(void)
{
	static unsigned long mPrevTimeTimeMs = 0;

	unsigned long nowMs = millis();
	unsigned long diff = nowMs-mPrevTimeTimeMs;
	
	if (diff>10000)
	{
		mNow = mRtc.now();

		if (mPrevMinute != mNow.mm)
		{
			handleTempPressure();

			// add a new value to the graphics bar every 30 minutes
			byte lNowMinute = mNow.mm;
			if ((lNowMinute==0 || lNowMinute==30))
			{
				registerPressure((byte)mCurrentPress);
			}

			UpdateDisplay(DISPLAY_TIME);
			mPrevMinute = lNowMinute;  // mNow.minute();
		}
		mPrevTimeTimeMs = nowMs;
	}

	/**
	if(digitalRead(PIN_PIR))
	{
		if (!mPirState)	// rising edge
		{
			mPirState = true;
			Serial.println("Pir activated");
			ActivateDisplay(DISPLAY_PRESSURE);
			mPirTimer.Start(1000);
		}
		if (mPirTimer.HasExpired())
		{
			ActivateDisplay(DISPLAY_TIME);
			mPirTimer.Stop();
		}
	}
	else
	{
		if(mPirState)
		{
			Serial.println("Pir deactivated");
			ActivateDisplay(DISPLAY_TIME);
			mPirState = false;
		}
	}
	**/

	delay(100);
}

void NextDisplay()
{
    mActiveDisplay++;
    mActiveDisplay %= MAX_DISPLAYS;
    ActivateDisplay(mActiveDisplay);
}

void UpdateDisplay(byte aDisplay)
{
  if(mActiveDisplay==aDisplay)
  {
    ShowDisplay();
  }
}

void ActivateDisplay(byte aDisplay)
{
  mActiveDisplay = aDisplay;
  mTft.fillScreen(BLACK);
  ShowDisplay();
}

void ShowDisplay()
{
  if(mActiveDisplay==DISPLAY_TIME)
  {
    mPrevDate.Clear();  // force update of date
    showTime(mNow);
    showTempPressure();
  }
  else if(mActiveDisplay==DISPLAY_PRESSURE)
  {
    displayPressureGraph();
  }
}

bool getTempAndPressure(double &lTemp,double &lPress)
{
  if( mBmp180.StartTemperature()!=0 )
  {
    delay(5);
    if( mBmp180.GetTemperature(lTemp)==1 )
    {
      int d = mBmp180.StartPressure(1);    // start for next sample
      if( d>0 )
      {
        delay(d);
        mBmp180.GetPressure(lPress,lTemp);
      }
    }
  }
  return true;
}

void registerPressure(byte lPressure)
{
  if(mHistoryIndex>=NR_OF_PRESSURES)
  {
    // history is 'full', clear oldest value and shift everything else
    // could be faster using memcpy 
    for(int i=0; i<(NR_OF_PRESSURES-1); i++)
    {
      mHistory[i] = mHistory[i+1];
    }
    mHistoryIndex = NR_OF_PRESSURES-1;
  }
  mHistory[mHistoryIndex] = lPressure;
  mHistoryIndex++;
}

void displayPressureGraph()
{
  showTempPressure();

  mTft.drawLine(XPOS_PRES_GRAPH, YPOS_PRES_GRAPH, XPOS_PRES_GRAPH+NR_OF_PRESSURES, YPOS_PRES_GRAPH, GRAY1);    
  mTft.drawLine(XPOS_PRES_GRAPH, YPOS_PRES_GRAPH+HEIGHT_PRES_GRAPH, XPOS_PRES_GRAPH+NR_OF_PRESSURES, YPOS_PRES_GRAPH+HEIGHT_PRES_GRAPH, GRAY1);     
  mTft.setCursor(XPOS_PRES_GRAPH,YPOS_PRES_GRAPH-2*TFT_CHAR_HEIGHT);   
  mTft.print(MIN_PRESSURE_STR);
  mTft.setCursor(XPOS_PRES_GRAPH+NR_OF_PRESSURES-4*TFT_CHAR_WIDTH,YPOS_PRES_GRAPH+HEIGHT_PRES_GRAPH+2);   
  mTft.print(MAX_PRESSURE_STR);
  
  for(int i=0; i<NR_OF_PRESSURES; i++)
  {
    mTft.drawPixel(XPOS_PRES_GRAPH+i, YPOS_PRES_GRAPH+HEIGHT_PRES_GRAPH-mHistory[i], GRAPHCOLOR);
  }
}

void showSunMoon(DateTime aDateTime)
{
  // erase all old information
  mTft.fillRect(XPOS_SUNMOON,YPOS_SUNMOON,WIDTH_SUNMOON,HEIGHT_SUNMOON, BLACK);

  byte lMoonPhase = GetMoonPhase(aDateTime);

  // first draw 'full-moon' 
  mTft.fillCircle(XPOS_MOON,YPOS_SUNMOON+MOON_RADIUS+10, MOON_RADIUS, LIGHTGRAY);   // x,y is center of circle
  // then draw shadow of the earth over it
  int16_t lEarthPos = - (2*lMoonPhase*2*MOON_RADIUS/29);
  if(lMoonPhase>14)
  {
    lEarthPos += 4*MOON_RADIUS;
  }
  mTft.fillCircle(XPOS_MOON+lEarthPos,YPOS_SUNMOON+MOON_RADIUS+10, MOON_RADIUS, BLACK);   // x,y is center of circle

  //mTft.setTextColor(WHITE);

  // Sun rise and set:
  // use top halve of sunny weather icon
  mTft.drawBitmap(XPOS_SUN+8, YPOS_SUN,
                  zon, 
                  WEATHERICON_WIDTH,24,
                  YELLOW,BLACK);
            
  mTft.setCursor(XPOS_SUN,0+36);  
  mTft.print(GetSunRise(aDateTime).GetTimeStr());
  mTft.setCursor(XPOS_SUN,36+20);  
  mTft.print(GetSunSet(aDateTime).GetTimeStr());
}

void handleTempPressure()
{
  getTempAndPressure(mCurrentTemp,mCurrentPress);
}


void showTempPressure()
{
  char str[10];

  mTft.fillRect(XPOS_PRES,YPOS_PRES,WIDTH_TEMPPRES,HEIGHT_TEMPPRES, BLACK);
  //mTft.setTextColor(WHITE);  
  
  mTft.setCursor(XPOS_PRES,YPOS_PRES);
  sprintf(str,"%4d mBar",(int)mCurrentPress);
  mTft.print(str);

  sprintf(str,"%d.%1d C",(int)mCurrentTemp,(int)(mCurrentTemp*10)%10);
  mTft.setCursor(XPOS_TEMP,YPOS_TEMP);
  mTft.print(str);
}

void showTime(DateTime aDateTime)
{
  char str[12];
  char strDate[STRLEN_DATE+1];

  mTft.fillRect(XPOS_TIME,YPOS_TIME,WIDTH_TIME,HEIGHT_TIME, BLACK);
  mTft.setCursor(XPOS_TIME,YPOS_TIME);
  mTft.setTextColor(WHITE);  
  mTft.setTextSize(TIME_SIZE);
  mTft.print(aDateTime.GetTimeStr());
  mTft.setTextSize(2);  // terug naar default
  mTft.setTextColor(WHITE);  

  if ((aDateTime.dayOfWeek() != mPrevDate.dayOfWeek()) ||
      (aDateTime.d != mPrevDate.d) ||
      (aDateTime.m != mPrevDate.m) || 
      (aDateTime.year() != mPrevDate.year()))
  {
    mTft.fillRect(0,YPOS_DATE,TFT_WIDTH,HEIGHT_DATE, BLACK);
    mTft.setCursor(XPOS_DATE,YPOS_DATE);
    strcpy_P(strDate, (char*)pgm_read_word(&(days_table[aDateTime.dayOfWeek()])));
    strcat(strDate,aDateTime.GetDateStr());
    mTft.print(strDate);
    
    mPrevDate = aDateTime;

    showSunMoon(aDateTime);
  }
}
