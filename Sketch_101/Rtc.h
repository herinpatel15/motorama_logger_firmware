#ifndef Rtc.h
#define Rtc.h

#include "RTClib.h"

RTC_DS3231 rtc;

void setupRTC() {
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}
#endif
