#ifndef HA_CLIENT_H
#define HA_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include "drawing.h"

// Home Assistant API response structure
struct HAResponse {
  String currentDate;
  String currentDay;
  String currentTime;
  String weekStart;
  String period;
  std::vector<CalendarEvent> events;
  bool success;
};

// Main HA client class
class HAClient {
public:
  HAClient();

  // Main method - fetches calendar data (from HA or sample based on config)
  HAResponse fetchCalendarData();

  // Parse sample data for fallback
  HAResponse parseSampleData(const String& sampleJson);

private:
  // Fetch data from Home Assistant API
  HAResponse fetchFromHA();

  // Parse JSON response and extract events
  bool parseResponse(const String& jsonResponse, HAResponse& response);
  HTTPClient http;

  // Helper functions
  bool isFullDayEvent(const String& start, const String& end);
  int calculateDayNumber(const String& dateStr, const String& weekStart);
  String extractTime(const String& datetime);
};

#endif // HA_CLIENT_H