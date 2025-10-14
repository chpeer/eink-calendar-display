#include "ha_client.h"
#include "config.h"
#include "sample_data.h"

HAClient::HAClient() {
  // Constructor
}

HAResponse HAClient::fetchCalendarData() {
  #if USE_SAMPLE_DATA
    Serial.println("Using sample data (configured in config.h)");
    return parseSampleData(sampleEventsJson);
  #else
    Serial.println("Fetching from Home Assistant API");
    HAResponse response = fetchFromHA();
    return response;
  #endif
}

HAResponse HAClient::fetchFromHA() {
  HAResponse response;
  response.success = false;

  Serial.println("Connecting to Home Assistant...");

  http.begin(HA_SERVER);
  http.addHeader("Authorization", "Bearer " + String(HA_TOKEN));
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000); // 10 seconds

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String jsonResponse = http.getString();
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);

    if (httpResponseCode == 200) {
      Serial.println("Successfully fetched calendar data from Home Assistant");
      Serial.printf("Response length: %d bytes\n", jsonResponse.length());

      response.success = parseResponse(jsonResponse, response);
    } else {
      Serial.printf("HTTP Error: %d\n", httpResponseCode);
      Serial.println("Response: " + jsonResponse);
    }
  } else {
    Serial.printf("HTTP Request failed: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end();
  return response;
}

HAResponse HAClient::parseSampleData(const String& sampleJson) {
  HAResponse response;
  response.success = parseResponse(sampleJson, response);
  return response;
}

bool HAClient::parseResponse(const String& jsonResponse, HAResponse& response) {
  // Use a larger JSON document to handle more events
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, jsonResponse);
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return false;
  }

  // Extract metadata
  JsonObject attributes = doc["attributes"];
  if (!attributes.isNull()) {
    response.currentDate = attributes["current_date"].as<String>();
    response.currentDay = attributes["current_day"].as<String>();
    response.currentTime = attributes["current_time"].as<String>();
    response.weekStart = attributes["week_start"].as<String>();
    response.period = attributes["period"].as<String>();

    // Extract events array
    JsonArray eventsArray = attributes["events"];
    if (!eventsArray.isNull()) {
      // Clear existing events and reserve space
      response.events.clear();
      response.events.reserve(eventsArray.size());

      Serial.printf("Parsing %d events from API response\n", eventsArray.size());

      for (JsonObject eventObj : eventsArray) {
        CalendarEvent event;
        event.title = eventObj["title"].as<String>();
        event.calendar = eventObj["calendar"].as<String>();

        String startStr = eventObj["start"].as<String>();
        String endStr = eventObj["end"].as<String>();

        // Check if this is a full-day event (no time component)
        bool isFullDay = isFullDayEvent(startStr, endStr);

        // Calculate day numbers for both start and end
        event.startDay = calculateDayNumber(startStr, response.weekStart);
        event.endDay = calculateDayNumber(endStr, response.weekStart);

        if (isFullDay) {
          // Full-day event - use "-" for time display on single-day events
          if (event.startDay == event.endDay) {
            // Single-day full-day event: show as single-day event with "-" as time
            event.isMultiDay = false;
            event.startTime = "-";
            event.endTime = "";
          } else {
            // Multi-day full-day event: show as multi-day event
            event.isMultiDay = true;
            event.startTime = "";
            event.endTime = "";
          }
        } else {
          // Timed event - extract time
          event.startTime = extractTime(startStr);
          event.endTime = extractTime(endStr);

          // Determine if it spans multiple days
          event.isMultiDay = (event.startDay != event.endDay);
        }

        // Only add events that fall within our 14-day calendar view
        if (event.startDay >= 1 && event.startDay <= 14) {
          response.events.push_back(event);
        }
      }

      Serial.printf("Added %d events within calendar view\n", response.events.size());
      return true;
    }
  }

  Serial.println("Failed to extract calendar data from JSON");
  return false;
}

bool HAClient::isFullDayEvent(const String& start, const String& end) {
  // Full-day events have no time component (just date)
  return (start.indexOf('T') == -1 && end.indexOf('T') == -1);
}

int HAClient::calculateDayNumber(const String& dateStr, const String& weekStart) {
  // Extract date portion (handles both "YYYY-MM-DD" and "YYYY-MM-DDTHH:MM:SS+TZ" formats)
  String date = dateStr;
  int tIndex = date.indexOf('T');
  if (tIndex != -1) {
    date = date.substring(0, tIndex);
  }

  // Simple day calculation relative to week start
  // This is a simplified version - you may want to use a proper date library
  int dateYear = date.substring(0, 4).toInt();
  int dateMonth = date.substring(5, 7).toInt();
  int dateDay = date.substring(8, 10).toInt();

  int weekStartYear = weekStart.substring(0, 4).toInt();
  int weekStartMonth = weekStart.substring(5, 7).toInt();
  int weekStartDay = weekStart.substring(8, 10).toInt();

  // Simple calculation for same month
  if (dateYear == weekStartYear && dateMonth == weekStartMonth) {
    return dateDay - weekStartDay + 1;
  }

  // Handle month boundaries (simplified)
  if (dateYear == weekStartYear) {
    if (dateMonth == weekStartMonth + 1) {
      // Next month - assume previous month had 31 days
      return (31 - weekStartDay + 1) + dateDay;
    }
  }

  // Handle year boundary (December to January)
  if (weekStartYear == dateYear - 1 && weekStartMonth == 12 && dateMonth == 1) {
    return (31 - weekStartDay + 1) + dateDay;
  }

  // Default fallback
  return 1;
}

String HAClient::extractTime(const String& datetime) {
  // Extract time from "YYYY-MM-DDTHH:MM:SS+TZ" format
  int tIndex = datetime.indexOf('T');
  if (tIndex == -1) return "";

  String timePart = datetime.substring(tIndex + 1);
  int plusIndex = timePart.indexOf('+');
  if (plusIndex != -1) {
    timePart = timePart.substring(0, plusIndex);
  }

  // Return HH:MM format
  return timePart.substring(0, 5);
}