#include "utilities.h"
#include "config.h"

#include <esp_sleep.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <ESPmDNS.h>
#include <esp_adc_cal.h>
#include <driver/adc.h>

extern GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT / 2> display;

// WiFi functions
wl_status_t startWiFi(int &wifiRSSI) {
  WiFi.mode(WIFI_STA);
  Serial.printf("%s '%s'\n", TXT_CONNECTING_TO, WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // timeout if WiFi does not connect in WIFI_TIMEOUT ms from now
  unsigned long timeout = millis() + WIFI_TIMEOUT;
  wl_status_t connection_status = WiFi.status();

  while ((connection_status != WL_CONNECTED) && (millis() < timeout)) {
    Serial.print(".");
    delay(50);
    connection_status = WiFi.status();
  }
  Serial.println();

  if (connection_status == WL_CONNECTED) {
    wifiRSSI = WiFi.RSSI(); // get WiFi signal strength now
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.printf("%s '%s'\n", TXT_COULD_NOT_CONNECT_TO, WIFI_SSID);
  }

    // Initialize mDNS for .iot domain resolution
  if (!MDNS.begin("esp32-calendar")) {
    Serial.println("Error setting up mDNS responder");
  }

  // Print network debugging information
  #if DEBUG_LEVEL > 0
    Serial.println("Network Debug Information:");
    Serial.printf("Local IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Gateway IP: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("DNS IP: %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("Subnet Mask: %s\n", WiFi.subnetMask().toString().c_str());
  #endif

  return connection_status;
}

void killWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}
/* Returns battery voltage in millivolts (mv).
 */
uint32_t readBatteryVoltage()
{
  esp_adc_cal_characteristics_t adc_chars;
  // __attribute__((unused)) disables compiler warnings about this variable
  // being unused (Clang, GCC) which is the case when DEBUG_LEVEL == 0.
  esp_adc_cal_value_t val_type __attribute__((unused));

  uint16_t adc_val = analogRead(PIN_BAT_ADC);

  // We will use the eFuse ADC calibration bits, to get accurate voltage
  // readings. The DFRobot FireBeetle Esp32-E V1.0's ADC is 12 bit, and uses
  // 11db attenuation, which gives it a measurable input voltage range of 150mV
  // to 2450mV.
  val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12,
                                      ADC_WIDTH_BIT_12, 1100, &adc_chars);

#if DEBUG_LEVEL >= 1
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
  {
    Serial.println("[debug] ADC Cal eFuse Vref");
  }
  else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
  {
    Serial.println("[debug] ADC Cal Two Point");
  }
  else
  {
    Serial.println("[debug] ADC Cal Default");
  }
#endif

  uint32_t batteryVoltage = esp_adc_cal_raw_to_voltage(adc_val, &adc_chars);
  // DFRobot FireBeetle Esp32-E V1.0 voltage divider (1M+1M), so readings are
  // multiplied by 2.
  batteryVoltage *= 2;
  return batteryVoltage;
} // end readBatteryVoltage


/* Returns battery percentage, rounded to the nearest integer.
 * Takes a voltage in millivolts and uses a sigmoidal approximation to find an
 * approximation of the battery life percentage remaining.
 * 
 * This function contains LGPLv3 code from 
 * <https://github.com/rlogiacco/BatterySense>.
 * 
 * Symmetric sigmoidal approximation
 * <https://www.desmos.com/calculator/7m9lu26vpy>
 *
 * c - c / (1 + k*x/v)^3
 */
uint32_t calcBatPercent(uint32_t v, uint32_t minv, uint32_t maxv)
{
  // slow
  //uint32_t p = 110 - (110 / (1 + pow(1.468 * (v - minv)/(maxv - minv), 6)));

  // steep
  //uint32_t p = 102 - (102 / (1 + pow(1.621 * (v - minv)/(maxv - minv), 8.1)));

  // normal
  uint32_t p = 105 - (105 / (1 + pow(1.724 * (v - minv)/(maxv - minv), 5.5)));
  return p >= 100 ? 100 : p;
} // end calcBatPercent

/* This function sets the builtin LED to LOW and disables it even during deep
 * sleep.
 */
void disableBuiltinLED()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  gpio_hold_en(static_cast<gpio_num_t>(LED_BUILTIN));
  gpio_deep_sleep_hold_en();
  return;
} // end disableBuiltinLED


/* Put esp32 into ultra low-power deep sleep (<11μA).
 * Aligns wake time to the minute. Sleep times defined in config.cpp.
 * timeInfo should already be populated with current time from HA.
 * If timeInfo is empty (error case), falls back to simple fixed sleep.
 */
