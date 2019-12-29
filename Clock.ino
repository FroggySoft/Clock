#include "Adafruit_GFX.h"    // Core graphics library
#include "TftSpfd5408.h" // Hardware-specific library
#include <Wire.h>
#include "DateTime.h"
#include "RTClib.h"
#include "Bmp180.h"
#include "timer.h"
#include "images/zon.h"

//#define ADJUST_TIME_AT_STARTUP
#define USE_SWITCH

#define DEBUG_INIT	Serial.begin(9600)
#define DEBUG_PRN		Serial.print
#define DEBUG_PRNLN	Serial.println
//#define DEBUG_INIT	//
//#define DEBUG_PRN	//
//#define DEBUG_PRNLN	// 

#include <Arduino.h> // capital A so it is error prone on case-sensitive filesystems

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

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
#define	BLACK   	0x0000
#define	BLUE    	0x001F
#define LIGHTBLUE    0x187F
#define	RED     	0xF800
#define LIGHTRED 	0xFACB
#define	GREEN   	0x07E0
#define CYAN    	0x07FF
#define MAGENTA 	0xF81F
#define YELLOW  	0xFFE0
#define WHITE   	0xFFFF
#define GRAY1   	0x38E7
#define GRAY    	0xD69A
#define LIGHTGRAY	0xE71C

#define TFT_WIDTH  320
#define TFT_HEIGHT 240

#define TFT_CHAR_WIDTH  6
#define TFT_CHAR_HEIGHT 8

// position and sizes of displayed items
#define WEATHERICON_WIDTH      48
#define WEATHERICON_HEIGHT     48
#define MOON_RADIUS 	 16

#define XPOS_SUNMOON     (TFT_WIDTH-WIDTH_SUNMOON)
#define YPOS_SUNMOON     0
#define XPOS_MOON        (XPOS_SUNMOON + (WIDTH_SUNMOON - WIDTH_MOON)/2)
#define YPOS_MOON        (YPOS_SUN+HEIGHT_SUN)
#define WIDTH_MOON 	     (2*MOON_RADIUS)
#define HEIGHT_MOON      (2*MOON_RADIUS)
#define XPOS_SUN         (XPOS_SUNMOON)
#define YPOS_SUN         0
#define WIDTH_SUN 	     (TFT_CHAR_WIDTH*DATE_SIZE*5)
#define HEIGHT_SUN       (WEATHERICON_HEIGHT+2*TFT_CHAR_HEIGHT*DATE_SIZE)
#define WIDTH_SUNMOON    (WIDTH_SUN)
#define HEIGHT_SUNMOON   (HEIGHT_SUN + HEIGHT_MOON)

#define TIME_SIZE   	10
#define STRLEN_TIME 	5
#define WIDTH_TIME  	(TFT_CHAR_WIDTH*TIME_SIZE*STRLEN_TIME)
#define HEIGHT_TIME 	(TFT_CHAR_HEIGHT*TIME_SIZE)
#define DATE_SIZE   	2
#define STRLEN_DATE 	(10+12)
#define WIDTH_DATE  	(TFT_CHAR_WIDTH*DATE_SIZE*STRLEN_DATE)
#define HEIGHT_DATE 	(TFT_CHAR_HEIGHT*DATE_SIZE)
#define XPOS_TIME   	((TFT_WIDTH-WIDTH_TIME)/2)
#define YPOS_TIME   	(TFT_HEIGHT-HEIGHT_TIME-HEIGHT_DATE-1)

#define XPOS_DATE   	((TFT_WIDTH-WIDTH_DATE)/2)
#define YPOS_DATE   	(YPOS_TIME+HEIGHT_TIME) // 300

#define TEMPPRES_SIZE   2
#define XPOS_TEMPPRES   10
#define YPOS_TEMPPRES   (HEIGHT_PRES_GRAPH+10)
#define WIDTH_TEMPPRES	(TFT_CHAR_WIDTH*TEMPPRES_SIZE*16)        //"1040 mBar 88.8 C" max string = 16 characters
#define HEIGHT_TEMPPRES (TFT_CHAR_HEIGHT*TEMPPRES_SIZE+4)

