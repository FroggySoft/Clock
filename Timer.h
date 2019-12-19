#pragma once

#include "Arduino.h"

class Timer
{
public:
  Timer(uint32_t aTimeMs=0);

  void Start(uint32_t aTimeMs);
  void Stop();
  bool IsActive() const;
  bool HasExpired();

private:
  uint32_t mStartTimeMs;
  uint32_t mTimeMs;
  bool mExpired;
  bool mActive;
};
