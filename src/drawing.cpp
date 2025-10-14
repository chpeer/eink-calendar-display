#include "drawing.h"
#include "utilities.h"
#include "icons.h"
#include "DongleLight9pt7b.h"
#include "DongleLight9pt15b.h"
#include "config.h"
#include <SPI.h>
#include <algorithm>

// ============================================================================
// Text Rendering Helper Functions
// ============================================================================

/* Returns the width of a string in pixels based on current font.
 */
uint16_t getStringWidth(const String &text)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return w;
}

/* Draws a string with specified alignment (LEFT, CENTER, RIGHT).
 */
void drawString(int16_t x, int16_t y, const String &text, alignment_t alignment,
                uint16_t color)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextColor(color);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (alignment == RIGHT)
  {
    x = x - w;
  }
  if (alignment == CENTER)
  {
    x = x - w / 2;
  }
  display.setCursor(x, y);
  display.print(text);
}

/* Draws multi-line string with intelligent word wrapping.
 */
void drawMultiLnString(int16_t x, int16_t y, const String &text,
                       alignment_t alignment, uint16_t max_width,
                       uint16_t max_lines, int16_t line_spacing,
                       uint16_t color)
{
  uint16_t current_line = 0;
  String textRemaining = text;
  // print until we reach max_lines or no more text remains
  while (current_line < max_lines && !textRemaining.isEmpty())
  {
    int16_t  x1, y1;
    uint16_t w, h;

    display.getTextBounds(textRemaining, 0, 0, &x1, &y1, &w, &h);

    int endIndex = textRemaining.length();
    // check if remaining text is to wide, if it is then print what we can
    String subStr = textRemaining;
    int splitAt = 0;
    int keepLastChar = 0;
    while (w > max_width && splitAt != -1)
    {
      if (keepLastChar)
      {
        // if we kept the last character during the last iteration of this while
        // loop, remove it now so we don't get stuck in an infinite loop.
        subStr.remove(subStr.length() - 1);
      }

      // find the last place in the string that we can break it.
      if (current_line < max_lines - 1)
      {
        splitAt = std::max(subStr.lastIndexOf(" "),
                           subStr.lastIndexOf("-"));
      }
      else
      {
        // this is the last line, only break at spaces so we can add ellipsis
        splitAt = subStr.lastIndexOf(" ");
      }

      // if splitAt == -1 then there is an unbroken set of characters that is
      // longer than max_width. Otherwise if splitAt != -1 then we can continue
      // the loop until the string is <= max_width
      if (splitAt != -1)
      {
        endIndex = splitAt;
        subStr = subStr.substring(0, endIndex + 1);

        char lastChar = subStr.charAt(endIndex);
        if (lastChar == ' ')
        {
          // remove this char now so it is not counted towards line width
          keepLastChar = 0;
          subStr.remove(endIndex);
          --endIndex;
        }
        else if (lastChar == '-')
        {
          // this char will be printed on this line and removed next iteration
          keepLastChar = 1;
        }

        if (current_line < max_lines - 1)
        {
          // this is not the last line
          display.getTextBounds(subStr, 0, 0, &x1, &y1, &w, &h);
        }
        else
        {
          // this is the last line, we need to make sure there is space for
          // ellipsis
          display.getTextBounds(subStr + "...", 0, 0, &x1, &y1, &w, &h);
          if (w <= max_width)
          {
            // ellipsis fit, add them to subStr
            subStr = subStr + "...";
          }
        }

      } // end if (splitAt != -1)
    } // end inner while

    drawString(x, y + (current_line * line_spacing), subStr, alignment, color);

    // update textRemaining to no longer include what was printed
    // +1 for exclusive bounds, +1 to get passed space/dash
    textRemaining = textRemaining.substring(endIndex + 2 - keepLastChar);

    ++current_line;
  } // end outer while

  return;
} // end drawMultiLnString

// ============================================================================
// Icon/Bitmap Helper Functions
// ============================================================================

/* Returns 24x24 bitmap incidcating battery status.
 */
