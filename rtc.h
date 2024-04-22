#include "RTClib.h"

RTC_DS3231 rtc;

char daysOfTheWeek[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
int Day;
int Month;
int Year;
int Secs;
int Minutes;
int Hours;
boolean isAm;
String dofweek; // day of week
String myDate;
String myTime;
DateTime now;
DateTime adjsTo;

boolean alarmCheckOn = false;
boolean alarmActiveNow = false;
byte alarmHour = 0;
byte alarmMinute = 0;


uint32_t minutesButtonPressCount;
uint32_t hoursButtonPressCount;


// place time in the global variables from the RTC
void get_time() {
  now = rtc.now();
  Day = now.day();
  Month = now.month();
  Year = now.year();
  Secs = now.second();
  Hours = now.hour();
  isAm = true;
  if (Hours > 12) {
    Hours = Hours - 12;
    isAm = false;
  } else if (Hours == 12) {
    isAm = false;
  } //end if

  Minutes = now.minute();
  dofweek = daysOfTheWeek[now.dayOfTheWeek()];
} //end get_time()

boolean setup_rtc() {
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    return false;
  } //end if
  return true;
}