#define MIN_PRESSURE  		960
#define MAX_PRESSURE  		1040
#define MIN_TEMPERATURE		10
#define MAX_TEMPERATURE		30
#define SAMPLES_A_DAY		(24*3)
#define NR_OF_DAYS			3
#define NR_OF_SAMPLES 		(SAMPLES_A_DAY*NR_OF_DAYS)
#define XPOS_PRES_GRAPH 	0
#define YPOS_PRES_GRAPH 	0
#define HEIGHT_PRES_GRAPH 	(MAX_PRESSURE-MIN_PRESSURE)
#define WIDTH_PRES_GRAPH 	(NR_OF_SAMPLES+TFT_CHAR_WIDTH*2)
#define PRESSURE_COLOR   	LIGHTBLUE
#define TEMPERATURE_COLOR   LIGHTRED
#define LABEL_SIZE   		1

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

typedef struct
{
	byte pressure;
	byte temperature;
} History;

History mHistory[NR_OF_SAMPLES];
int  	mHistoryIndex = 0;
byte 	mPrevSampleTime = 0;

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
	DEBUG_INIT;
	delay(100);
	
	Wire.begin();
	mRtc.begin();
	mTft.init();
	mBmp180.Init();
	
	for( int i=0; i<NR_OF_SAMPLES; i++)
	{
		mHistory[i].temperature=200;	// outside range
		mHistory[i].pressure=200;
	}
	mHistoryIndex = 0;
	
	mNow = mRtc.now();
	
	#ifdef ADJUST_TIME_AT_STARTUP
	DEBUG_PRN("Adjust RTC to ");
	DEBUG_PRN(__DATE__); DEBUG_PRN(__TIME__);
	DateTime nu(BUILDTM_YEAR,BUILDTM_MONTH,BUILDTM_DAY,BUILDTM_HOUR,BUILDTM_MIN);
	mRtc.adjust(nu);
	#endif
	
	mTft.fillScreen(BLACK);
	
	handleTempPressure();
	showTempPressure();
	showTime(mNow);
	displayGraph();
}


void loop(void)
{
	handleTempPressure();
	showTempPressure();
	
	mNow = mRtc.now();

	if (mPrevMinute != mNow.mm)
	{
	    showTime(mNow);

		// add a new value to the graphics bar every 20 minutes
		if ((mNow.mm==0 || mNow.mm==20 || mNow.mm==40))
		{
			registerData(mCurrentPress,mCurrentTemp);
			displayGraph();
		}
		mPrevMinute = mNow.mm;
	}

	delay(10000);	// check once every 10 seconds
}


bool getTempAndPressure(double& lTemp,double& lPress)
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

void registerData(double aPressure, double aTemperature)
{
	double p = max(aPressure,MIN_PRESSURE);
	p = min(p,MAX_PRESSURE);
	p -= MIN_PRESSURE;

	double t = max(aTemperature,MIN_TEMPERATURE);
	t = min(t,MAX_TEMPERATURE);
	t -= MIN_TEMPERATURE;
	t *= (MAX_PRESSURE-MIN_PRESSURE)/(MAX_TEMPERATURE-MIN_TEMPERATURE);

	if(mHistoryIndex>=NR_OF_SAMPLES)
	{
		// history is 'full', clear oldest value and shift everything else
		// could be faster using memcpy 
		for(int i=0; i<(NR_OF_SAMPLES-1); i++)
		{
			mHistory[i] = mHistory[i+1];
		}
		mHistoryIndex = NR_OF_SAMPLES-1;
	}
	mHistory[mHistoryIndex].pressure = p;
	mHistory[mHistoryIndex].temperature = t;
	DEBUG_PRNLN(p);
	DEBUG_PRNLN(t);
	mHistoryIndex++;
}