const uint8_t *getBatBitmap24(uint32_t batPercent)
{
  if (batPercent >= 93)
  {
    return battery_full_90deg_24x24;
  }
  else if (batPercent >= 79)
  {
    return battery_6_bar_90deg_24x24;
  }
  else if (batPercent >= 65)
  {
    return battery_5_bar_90deg_24x24;
  }
  else if (batPercent >= 50)
  {
    return battery_4_bar_90deg_24x24;
  }
  else if (batPercent >= 36)
  {
    return battery_3_bar_90deg_24x24;
  }
  else if (batPercent >= 22)
  {
    return battery_2_bar_90deg_24x24;
  }
  else if (batPercent >= 8)
  {
    return battery_1_bar_90deg_24x24;
  }
  else
  {  // batPercent < 8
    return battery_0_bar_90deg_24x24;
  }
} // end getBatBitmap24

/* Returns appropriate WiFi icon based on signal strength.
 */
const uint8_t *getWiFiBitmap16(int rssi)
{
  if (rssi == 0)
    return wifi_x_16x16;
  else if (rssi >= -50)
    return wifi_16x16;
  else if (rssi >= -60)
    return wifi_3_bar_16x16;
  else if (rssi >= -70)
    return wifi_2_bar_16x16;
  else
    return wifi_1_bar_16x16;
}

/* Returns WiFi signal quality description.
 */
const char *getWiFidesc(int rssi)
{
  if (rssi == 0)
    return TXT_WIFI_NO_CONNECTION;
  else if (rssi >= -50)
    return TXT_WIFI_EXCELLENT;
  else if (rssi >= -60)
    return TXT_WIFI_GOOD;
  else if (rssi >= -70)
    return TXT_WIFI_FAIR;
  else
    return TXT_WIFI_POOR;
}

// ============================================================================
// Calendar Drawing Functions
// ============================================================================

// Helper function to calculate grid position from date string (YYYY-MM-DD)
int calculateGridPosition(String date, String weekStart) {
  // Extract components from date strings
  int dateYear = date.substring(0, 4).toInt();
  int dateMonth = date.substring(5, 7).toInt();
  int dateDay = date.substring(8, 10).toInt();

  int weekStartYear = weekStart.substring(0, 4).toInt();
  int weekStartMonth = weekStart.substring(5, 7).toInt();
  int weekStartDay = weekStart.substring(8, 10).toInt();

  // Simple date calculation for same month
  if (dateYear == weekStartYear && dateMonth == weekStartMonth) {
    return dateDay - weekStartDay + 1;
  }

  // Handle month boundaries (simplified for common cases)
  if (dateYear == weekStartYear) {
    if (dateMonth == weekStartMonth + 1) {
      // Next month - assume previous month had 31 days
      return (31 - weekStartDay + 1) + dateDay;
    } else if (dateMonth == weekStartMonth - 1) {
      // Previous month case (shouldn't happen in normal calendar view)
      return dateDay - weekStartDay + 1;
    }
  }

  // Handle year boundary (December to January)
  if (weekStartYear == dateYear - 1 && weekStartMonth == 12 && dateMonth == 1) {
    return (31 - weekStartDay + 1) + dateDay;
  }

  // Default fallback
  return 1;
}

// Helper function to calculate calendar day number for display
int calculateCalendarDay(String weekStart, int weekNumber, int dayInWeek) {
  int weekStartYear = weekStart.substring(0, 4).toInt();
  int weekStartMonth = weekStart.substring(5, 7).toInt();
  int weekStartDay = weekStart.substring(8, 10).toInt();

  // Calculate total days offset from week start
  int totalDaysOffset = (weekNumber * 7) + dayInWeek;

  // Add offset to week start day
  int resultDay = weekStartDay + totalDaysOffset;

  // Simple month handling (assumes 31 days per month for simplicity)
  // In production, you'd want proper date arithmetic with actual month lengths
  if (resultDay > 31) {
    resultDay = resultDay - 31; // Move to next month
  }

  return resultDay;
}

