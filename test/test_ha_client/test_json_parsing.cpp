#include <unity.h>
#include <ArduinoJson.h>

#ifndef UNIT_TEST
#include <Arduino.h>
#else
#include <string>
#include <vector>
#include <cstring>

// Mock Arduino String class for native testing
class String {
private:
    std::string data;

public:
    String() : data("") {}
    String(const char* str) : data(str ? str : "") {}
    String(const std::string& str) : data(str) {}
    String(int val) : data(std::to_string(val)) {}

    size_t length() const { return data.length(); }
    const char* c_str() const { return data.c_str(); }

    String substring(size_t start, size_t end = std::string::npos) const {
        if (end == std::string::npos) {
            return String(data.substr(start).c_str());
        }
        return String(data.substr(start, end - start).c_str());
    }

    int toInt() const {
        return std::atoi(data.c_str());
    }

    int indexOf(char ch) const {
        size_t pos = data.find(ch);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }

    int indexOf(const char* str) const {
        size_t pos = data.find(str);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }

    bool operator==(const char* str) const {
        return data == str;
    }

    bool operator==(const String& other) const {
        return data == other.data;
    }
};

// Mock Serial for native testing
class SerialMock {
public:
    void println(const char* str) {}
    void println(const String& str) {}
    void printf(const char* format, ...) {}
    void print(const char* str) {}
};

SerialMock Serial;
#endif

// Copy CalendarEvent struct
struct CalendarEvent {
    String title;
    int startDay;
    int endDay;
    String startTime;
    String endTime;
    String calendar;
    bool isMultiDay;
};

// Copy HAResponse struct
struct HAResponse {
    String currentDate;
    String currentDay;
    String currentTime;
    String weekStart;
    String period;
    std::vector<CalendarEvent> events;
    bool success;
};

// Copy helper functions from HAClient
bool isFullDayEvent(const String& start, const String& end) {
    return (start.indexOf('T') == -1 && end.indexOf('T') == -1);
}

int calculateDayNumber(const String& dateStr, const String& weekStart) {
    String date = dateStr;
    int tIndex = date.indexOf('T');
    if (tIndex != -1) {
        date = date.substring(0, tIndex);
    }

    int dateYear = date.substring(0, 4).toInt();
    int dateMonth = date.substring(5, 7).toInt();
    int dateDay = date.substring(8, 10).toInt();

    int weekStartYear = weekStart.substring(0, 4).toInt();
    int weekStartMonth = weekStart.substring(5, 7).toInt();
    int weekStartDay = weekStart.substring(8, 10).toInt();

    if (dateYear == weekStartYear && dateMonth == weekStartMonth) {
        return dateDay - weekStartDay + 1;
    }

    if (dateYear == weekStartYear) {
        if (dateMonth == weekStartMonth + 1) {
            return (31 - weekStartDay + 1) + dateDay;
        }
    }

    if (weekStartYear == dateYear - 1 && weekStartMonth == 12 && dateMonth == 1) {
        return (31 - weekStartDay + 1) + dateDay;
    }

    return 1;
}

String extractTime(const String& datetime) {
    int tIndex = datetime.indexOf('T');
    if (tIndex == -1) return "";

    String timePart = datetime.substring(tIndex + 1);
    int plusIndex = timePart.indexOf('+');
    if (plusIndex != -1) {
        timePart = timePart.substring(0, plusIndex);
    }

    return timePart.substring(0, 5);
}

