#ifndef DRAWING_H
#define DRAWING_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <vector>

// Text alignment enum
typedef enum alignment {
  LEFT,
  RIGHT,
  CENTER
} alignment_t;

struct CalendarEvent {
  String title;
  int startDay;
  int endDay;
  String startTime;
  String endTime;
  String calendar;
  bool isMultiDay;
};

extern GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT / 2> display;

// Display initialization
void initDisplay();

// Text rendering functions
uint16_t getStringWidth(const String &text);
void drawString(int16_t x, int16_t y, const String &text, alignment_t alignment, uint16_t color=GxEPD_BLACK);
void drawMultiLnString(int16_t x, int16_t y, const String &text, alignment_t alignment, uint16_t max_width, uint16_t max_lines, int16_t line_spacing, uint16_t color=GxEPD_BLACK);

// Drawing functions
void drawRoundedRect(int x, int y, int width, int height, int radius, uint16_t color);
void drawSingleDayEvent(int x, int y, int width, int height, String startTime, String endTime, String title, uint16_t color = GxEPD_BLACK, bool singleLineMode = false);
void drawMultiDayEvent(int x, int y, int width, int height, String title, bool isStart, bool isEnd, uint16_t color = GxEPD_BLACK);
void drawCalendar(const std::vector<CalendarEvent>& events, String currentDate, String currentDay, String currentTime, String weekStart);
void drawStatusBar(const String &refreshTimeStr, int rssi, uint32_t batVoltage);
void drawError(const uint8_t *bitmap_196x196, const String &errMsgLn1, const String &errMsgLn2="");

// Icon/bitmap helper functions
const uint8_t *getBatBitmap24(uint32_t batPercent);
const uint8_t *getWiFiBitmap16(int rssi);
const char *getWiFidesc(int rssi);

// Date calculation helper functions
int calculateGridPosition(String date, String weekStart);

#endif