void initDisplay() {
  // Now initialize the display
  display.init(115200, true, 10, false);
 
  // Configure SPI pins
  SPI.end();
  SPI.begin(EPD_SCK,
            EPD_MISO,
            EPD_MOSI,
            EPD_CS);
  display.setRotation(0);
  display.setFont(&DongleLight15pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();
  display.firstPage(); // use paged drawing mode, sets fillScreen(GxEPD_WHITE)
  return;
}

void powerOffDisplay() {
   display.hibernate(); // turns powerOff() and sets controller to deep sleep for
                       // minimum power use
}

void drawRoundedRect(int x, int y, int width, int height, int radius, uint16_t color) {
  display.fillRect(x + radius, y, width - 2 * radius, height, color);
  display.fillRect(x, y + radius, width, height - 2 * radius, color);
  display.fillCircle(x + radius, y + radius, radius, color);
  display.fillCircle(x + width - radius - 1, y + radius, radius, color);
  display.fillCircle(x + radius, y + height - radius - 1, radius, color);
  display.fillCircle(x + width - radius - 1, y + height - radius - 1, radius, color);
}

void drawSingleDayEvent(int x, int y, int width, int height, String startTime, String endTime, String title, uint16_t color, bool singleLineMode) {
  drawRoundedRect(x, y, width, height, EVENT_BORDER_RADIUS, color);

  // Display time range on first line
  display.setTextColor(GxEPD_WHITE);
  display.setFont(&DongleLight9pt7b);
  display.setCursor(x + EVENT_TEXT_MARGIN, y + TIME_TEXT_Y_OFFSET);
  display.print(startTime + "-" + endTime);

  // Display title - supporting single-line mode for overflow
  display.setFont(&DongleLight15pt7b);

  // Calculate available characters per line
  int charsPerLine = CHARS_PER_LINE;
  String titleLine1 = "";
  String titleLine2 = "";

  if (singleLineMode || title.length() <= charsPerLine) {
    // Single line mode or title fits on one line
    if (title.length() <= charsPerLine) {
      titleLine1 = title;
    } else {
      // Truncate for single line mode
      titleLine1 = title.substring(0, charsPerLine - 1) + ".";
    }
  } else {
    // Split title across two lines at word boundary if possible
    int splitPos = charsPerLine;
    // Try to find a space near the split point to avoid breaking words
    for (int i = charsPerLine - 3; i < min((int)title.length(), charsPerLine + 3); i++) {
      if (i < title.length() && title.charAt(i) == ' ') {
        splitPos = i;
        break;
      }
    }

    titleLine1 = title.substring(0, splitPos);
    if (splitPos < title.length()) {
      titleLine2 = title.substring(splitPos + (title.charAt(splitPos) == ' ' ? 1 : 0));
      // Truncate second line if too long
      if (titleLine2.length() > charsPerLine) {
        titleLine2 = titleLine2.substring(0, charsPerLine - 1) + ".";
      }
    }
  }

  // Draw first line of title
  display.setCursor(x + EVENT_TEXT_MARGIN, y + TITLE_FIRST_LINE_Y_OFFSET);
  display.print(titleLine1);

  // Draw second line of title if it exists and not in single line mode
  if (titleLine2.length() > 0 && !singleLineMode) {
    display.setCursor(x + EVENT_TEXT_MARGIN, y + TITLE_SECOND_LINE_Y_OFFSET);
    display.print(titleLine2);
  }

  display.setTextColor(GxEPD_BLACK);
  display.setFont(&DongleLight15pt7b);
}

void drawMultiDayEvent(int x, int y, int width, int height, String title, bool isStart, bool isEnd, uint16_t color) {
  if (isStart && isEnd) {
    drawRoundedRect(x, y, width, height, EVENT_BORDER_RADIUS, color);
  } else if (isStart) {
    display.fillRect(x + EVENT_BORDER_RADIUS, y, width - EVENT_BORDER_RADIUS, height, color);
    display.fillCircle(x + EVENT_BORDER_RADIUS, y + EVENT_BORDER_RADIUS, EVENT_BORDER_RADIUS, color);
    display.fillCircle(x + EVENT_BORDER_RADIUS, y + height - EVENT_BORDER_RADIUS, EVENT_BORDER_RADIUS, color);
  } else if (isEnd) {
    display.fillRect(x, y, width - EVENT_BORDER_RADIUS, height, color);
    display.fillCircle(x + width - EVENT_BORDER_RADIUS, y + EVENT_BORDER_RADIUS, EVENT_BORDER_RADIUS, color);
    display.fillCircle(x + width - EVENT_BORDER_RADIUS, y + height - EVENT_BORDER_RADIUS, EVENT_BORDER_RADIUS, color);
  } else {
    display.fillRect(x, y, width, height, color);
  }

  display.setTextColor(GxEPD_WHITE);
  display.setFont(&DongleLight15pt7b);
  display.setCursor(x + EVENT_TEXT_MARGIN, y + 19);
  display.print(title);

  display.setTextColor(GxEPD_BLACK);
}

void drawCalendar(const std::vector<CalendarEvent>& events, String currentDate, String currentDay, String currentTime, String weekStart) {

  // Calculate current day grid position relative to week start
  int currentDayNumber = calculateGridPosition(currentDate, weekStart);

    const char* weekdays[] = {TXT_MONDAY, TXT_TUESDAY, TXT_WEDNESDAY, TXT_THURSDAY, TXT_FRIDAY, TXT_SATURDAY, TXT_SUNDAY};

    display.setTextColor(GxEPD_BLACK);

    for (int day = 0; day < 7; day++) {
      int x = day * DAY_WIDTH;
      int y = HEADER_HEIGHT - HEADER_TEXT_Y_OFFSET;

      // Calculate which weekday is current based on currentDayNumber
      int currentWeekday = (currentDayNumber - 1) % 7; // Convert day number to weekday index

      // Set color for current weekday
      if (day == currentWeekday) {
        display.setTextColor(GxEPD_RED);
      } else {
        display.setTextColor(GxEPD_BLACK);
      }

      // Proper text width calculation for Dongle Light font centering
      // Use font-specific character widths for accurate centering
      int textWidth;
      switch(day) {
        case 0: textWidth = 52; break; // MONDAY (6 chars)
        case 1: textWidth = 62; break; // TUESDAY (7 chars)
        case 2: textWidth = 82; break; // WEDNESDAY (9 chars)
        case 3: textWidth = 70; break; // THURSDAY (8 chars)
        case 4: textWidth = 52; break; // FRIDAY (6 chars)
        case 5: textWidth = 70; break; // SATURDAY (8 chars)
        case 6: textWidth = 50; break; // SUNDAY (6 chars)
        default: textWidth = 60; break;
      }
      int centerX = x + (DAY_WIDTH - textWidth) / 2;

      display.setCursor(centerX, y);
      display.print(weekdays[day]);
    }
    display.setTextColor(GxEPD_BLACK); // Reset to black

    display.drawLine(0, HEADER_HEIGHT, 800, HEADER_HEIGHT, GxEPD_BLACK);

    // STEP 1: Assign consistent rows to all multi-day events globally
    // Create a dynamic row assignment for multi-day events
    std::vector<int> eventRows(events.size(), -1); // Initialize to -1 (unassigned)

    // Sort multi-day events by start day and assign rows
    for (int currentDay = 1; currentDay <= 14; currentDay++) {
      for (int i = 0; i < events.size(); i++) {
        if (events[i].isMultiDay && events[i].startDay == currentDay && eventRows[i] == -1) {
          // Find the first available row for this event
          int assignedRow = 0;
          bool rowFound = false;

          while (!rowFound) {
            bool rowOccupied = false;
            // Check if this row is occupied by any other event on any day this event spans
            for (int checkDay = events[i].startDay; checkDay <= events[i].endDay; checkDay++) {
              for (int j = 0; j < events.size(); j++) {
                if (j != i && events[j].isMultiDay && eventRows[j] == assignedRow &&
                    checkDay >= events[j].startDay && checkDay <= events[j].endDay) {
                  rowOccupied = true;
                  break;
                }
              }
              if (rowOccupied) break;
            }

            if (!rowOccupied) {
              eventRows[i] = assignedRow;
              rowFound = true;
            } else {
              assignedRow++;
            }
          }
        }
      }
    }

    // STEP 2: Draw multi-day events as continuous boxes (only on their start day)
    for (int i = 0; i < events.size(); i++) {
      if (events[i].isMultiDay) {
        int startDay = events[i].startDay;
        int endDay = events[i].endDay;

        // Calculate which week and day position the start day is in
        int startWeek = (startDay - 1) / 7;
        int startDayInWeek = (startDay - 1) % 7;

        if (startWeek < 2) { // Only draw if it starts in our 2-week view
          int startX = startDayInWeek * DAY_WIDTH + EVENT_MARGIN;
          int startY = HEADER_HEIGHT + (startWeek * ROW_HEIGHT) + DAY_NUMBER_MARGIN + (eventRows[i] * MULTI_DAY_EVENT_SPACING);

          // Calculate how many days this event spans in the visible calendar
          int daysInCalendar = min(endDay, 14) - startDay + 1;
          int daysToEndOfWeek = 7 - startDayInWeek;
          int daysInFirstWeek = min(daysInCalendar, daysToEndOfWeek);

          // Draw first week portion
          int eventWidth = (daysInFirstWeek * DAY_WIDTH) - (2 * EVENT_MARGIN);

          String titleToShow = events[i].title;
          // Only truncate if title is longer than the total span allows
          int totalAvailableChars = (daysInCalendar * DAY_WIDTH - 6) / 7;
          if (titleToShow.length() > totalAvailableChars) {
            titleToShow = titleToShow.substring(0, totalAvailableChars - 3) + "...";
          }

          bool isStart = true;
          bool isEnd = (endDay <= startDay + daysInFirstWeek - 1);

          uint16_t eventColor = (currentDayNumber >= events[i].startDay && currentDayNumber <= events[i].endDay) ? GxEPD_RED : GxEPD_BLACK;
          drawMultiDayEvent(startX, startY, eventWidth, MULTI_DAY_EVENT_HEIGHT, titleToShow, isStart, isEnd, eventColor);

          // If event continues to second week, draw continuation
          if (daysInCalendar > daysInFirstWeek && startWeek == 0) {
            int secondWeekDays = daysInCalendar - daysInFirstWeek;
            int secondWeekX = EVENT_MARGIN;
            int secondWeekY = HEADER_HEIGHT + ROW_HEIGHT + DAY_NUMBER_MARGIN + (eventRows[i] * MULTI_DAY_EVENT_SPACING);
            int secondWeekWidth = (secondWeekDays * DAY_WIDTH) - (2 * EVENT_MARGIN);

            drawMultiDayEvent(secondWeekX, secondWeekY, secondWeekWidth, MULTI_DAY_EVENT_HEIGHT, "", false, true, eventColor);
          }
        }
      }
    }

    // STEP 3: Draw day numbers and single-day events
    for (int week = 0; week < 2; week++) {
      int y = HEADER_HEIGHT + (week * ROW_HEIGHT);

      if (week < 1) {
        display.drawLine(0, y + ROW_HEIGHT, 800, y + ROW_HEIGHT, GxEPD_BLACK);
      }

      for (int day = 0; day < 7; day++) {
        int x = day * DAY_WIDTH;
        int dayNumber = calculateCalendarDay(weekStart, week, day);

        // Calculate the relative day number for event comparison
        int relativeDayNumber = (week * 7) + day + 1;

        // Draw day number with red background box if current day
        if (relativeDayNumber == currentDayNumber) {
          // Draw red background box for current day
          display.fillRoundRect(x + CURRENT_DAY_BOX_X_OFFSET, y + CURRENT_DAY_BOX_Y_OFFSET,
                               CURRENT_DAY_BOX_WIDTH, CURRENT_DAY_BOX_HEIGHT, EVENT_BORDER_RADIUS, GxEPD_RED);
          display.setTextColor(GxEPD_WHITE);
        } else {
          display.setTextColor(GxEPD_BLACK);
        }
        display.setCursor(x + DAY_NUMBER_X_OFFSET, y + DAY_NUMBER_Y_OFFSET);
        display.print(dayNumber);
        display.setTextColor(GxEPD_BLACK); // Reset to black

        // Count how many multi-day events affect this day to know where to start single-day events
        int multiDayCount = 0;
        for (int i = 0; i < events.size(); i++) {
          if (events[i].isMultiDay && relativeDayNumber >= events[i].startDay && relativeDayNumber <= events[i].endDay) {
            if (eventRows[i] + 1 > multiDayCount) {
              multiDayCount = eventRows[i] + 1;
            }
          }
        }

        // Calculate available space for single-day events
        int dayNumberMargin = 25; // Increased to 25px for better spacing
        int availableHeight = ROW_HEIGHT - dayNumberMargin - (multiDayCount * 30);

        // Collect single-day events for this day and sort by time
        int singleDayEventIndices[30]; // Max 30 events
        int singleDayEventCount = 0;

        for (int i = 0; i < events.size(); i++) {
          if (!events[i].isMultiDay && events[i].startDay == relativeDayNumber) {
            singleDayEventIndices[singleDayEventCount] = i;
            singleDayEventCount++;
          }
        }

        // Sort single-day events by start time (simple bubble sort)
        for (int i = 0; i < singleDayEventCount - 1; i++) {
          for (int j = 0; j < singleDayEventCount - i - 1; j++) {
            String time1 = events[singleDayEventIndices[j]].startTime;
            String time2 = events[singleDayEventIndices[j + 1]].startTime;

            // Compare times (HH:MM format)
            if (time1.compareTo(time2) > 0) {
              int temp = singleDayEventIndices[j];
              singleDayEventIndices[j] = singleDayEventIndices[j + 1];
              singleDayEventIndices[j + 1] = temp;
            }
          }
        }

        // Draw single-day events starting after all multi-day events
        int eventY = y + dayNumberMargin + (multiDayCount * 30);
        int singleDayEventsDrawn = 0;

        // Calculate how many events we can actually show
        // Simple approach: calculate how many full events fit, then check if we can add one reduced
        int maxFullEvents = availableHeight / SINGLE_DAY_EVENT_SPACING;
        int eventsToShow;

        if (singleDayEventCount <= maxFullEvents) {
          // All events fit with full height
          eventsToShow = singleDayEventCount;
        } else {
          // Not all events fit - check if we can show maxFullEvents-1 full + 1 reduced + overflow
          int spaceForReduced = ((maxFullEvents - 1) * SINGLE_DAY_EVENT_SPACING) +
                               (SINGLE_DAY_EVENT_HEIGHT_REDUCED + 2) +
                               OVERFLOW_TEXT_SPACING;

          if (spaceForReduced <= availableHeight && maxFullEvents > 0) {
            // We can fit maxFullEvents-1 full + 1 reduced + overflow text
            eventsToShow = maxFullEvents;
          } else {
            // Show maxFullEvents-1 full events + overflow (no reduced event)
            eventsToShow = max(1, maxFullEvents - 1);
          }
        }

        for (int i = 0; i < eventsToShow; i++) {
          int eventWidth = DAY_WIDTH - (2 * EVENT_MARGIN);
          int eventX = x + EVENT_MARGIN;
          int eventIndex = singleDayEventIndices[i];

          // Determine if this is the last event and there's overflow
          bool isLastEventWithOverflow = (i == eventsToShow - 1 && singleDayEventCount > eventsToShow);
          bool singleLineMode = isLastEventWithOverflow;
          int eventHeight = isLastEventWithOverflow ? SINGLE_DAY_EVENT_HEIGHT_REDUCED : SINGLE_DAY_EVENT_HEIGHT;

          uint16_t singleEventColor = (relativeDayNumber == currentDayNumber) ? GxEPD_RED : GxEPD_BLACK;
          drawSingleDayEvent(eventX, eventY, eventWidth, eventHeight,
                            events[eventIndex].startTime, events[eventIndex].endTime, events[eventIndex].title, singleEventColor, singleLineMode);

          if (isLastEventWithOverflow) {
            // For reduced height event, use smaller spacing
            eventY += SINGLE_DAY_EVENT_HEIGHT_REDUCED + 2; // 2px margin
          } else {
            eventY += SINGLE_DAY_EVENT_SPACING;
          }
          singleDayEventsDrawn++;
        }

        // Show overflow indicator if there are more events than we could display
        if (singleDayEventCount > eventsToShow) {
          int remainingEvents = singleDayEventCount - eventsToShow;
          display.setTextColor(GxEPD_BLACK);
          display.setFont(&DongleLight9pt7b);
          display.setCursor(x + EVENT_MARGIN, eventY + OVERFLOW_TEXT_Y_OFFSET);
          display.print(String(remainingEvents) + " more events...");
          // Reset font back to default for subsequent day numbers
          display.setFont(&DongleLight15pt7b);
        }
      }
    }
}

/* This function is responsible for drawing the status bar along the bottom of
 * the display.
 */
void drawStatusBar(const String &refreshTimeStr,
                   int rssi, uint32_t batVoltage)
{
  String dataStr;
  uint16_t dataColor = GxEPD_BLACK;
  display.setFont(&DongleLight9pt7b);
  int pos = DISP_WIDTH - 2;
  const int sp = 2;

#if BATTERY_MONITORING
  // battery - (expecting 3.7v LiPo)
  uint32_t batPercent = calcBatPercent(batVoltage,
                                       MIN_BATTERY_VOLTAGE,
                                       MAX_BATTERY_VOLTAGE);
  if (batVoltage < WARN_BATTERY_VOLTAGE)
  {
    dataColor = ACCENT_COLOR;
  }
  dataStr = String(batPercent) + "%";
  dataStr += " (" + String( std::round(batVoltage / 10.f) / 100.f, 2 ) + "v)";
  drawString(pos, DISP_HEIGHT - 1 - 2, dataStr, RIGHT, dataColor);
  pos -= getStringWidth(dataStr) + 25;
  display.drawInvertedBitmap(pos, DISP_HEIGHT - 1 - 17,
                             getBatBitmap24(batPercent), 24, 24, dataColor);
  pos -= sp + 9;
#endif

  // WiFi
  dataStr = String(getWiFidesc(rssi));
  dataColor = rssi >= -70 ? GxEPD_BLACK : ACCENT_COLOR;
  if (rssi != 0)
  {
    dataStr += " (" + String(rssi) + "dBm)";
  }
  // Calculate positions: icon first, then text to its right
  int wifiIconPos = pos - getStringWidth(dataStr) - 19;
  int wifiTextPos = wifiIconPos + 16 + 3; // icon width + small margin
  display.drawInvertedBitmap(wifiIconPos, DISP_HEIGHT - 1 - 13, getWiFiBitmap16(rssi),
                             16, 16, dataColor);
  drawString(wifiTextPos, DISP_HEIGHT - 1 - 2, dataStr, LEFT, dataColor);
  pos = wifiIconPos - sp;

  // last refresh
  dataColor = GxEPD_BLACK;
  drawString(pos, DISP_HEIGHT - 1 - 2, refreshTimeStr, RIGHT, dataColor);
  pos -= getStringWidth(refreshTimeStr) + 25;
  display.drawInvertedBitmap(pos, DISP_HEIGHT - 1 - 21, wi_refresh_32x32,
                             32, 32, dataColor);
  pos -= sp;
  return;
} // end drawStatusBar


/* This function is responsible for drawing prominent error messages to the
 * screen.
 *
 * If error message line 2 (errMsgLn2) is empty, line 1 will be automatically
 * wrapped.
 */
void drawError(const uint8_t *bitmap_196x196,
               const String &errMsgLn1, const String &errMsgLn2)
{
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&DongleLight15pt7b);
  if (!errMsgLn2.isEmpty())
  {
    drawString(DISP_WIDTH / 2,
               DISP_HEIGHT / 2 + 196 / 2 + 21,
               errMsgLn1, CENTER);
    drawString(DISP_WIDTH / 2,
               DISP_HEIGHT / 2 + 196 / 2 + 21 + 55,
               errMsgLn2, CENTER);
  }
  else
  {
    drawMultiLnString(DISP_WIDTH / 2,
                      DISP_HEIGHT / 2 + 196 / 2 + 21,
                      errMsgLn1, CENTER, DISP_WIDTH - 200, 2, 55);
  }
  display.drawInvertedBitmap(DISP_WIDTH / 2 - 196 / 2,
                             DISP_HEIGHT / 2 - 196 / 2 - 21,
                             bitmap_196x196, 196, 196, ACCENT_COLOR);
  return;
} // end drawError