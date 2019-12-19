#include "Timer.h"

Timer::Timer(uint32_t aTimeMs):
	mStartTimeMs(millis()),
	mTimeMs(aTimeMs),
  mExpired(false),
  mActive(false)
{
  if(aTimeMs!=0)
  {
    Start(aTimeMs);
  }
}

void Timer::Start(uint32_t aTimeMs)
{
	mTimeMs = aTimeMs;
	mStartTimeMs = millis();
  mExpired = false;
  mActive = true;
}

void Timer::Stop()
{
	mStartTimeMs = 0;
  mExpired = false;
  mActive = false;
}

bool Timer::IsActive() const
{
  return mActive;
}

bool Timer::HasExpired()
{
  if(!mActive)
	  mExpired = false;
  else if (millis()>(mStartTimeMs+mTimeMs))
	  mExpired = true;
  return mExpired;
}