void beginDeepSleep(unsigned long startTime, tm *timeInfo)
{
  // Check if timeInfo has valid data (year will be 0 if uninitialized)
  if (timeInfo->tm_year == 0) {
    // Error case - no valid time available, use simple fixed sleep
    Serial.println("No valid time for sleep calculation, using fixed SLEEP_DURATION");
    uint64_t sleepDuration = SLEEP_DURATION * 60ULL; // minutes to seconds
    esp_sleep_enable_timer_wakeup(sleepDuration * 1000000ULL);
    Serial.print("Awake for");
    Serial.println(" "  + String((millis() - startTime) / 1000.0, 3) + "s");
    Serial.print("Entering deep sleep for");
    Serial.println(" " + String(sleepDuration) + "s");
    esp_deep_sleep_start();
    return; // Never reached, but explicit
  }

  // timeInfo is already populated from Home Assistant time
  // To simplify sleep time calculations, the current time stored by timeInfo
  // will be converted to time relative to the WAKE_TIME. This way if a
  // SLEEP_DURATION is not a multiple of 60 minutes it can be more trivially,
  // aligned and it can easily be deterimined whether we must sleep for
  // additional time due to bedtime.
  // i.e. when curHour == 0, then timeInfo->tm_hour == WAKE_TIME
  int bedtimeHour = INT_MAX;
  if (BED_TIME != WAKE_TIME)
  {
    bedtimeHour = (BED_TIME - WAKE_TIME + 24) % 24;
  }

  // time is relative to wake time
  int curHour = (timeInfo->tm_hour - WAKE_TIME + 24) % 24;
  const int curMinute = curHour * 60 + timeInfo->tm_min;
  const int curSecond = curHour * 3600
                      + timeInfo->tm_min * 60
                      + timeInfo->tm_sec;
  const int desiredSleepSeconds = SLEEP_DURATION * 60;
  const int offsetMinutes = curMinute % SLEEP_DURATION;
  const int offsetSeconds = curSecond % desiredSleepSeconds;

  // align wake time to nearest multiple of SLEEP_DURATION
  int sleepMinutes = SLEEP_DURATION - offsetMinutes;
  if (desiredSleepSeconds - offsetSeconds < 120
   || offsetSeconds / (float)desiredSleepSeconds > 0.95f)
  { // if we have a sleep time less than 2 minutes OR less 5% SLEEP_DURATION,
    // skip to next alignment
    sleepMinutes += SLEEP_DURATION;
  }

  // estimated wake time, if this falls in a sleep period then sleepDuration
  // must be adjusted
  const int predictedWakeHour = ((curMinute + sleepMinutes) / 60) % 24;

  uint64_t sleepDuration;
  if (predictedWakeHour < bedtimeHour)
  {
    sleepDuration = sleepMinutes * 60 - timeInfo->tm_sec;
  }
  else
  {
    const int hoursUntilWake = 24 - curHour;
    sleepDuration = hoursUntilWake * 3600ULL
                    - (timeInfo->tm_min * 60ULL + timeInfo->tm_sec);
  }

  // add extra delay to compensate for esp32's with fast RTCs.
  sleepDuration += 3ULL;
  sleepDuration *= 1.0015f;

  esp_sleep_enable_timer_wakeup(sleepDuration * 1000000ULL);
  Serial.print("Awake for");
  Serial.println(" "  + String((millis() - startTime) / 1000.0, 3) + "s");
  Serial.print("Entering deep sleep for");
  Serial.println(" " + String(sleepDuration) + "s");
  esp_deep_sleep_start();
} // end beginDeepSleep


/* Parses Home Assistant date and time strings into a tm struct.
 * Date format: "YYYY-MM-DD"
 * Time format: "HH:MM:SS"
 * Returns true if successful, false otherwise.
 */
bool parseHADateTime(const String &date, const String &time, tm *timeInfo)
{
  if (date.length() < 10 || time.length() < 8) {
    Serial.println("Invalid date or time format from HA");
    return false;
  }

  // Parse date: "YYYY-MM-DD"
  timeInfo->tm_year = date.substring(0, 4).toInt() - 1900; // tm_year is years since 1900
  timeInfo->tm_mon = date.substring(5, 7).toInt() - 1;     // tm_mon is 0-11
  timeInfo->tm_mday = date.substring(8, 10).toInt();

  // Parse time: "HH:MM:SS"
  timeInfo->tm_hour = time.substring(0, 2).toInt();
  timeInfo->tm_min = time.substring(3, 5).toInt();
  timeInfo->tm_sec = time.substring(6, 8).toInt();

  // Calculate day of week (0=Sunday, 6=Saturday)
  // Using Zeller's congruence algorithm
  int year = timeInfo->tm_year + 1900;
  int month = timeInfo->tm_mon + 1;
  int day = timeInfo->tm_mday;

  if (month < 3) {
    month += 12;
    year--;
  }

  int q = day;
  int m = month;
  int k = year % 100;
  int j = year / 100;

  int h = (q + ((13 * (m + 1)) / 5) + k + (k / 4) + (j / 4) - (2 * j)) % 7;
  // Convert Zeller's result (0=Saturday) to tm format (0=Sunday)
  timeInfo->tm_wday = ((h + 6) % 7);

  // Day of year (approximate - doesn't account for leap years perfectly)
  timeInfo->tm_yday = 0; // Not critical for sleep calculations
  timeInfo->tm_isdst = -1; // Let system determine DST

  Serial.printf("Parsed HA time: %04d-%02d-%02d %02d:%02d:%02d\n",
                timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday,
                timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);

  return true;
} // end parseHADateTime