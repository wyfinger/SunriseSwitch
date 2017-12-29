/*
  Sun Rise/Set Switch Project
  Wyfinger, 2017-12-23.
  https://github.com/wyfinger/SunriseSwitch
*/

#include <Wire.h>
#include <math.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

const int    PIN_RELAY = 4;
const double LATITUDE  = 43.1096;
const double LONGITUDE = 131.9551;
const double ZENITH    = 91;
const double UTCshift  = 10;

TimeElements tm;

void setup() {
  // prepare relay pin
  pinMode(PIN_RELAY, OUTPUT);

  // print project info and settings
  Serial.begin(9600);
  Serial.println();
  Serial.println("Sun Rise/Set Switch Project");
  Serial.println("Wyfinger, 2017-12-23");
  Serial.println("https://github.com/wyfinger/SunriseSwitch");
  Serial.println("------");
  Serial.print("PIN_RELAY = "); Serial.print(PIN_RELAY); Serial.print(", ");
  Serial.print("LATITUDE = "); Serial.print(LATITUDE); Serial.print(", ");
  Serial.print("LONGITUDE = "); Serial.print(LONGITUDE); Serial.print(", ");
  Serial.print("ZENITH = "); Serial.print(ZENITH); Serial.print(", ");
  Serial.print("UTCshift = "); Serial.print(UTCshift); Serial.println(";");
}

void loop() {
  if (RTC.read(tm)) {
    // info to console
    Serial.print("Date and time = ");
    Serial.print(tmYearToCalendar(tm.Year)); Serial.print("-"), Serial.print(tm.Month); Serial.print("-"); Serial.print(tm.Day);
    Serial.print(" "); Serial.print(tm.Hour), Serial.print(":"); Serial.print(tm.Minute); Serial.print(":"); Serial.print(tm.Second);
    Serial.println(";");
    // relay action
    digitalWrite(PIN_RELAY, !isDayLight(tm, ZENITH, LATITUDE, LONGITUDE, UTCshift));
    Serial.println("------");
  } else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped. Please run the SetTime");
      Serial.println();
    } else {
      Serial.println("DS1307 read error! Please check the circuitry.");
      Serial.println();
    }
  }
  delay(5000);
}

bool isDayLight(TimeElements tm, float zenith, double latitude, double longitude, int UTCshift) {
  // calculate day of year
  int dayOfYear = calculateDayOfYear(tm.Day, tm.Month, tmYearToCalendar(tm.Year));
  // calculate time of Sun rise and set
  double lngHour = longitude/15;
  double t_rise = dayOfYear+(6-lngHour)/24;
  double t_set = dayOfYear+(18-lngHour)/24;
  double M_rise = 0.9856*t_rise-3.289;
  double M_set = 0.9856*t_set-3.289;
  double L_rise = M_rise + 1.916*sin((M_rise*M_PI)/180) + 0.02*sin((2*M_rise*M_PI)/180) + 282.634;
  double L_set =  M_set  + 1.916*sin((M_set*M_PI)/180)  + 0.02*sin((2*M_set*M_PI)/180)  + 282.634;
  if (L_rise >= 360) {
    L_rise -= 360;
  } else if (L_rise < 0) {
    L_rise += 360;
  }
  if (L_set >= 360) {
    L_set -= 360;
  } else if (L_set < 0) {
    L_set += 360;
  }
  double RA_rise = (180*atan(0.91764*tan((L_rise*M_PI)/180)))/M_PI;
  double LQuadrant = 90 * floor(L_rise/90);
  double RQuadrant = 90 * floor(RA_rise/90);
  RA_rise = (RA_rise + LQuadrant - RQuadrant) / 15;
  double RA_set = (180*atan(0.91764*tan((L_set*M_PI)/180)))/M_PI;
  LQuadrant = 90 * floor(L_set/90);
  RQuadrant = 90 * floor(RA_set/90);
  RA_set = (RA_set + LQuadrant - RQuadrant) / 15;
  double sinDec_rise = 0.39782*sin(L_rise*M_PI/180);
  double sinDec_set = 0.39782*sin(L_set*M_PI/180);
  double cosDec_rise = cos(asin(sinDec_rise));
  double cosDec_set = cos(asin(sinDec_set));
  double cosH_rise = (cos(zenith*M_PI/180) - sinDec_rise*sin(latitude*M_PI/180)) / (cosDec_rise * cos(latitude*M_PI/180));
  double cosH_set = (cos(zenith*M_PI/180) - sinDec_set*sin(latitude*M_PI/180)) / (cosDec_set * cos(latitude*M_PI/180));
  double H_rise = (360 - (180 * acos(cosH_rise)) / M_PI) / 15;
  double H_set = ((180 * acos(cosH_rise)) / M_PI) / 15;
  double T_rise = H_rise+RA_rise-0.06571*t_rise-6.622;
  double T_set = H_set+RA_set-0.06571*t_set-6.622;
  double RT_rise = T_rise-lngHour+UTCshift;
  double RT_set = T_set-lngHour+UTCshift;
  if (RT_rise >= 24) {
    RT_rise -= 24;
  } else if (RT_rise < 0) {
    RT_rise += 24;
  }
  if (RT_set >= 24) {
    RT_set -= 24;
  } else if (RT_set < 0) {
    RT_set += 24;
  }
  double currTime = tm.Hour + (tm.Minute * 0.0166666);
  Serial.print("SunRise = "); Serial.print((int)RT_rise); Serial.print(":"); Serial.print((int)(60 * (RT_rise - floor(RT_rise)))); Serial.println(";");
  Serial.print("SunSet = ");  Serial.print((int)RT_set);  Serial.print(":"); Serial.print((int)(60 * (RT_set - floor(RT_set)))); Serial.println(";");

  return (currTime > RT_rise) && (currTime < RT_set); // check tome zone!!!
}

int calculateDayOfYear(int day, int month, int year) {
  // Given a day, month, and year (4 digit), returns
  // the day of year. Errors return 999.
  // https://gist.github.com/jrleeman/3b7c10712112e49d8607
  int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  // Verify we got a 4-digit year
  if (year < 1000) {
    return 999;
  }
  // Check if it is a leap year, this is confusing business
  // See: https://support.microsoft.com/en-us/kb/214019
  if (year%4  == 0) {
    if (year%100 != 0) {
      daysInMonth[1] = 29;
    }
    else {
      if (year%400 == 0) {
        daysInMonth[1] = 29;
      }
    }
   }
  // Make sure we are on a valid day of the month
  if (day < 1) {
    return 999;
  } else if (day > daysInMonth[month-1]) {
    return 999;
  }
  int doy = 0;
  for (int i = 0; i < month - 1; i++) {
    doy += daysInMonth[i];
  }
  doy += day;

  return doy;
}