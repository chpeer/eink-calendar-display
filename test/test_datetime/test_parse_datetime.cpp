#include <unity.h>

#ifndef UNIT_TEST
#include <Arduino.h>
#else
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Mock String class for native testing
class String {
public:
    char* buffer;
    size_t len;

    String(const char* str = "") {
        len = strlen(str);
        buffer = new char[len + 1];
        strcpy(buffer, str);
    }

    String(const String& other) {
        len = other.len;
        buffer = new char[len + 1];
        strcpy(buffer, other.buffer);
    }

    ~String() {
        delete[] buffer;
    }

    size_t length() const { return len; }

    String substring(int start, int end = -1) const {
        if (end == -1) end = len;
        int subLen = end - start;
        char* sub = new char[subLen + 1];
        strncpy(sub, buffer + start, subLen);
        sub[subLen] = '\0';
        String result(sub);
        delete[] sub;
        return result;
    }

    int toInt() const {
        return atoi(buffer);
    }

    const char* c_str() const { return buffer; }
};

// Mock Serial for native testing
class SerialMock {
public:
    void println(const char* str) {
        printf("%s\n", str);
    }
    void printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
};

SerialMock Serial;
#endif

// Copy the parseHADateTime function here
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
}

void test_parse_valid_datetime() {
    tm timeInfo = {0};
    String date = "2025-01-09";
    String time = "20:48:00";

    bool result = parseHADateTime(date, time, &timeInfo);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2025 - 1900, timeInfo.tm_year);
    TEST_ASSERT_EQUAL_INT(0, timeInfo.tm_mon);  // January = 0
    TEST_ASSERT_EQUAL_INT(9, timeInfo.tm_mday);
    TEST_ASSERT_EQUAL_INT(20, timeInfo.tm_hour);
    TEST_ASSERT_EQUAL_INT(48, timeInfo.tm_min);
    TEST_ASSERT_EQUAL_INT(0, timeInfo.tm_sec);
}

void test_parse_new_years_day() {
    tm timeInfo = {0};
    String date = "2025-01-01";
    String time = "00:00:00";

    bool result = parseHADateTime(date, time, &timeInfo);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2025 - 1900, timeInfo.tm_year);
    TEST_ASSERT_EQUAL_INT(0, timeInfo.tm_mon);
    TEST_ASSERT_EQUAL_INT(1, timeInfo.tm_mday);
    TEST_ASSERT_EQUAL_INT(0, timeInfo.tm_hour);
    TEST_ASSERT_EQUAL_INT(0, timeInfo.tm_min);
    TEST_ASSERT_EQUAL_INT(0, timeInfo.tm_sec);
}

void test_parse_december_date() {
    tm timeInfo = {0};
    String date = "2024-12-31";
    String time = "23:59:59";

    bool result = parseHADateTime(date, time, &timeInfo);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2024 - 1900, timeInfo.tm_year);
    TEST_ASSERT_EQUAL_INT(11, timeInfo.tm_mon);  // December = 11
    TEST_ASSERT_EQUAL_INT(31, timeInfo.tm_mday);
    TEST_ASSERT_EQUAL_INT(23, timeInfo.tm_hour);
    TEST_ASSERT_EQUAL_INT(59, timeInfo.tm_min);
    TEST_ASSERT_EQUAL_INT(59, timeInfo.tm_sec);
}

void test_parse_midday() {
    tm timeInfo = {0};
    String date = "2025-06-15";
    String time = "12:30:45";

    bool result = parseHADateTime(date, time, &timeInfo);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2025 - 1900, timeInfo.tm_year);
    TEST_ASSERT_EQUAL_INT(5, timeInfo.tm_mon);  // June = 5
    TEST_ASSERT_EQUAL_INT(15, timeInfo.tm_mday);
    TEST_ASSERT_EQUAL_INT(12, timeInfo.tm_hour);
    TEST_ASSERT_EQUAL_INT(30, timeInfo.tm_min);
    TEST_ASSERT_EQUAL_INT(45, timeInfo.tm_sec);
}

void test_parse_invalid_date_too_short() {
    tm timeInfo = {0};
    String date = "2025-01";  // Too short
    String time = "20:48:00";

    bool result = parseHADateTime(date, time, &timeInfo);

    TEST_ASSERT_FALSE(result);
}

void test_parse_invalid_time_too_short() {
    tm timeInfo = {0};
    String date = "2025-01-09";
    String time = "20:48";  // Too short

    bool result = parseHADateTime(date, time, &timeInfo);

    TEST_ASSERT_FALSE(result);
}

void test_parse_day_of_week_thursday() {
    // 2025-01-09 is a Thursday
    tm timeInfo = {0};
    String date = "2025-01-09";
    String time = "12:00:00";

    bool result = parseHADateTime(date, time, &timeInfo);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(4, timeInfo.tm_wday);  // Thursday = 4
}

void test_parse_day_of_week_monday() {
    // 2025-01-06 is a Monday
    tm timeInfo = {0};
    String date = "2025-01-06";
    String time = "12:00:00";

    bool result = parseHADateTime(date, time, &timeInfo);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, timeInfo.tm_wday);  // Monday = 1
}

void test_parse_day_of_week_sunday() {
    // 2025-01-05 is a Sunday
    tm timeInfo = {0};
    String date = "2025-01-05";
    String time = "12:00:00";

    bool result = parseHADateTime(date, time, &timeInfo);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, timeInfo.tm_wday);  // Sunday = 0
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_valid_datetime);
    RUN_TEST(test_parse_new_years_day);
    RUN_TEST(test_parse_december_date);
    RUN_TEST(test_parse_midday);
    RUN_TEST(test_parse_invalid_date_too_short);
    RUN_TEST(test_parse_invalid_time_too_short);
    RUN_TEST(test_parse_day_of_week_thursday);
    RUN_TEST(test_parse_day_of_week_monday);
    RUN_TEST(test_parse_day_of_week_sunday);

    return UNITY_END();
}