// Copy parseResponse function from HAClient
bool parseResponse(const String& jsonResponse, HAResponse& response) {
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, jsonResponse.c_str());
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }

    JsonObject attributes = doc["attributes"];
    if (!attributes.isNull()) {
        response.currentDate = attributes["current_date"].as<const char*>();
        response.currentDay = attributes["current_day"].as<const char*>();
        response.currentTime = attributes["current_time"].as<const char*>();
        response.weekStart = attributes["week_start"].as<const char*>();
        response.period = attributes["period"].as<const char*>();

        JsonArray eventsArray = attributes["events"];
        if (!eventsArray.isNull()) {
            response.events.clear();
            response.events.reserve(eventsArray.size());

            Serial.printf("Parsing %d events from API response\n", eventsArray.size());

            for (JsonObject eventObj : eventsArray) {
                CalendarEvent event;
                event.title = eventObj["title"].as<const char*>();
                event.calendar = eventObj["calendar"].as<const char*>();

                String startStr = eventObj["start"].as<const char*>();
                String endStr = eventObj["end"].as<const char*>();

                bool isFullDay = isFullDayEvent(startStr, endStr);

                event.startDay = calculateDayNumber(startStr, response.weekStart);
                event.endDay = calculateDayNumber(endStr, response.weekStart);

                if (isFullDay) {
                    if (event.startDay == event.endDay) {
                        event.isMultiDay = false;
                        event.startTime = "-";
                        event.endTime = "";
                    } else {
                        event.isMultiDay = true;
                        event.startTime = "";
                        event.endTime = "";
                    }
                } else {
                    event.startTime = extractTime(startStr);
                    event.endTime = extractTime(endStr);
                    event.isMultiDay = (event.startDay != event.endDay);
                }

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

// Sample data from sample_data.cpp
const char* sampleEventsJson = R"json({
  "entity_id": "sensor.esp32_calendar_data",
  "state": "23",
  "attributes": {
    "events": [
      {
        "title": "House cleaning service",
        "start": "2025-01-08T09:30:00+01:00",
        "end": "2025-01-08T11:00:00+01:00",
        "calendar": "family"
      },
      {
        "title": "Child flu vaccine at school",
        "start": "2025-01-09",
        "end": "2025-01-09",
        "calendar": "family"
      },
      {
        "title": "Swimming lesson",
        "start": "2025-01-10T15:45:00+01:00",
        "end": "2025-01-10T16:15:00+01:00",
        "calendar": "family"
      }
    ],
    "current_date": "2025-01-09",
    "current_day": "Thursday",
    "current_time": "20:48:00",
    "week_start": "2025-01-06",
    "period": "2025-01-06 to 2025-01-19",
    "friendly_name": "ESP32 Calendar Data"
  }
})json";

void test_parse_sample_json_success() {
    HAResponse response;
    bool result = parseResponse(sampleEventsJson, response);

    TEST_ASSERT_TRUE(result);
}

void test_parse_metadata_fields() {
    HAResponse response;
    parseResponse(sampleEventsJson, response);

    TEST_ASSERT_TRUE(response.currentDate == "2025-01-09");
    TEST_ASSERT_TRUE(response.currentDay == "Thursday");
    TEST_ASSERT_TRUE(response.currentTime == "20:48:00");
    TEST_ASSERT_TRUE(response.weekStart == "2025-01-06");
    TEST_ASSERT_TRUE(response.period == "2025-01-06 to 2025-01-19");
}

void test_parse_events_count() {
    HAResponse response;
    parseResponse(sampleEventsJson, response);

    // Should have 3 events from our simplified sample
    TEST_ASSERT_EQUAL_size_t(3, response.events.size());
}

void test_parse_timed_event() {
    HAResponse response;
    parseResponse(sampleEventsJson, response);

    // First event is "House cleaning service" - a timed event
    CalendarEvent& event = response.events[0];
    TEST_ASSERT_TRUE(event.title == "House cleaning service");
    TEST_ASSERT_TRUE(event.calendar == "family");
    TEST_ASSERT_TRUE(event.startTime == "09:30");
    TEST_ASSERT_TRUE(event.endTime == "11:00");
    TEST_ASSERT_FALSE(event.isMultiDay);
    TEST_ASSERT_EQUAL_INT(3, event.startDay);  // 2025-01-08 is day 3 from week start 2025-01-06
    TEST_ASSERT_EQUAL_INT(3, event.endDay);
}

void test_parse_full_day_event() {
    HAResponse response;
    parseResponse(sampleEventsJson, response);

    // Second event is "Child flu vaccine" - a full day event
    CalendarEvent& event = response.events[1];
    TEST_ASSERT_TRUE(event.title == "Child flu vaccine at school");
    TEST_ASSERT_TRUE(event.calendar == "family");
    TEST_ASSERT_TRUE(event.startTime == "-");  // Full day events use "-"
    TEST_ASSERT_TRUE(event.endTime == "");
    TEST_ASSERT_FALSE(event.isMultiDay);
    TEST_ASSERT_EQUAL_INT(4, event.startDay);  // 2025-01-09 is day 4 from week start
}

void test_extract_time_from_datetime() {
    String datetime = "2025-01-08T09:30:00+01:00";
    String time = extractTime(datetime);

    TEST_ASSERT_TRUE(time == "09:30");
}

void test_extract_time_from_date_only() {
    String datetime = "2025-01-09";
    String time = extractTime(datetime);

    TEST_ASSERT_TRUE(time == "");
}

void test_is_full_day_event_true() {
    String start = "2025-01-09";
    String end = "2025-01-09";

    TEST_ASSERT_TRUE(isFullDayEvent(start, end));
}

void test_is_full_day_event_false() {
    String start = "2025-01-08T09:30:00+01:00";
    String end = "2025-01-08T11:00:00+01:00";

    TEST_ASSERT_FALSE(isFullDayEvent(start, end));
}

void test_calculate_day_number_same_month() {
    String date = "2025-01-08";
    String weekStart = "2025-01-06";

    int dayNum = calculateDayNumber(date, weekStart);

    TEST_ASSERT_EQUAL_INT(3, dayNum);  // Jan 8 is day 3 from Jan 6
}

void test_calculate_day_number_with_time() {
    String date = "2025-01-10T15:45:00+01:00";
    String weekStart = "2025-01-06";

    int dayNum = calculateDayNumber(date, weekStart);

    TEST_ASSERT_EQUAL_INT(5, dayNum);  // Jan 10 is day 5 from Jan 6
}

void test_parse_invalid_json() {
    HAResponse response;
    const char* invalidJson = "{ invalid json }";

    bool result = parseResponse(invalidJson, response);

    TEST_ASSERT_FALSE(result);
}

void test_parse_empty_events_array() {
    const char* emptyEventsJson = R"json({
      "attributes": {
        "events": [],
        "current_date": "2025-01-09",
        "current_day": "Thursday",
        "current_time": "20:48:00",
        "week_start": "2025-01-06",
        "period": "2025-01-06 to 2025-01-19"
      }
    })json";

    HAResponse response;
    bool result = parseResponse(emptyEventsJson, response);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_size_t(0, response.events.size());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_sample_json_success);
    RUN_TEST(test_parse_metadata_fields);
    RUN_TEST(test_parse_events_count);
    RUN_TEST(test_parse_timed_event);
    RUN_TEST(test_parse_full_day_event);
    RUN_TEST(test_extract_time_from_datetime);
    RUN_TEST(test_extract_time_from_date_only);
    RUN_TEST(test_is_full_day_event_true);
    RUN_TEST(test_is_full_day_event_false);
    RUN_TEST(test_calculate_day_number_same_month);
    RUN_TEST(test_calculate_day_number_with_time);
    RUN_TEST(test_parse_invalid_json);
    RUN_TEST(test_parse_empty_events_array);

    return UNITY_END();
}
