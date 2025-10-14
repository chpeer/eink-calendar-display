#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>
#include <WiFi.h>

// WiFi functions
wl_status_t startWiFi(int &wifiRSSI);
void killWiFi();

// Battery monitoring functions
uint32_t readBatteryVoltage();
uint32_t calcBatPercent(uint32_t v, uint32_t minv, uint32_t maxv);

// Power management functions
void beginDeepSleep(unsigned long startTime, tm *timeInfo);
void powerOffDisplay();
void disableBuiltinLED();

// Time parsing functions
bool parseHADateTime(const String &date, const String &time, tm *timeInfo);

#endif
