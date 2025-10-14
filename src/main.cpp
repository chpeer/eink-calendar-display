#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <ArduinoJson.h>
#include "drawing.h"
#include "utilities.h"
#include "icons.h"
#include "config.h"
#include "ha_client.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_sleep.h>
#include <Preferences.h>
#include <vector>

// Display instance - using GxEPD2_750c_Z08 for GDEY075Z08
// Pin definitions and configuration are in config.h
GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT / 2> display(GxEPD2_750c_Z08(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// Home Assistant client
HAClient haClient;

// Non-volatile storage for configuration
Preferences prefs;

/* Program entry point.
 */
void setup()
{
  unsigned long startTime = millis();
  Serial.begin(SERIAL_BAUDRATE);

  disableBuiltinLED();

  // Open namespace for read/write to non-volatile storage
  prefs.begin("calendar", false);

#if BATTERY_MONITORING
  uint32_t batteryVoltage = readBatteryVoltage();
  Serial.print(TXT_BATTERY_VOLTAGE);
  Serial.println(": " + String(batteryVoltage) + "mv");

  // When the battery is low, the display should be updated to reflect that, but
  // only the first time we detect low voltage. The next time the display will
  // refresh is when voltage is no longer low. To keep track of that we will
  // make use of non-volatile storage.
  bool lowBat = prefs.getBool("lowBat", false);

  // low battery, deep sleep now
  if (batteryVoltage <= LOW_BATTERY_VOLTAGE)
  {
    if (lowBat == false)
    { // battery is now low for the first time
      prefs.putBool("lowBat", true);
      prefs.end();
      initDisplay();
      do
      {
        drawError(battery_alert_0deg_196x196, TXT_LOW_BATTERY);
      } while (display.nextPage());
      powerOffDisplay();
    }

    if (batteryVoltage <= CRIT_LOW_BATTERY_VOLTAGE)
    { // critically low battery
      // don't set esp_sleep_enable_timer_wakeup();
      // We won't wake up again until someone manually presses the RST button.
      Serial.println(TXT_CRIT_LOW_BATTERY_VOLTAGE);
      Serial.println(TXT_HIBERNATING_INDEFINITELY_NOTICE);
    }
    else if (batteryVoltage <= VERY_LOW_BATTERY_VOLTAGE)
    { // very low battery
      esp_sleep_enable_timer_wakeup(VERY_LOW_BATTERY_SLEEP_INTERVAL
                                    * 60ULL * 1000000ULL);
      Serial.println(TXT_VERY_LOW_BATTERY_VOLTAGE);
      Serial.print(TXT_ENTERING_DEEP_SLEEP_FOR);
      Serial.println(" " + String(VERY_LOW_BATTERY_SLEEP_INTERVAL) + "min");
    }
    else
    { // low battery
      esp_sleep_enable_timer_wakeup(LOW_BATTERY_SLEEP_INTERVAL
                                    * 60ULL * 1000000ULL);
      Serial.println(TXT_LOW_BATTERY_VOLTAGE);
      Serial.print(TXT_ENTERING_DEEP_SLEEP_FOR);
      Serial.println(" " + String(LOW_BATTERY_SLEEP_INTERVAL) + "min");
    }
    esp_deep_sleep_start();
  }
  // battery is no longer low, reset variable in non-volatile storage
  if (lowBat == true)
  {
    prefs.putBool("lowBat", false);
  }
#else
  uint32_t batteryVoltage = UINT32_MAX;
#endif

  // All data should have been loaded from NVS. Close filesystem.
  prefs.end();

  String refreshTimeStr = {};
  tm timeInfo = {};


    // START WIFI
  int wifiRSSI = 0; // “Received Signal Strength Indicator"
  wl_status_t wifiStatus = startWiFi(wifiRSSI);
  if (wifiStatus != WL_CONNECTED)
  { // WiFi Connection Failed
    killWiFi();
    initDisplay();
    if (wifiStatus == WL_NO_SSID_AVAIL)
    {
      Serial.println(TXT_NETWORK_NOT_AVAILABLE);
      do
      {
        drawError(wifi_x_196x196, TXT_NETWORK_NOT_AVAILABLE);
      } while (display.nextPage());
    }
    else
    {
      Serial.println(TXT_WIFI_CONNECTION_FAILED);
      do
      {
        drawError(wifi_x_196x196, TXT_WIFI_CONNECTION_FAILED);
      } while (display.nextPage());
    }
    powerOffDisplay();
    beginDeepSleep(startTime, &timeInfo);
  }

  // Fetch calendar data (HA API or sample data based on config)
  Serial.println("Fetching calendar data...");
  HAResponse response = haClient.fetchCalendarData();

  // Disconnect WiFi to save power
  killWiFi();

  if (!response.success) {
    Serial.println("Failed to fetch calendar data");

    // Display error on screen
    initDisplay();
    do {
      drawError(wifi_x_196x196, "Calendar Data Error", "Check configuration");
    } while (display.nextPage());
    powerOffDisplay();

    // Sleep for 2 minutes before retry
    esp_sleep_enable_timer_wakeup(2 * 60 * 1000000ULL);
    esp_deep_sleep_start();
  }

  Serial.printf("Successfully loaded %d events\n", response.events.size());

  // Parse time from Home Assistant response (no need for NTP!)
  if (!parseHADateTime(response.currentDate, response.currentTime, &timeInfo)) {
    Serial.println("Failed to parse time from Home Assistant");

    // Display error on screen
    initDisplay();
    do {
      drawError(wi_time_4_196x196, TXT_TIME_SYNCHRONIZATION_FAILED);
    } while (display.nextPage());
    powerOffDisplay();

    // Sleep for 2 minutes before retry
    esp_sleep_enable_timer_wakeup(2 * 60 * 1000000ULL);
    esp_deep_sleep_start();
  }

  // Format refresh time string from response
  refreshTimeStr = response.currentTime;

    // RENDER FULL REFRESH
  initDisplay();
  do
  {
   // Draw calendar grid with events using response object directly
  drawCalendar(response.events, response.currentDate, response.currentDay,
                   response.currentTime, response.weekStart);
    drawStatusBar(refreshTimeStr, wifiRSSI, batteryVoltage);
  } while (display.nextPage());
  powerOffDisplay();

  // DEEP SLEEP
  beginDeepSleep(startTime, &timeInfo);
}

void loop() {
  // This should never be reached because the device enters deep sleep
  // If we get here, something went wrong, so restart
  Serial.println("Unexpected loop execution - restarting");
  delay(1000);
  ESP.restart();
}