void displayGraph()
{
	char str[12];
	// whole grafic is shifted 2 characters to the right for display of temperature label
	byte xPos = XPOS_PRES_GRAPH + 2*TFT_CHAR_HEIGHT*LABEL_SIZE;

	mTft.fillRect(xPos,YPOS_PRES_GRAPH,WIDTH_PRES_GRAPH,HEIGHT_PRES_GRAPH, BLACK);
	
	mTft.drawLine(xPos, YPOS_PRES_GRAPH, xPos+NR_OF_SAMPLES, YPOS_PRES_GRAPH, LIGHTGRAY);
	mTft.drawLine(xPos, YPOS_PRES_GRAPH+HEIGHT_PRES_GRAPH, xPos+NR_OF_SAMPLES, YPOS_PRES_GRAPH+HEIGHT_PRES_GRAPH, LIGHTGRAY);
	for (int i=0; i<4; i++)
	{
		mTft.drawLine(xPos + i*SAMPLES_A_DAY, YPOS_PRES_GRAPH, xPos+i*SAMPLES_A_DAY, YPOS_PRES_GRAPH+HEIGHT_PRES_GRAPH, LIGHTGRAY);
	}	
	mTft.setTextSize(LABEL_SIZE);
	mTft.setTextColor(PRESSURE_COLOR);
	mTft.setCursor(xPos+NR_OF_SAMPLES,YPOS_PRES_GRAPH);   
	sprintf(str,"%4d",MAX_PRESSURE);
	mTft.print(str);
	mTft.setCursor(xPos+NR_OF_SAMPLES,YPOS_PRES_GRAPH+HEIGHT_PRES_GRAPH-TFT_CHAR_HEIGHT*LABEL_SIZE);
	sprintf(str,"%4d",MIN_PRESSURE);
	mTft.print(str);

	mTft.setTextColor(TEMPERATURE_COLOR);
	mTft.setCursor(XPOS_PRES_GRAPH,YPOS_PRES_GRAPH);   
	sprintf(str,"%2d",MAX_TEMPERATURE);
	mTft.print(str);
	mTft.setCursor(XPOS_PRES_GRAPH,YPOS_PRES_GRAPH+HEIGHT_PRES_GRAPH-TFT_CHAR_HEIGHT*LABEL_SIZE);
	sprintf(str,"%2d",MIN_TEMPERATURE);
	mTft.print(str);
	
	for(int i=0; i<NR_OF_SAMPLES; i++)
	{
		if (mHistory[i].pressure<100)
			mTft.drawPixel(xPos+i, YPOS_PRES_GRAPH+HEIGHT_PRES_GRAPH-mHistory[i].pressure, PRESSURE_COLOR);
		if (mHistory[i].temperature<100)
			mTft.drawPixel(xPos+i, YPOS_PRES_GRAPH+HEIGHT_PRES_GRAPH-mHistory[i].temperature, TEMPERATURE_COLOR);
	}
}

void showSunMoon(DateTime aDateTime)
{
  // erase all old information
  mTft.fillRect(XPOS_SUNMOON,YPOS_SUNMOON,WIDTH_SUNMOON,HEIGHT_SUNMOON, BLACK);

  byte moonPhase = GetMoonPhase(aDateTime);
  DEBUG_PRN("Maan="); DEBUG_PRNLN(moonPhase);

  // first draw 'full-moon' 
  mTft.fillCircle(XPOS_MOON+MOON_RADIUS,YPOS_MOON+MOON_RADIUS, MOON_RADIUS, LIGHTGRAY);   // x,y is center of circle
  // then draw shadow of the earth over it
  int16_t earthPos = - (2*moonPhase*2*MOON_RADIUS/29);
  if(moonPhase>14)
  {
    earthPos += 4*MOON_RADIUS;
  }
  mTft.fillCircle(XPOS_MOON+MOON_RADIUS+earthPos,YPOS_MOON+MOON_RADIUS, MOON_RADIUS, BLACK);   // x,y is center of circle

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
  char str[20];
  sprintf(str,"%d.%1d C  %4d mBar", (int)mCurrentTemp,(int)(mCurrentTemp*10)%10, (int)mCurrentPress );

  mTft.fillRect(XPOS_TEMPPRES,YPOS_TEMPPRES,WIDTH_TEMPPRES,HEIGHT_TEMPPRES, BLACK);
  mTft.setTextColor(WHITE);  
  mTft.setTextSize(TEMPPRES_SIZE);
  
  mTft.setCursor(XPOS_TEMPPRES,YPOS_TEMPPRES);
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
  mTft.print(aDateTime.GetTimeStr(true));
  mTft.setTextSize(2);  // terug naar default
  mTft.setTextColor(WHITE);  

  if ((aDateTime.dayOfWeek() != mPrevDate.dayOfWeek()) ||
      (aDateTime.d != mPrevDate.d) ||
      (aDateTime.m != mPrevDate.m) || 
      (aDateTime.y != mPrevDate.y))
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
