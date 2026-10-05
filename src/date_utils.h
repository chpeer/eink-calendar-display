#ifndef DATE_UTILS_H
#define DATE_UTILS_H

// Header-only calendar date arithmetic with no Arduino dependencies, so it
// can be shared by the firmware and the native unit tests.

#include <cstdlib>

/* Returns the number of days since 1970-01-01 for a proleptic Gregorian date.
 * Handles real month lengths and leap years.
 * Algorithm: Howard Hinnant, "chrono-Compatible Low-Level Date Algorithms".
 */
inline long daysFromCivil(int y, int m, int d)
{
  y -= m <= 2;
  const long era = (y >= 0 ? y : y - 399) / 400;
  const long yoe = y - era * 400;                                // [0, 399]
  const long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; // [0, 365]
  const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;        // [0, 146096]
  return era * 146097 + doe - 719468;
}

/* Inverse of daysFromCivil: converts days since 1970-01-01 to y/m/d.
 */
inline void civilFromDays(long z, int &y, int &m, int &d)
{
  z += 719468;
  const long era = (z >= 0 ? z : z - 146096) / 146097;
  const long doe = z - era * 146097;                                   // [0, 146096]
  const long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
  const long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);            // [0, 365]
  const long mp = (5 * doy + 2) / 153;                                 // [0, 11]
  d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
  m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
  y = static_cast<int>(yoe + era * 400 + (m <= 2));
}

/* Parses the date part of "YYYY-MM-DD" or "YYYY-MM-DDTHH:MM:SS+TZ" into
 * days since 1970-01-01.
 */
inline long parseDateToDays(const char *date)
{
  int y = std::atoi(date);
  int m = std::atoi(date + 5);
  int d = std::atoi(date + 8);
  return daysFromCivil(y, m, d);
}

/* Returns the 1-based grid position of a date relative to the week start
 * (weekStart itself is 1, the following day is 2, ...).
 */
inline int dayNumberFromWeekStart(const char *date, const char *weekStart)
{
  return static_cast<int>(parseDateToDays(date) - parseDateToDays(weekStart)) + 1;
}

/* Returns the day of month for the date that is offsetDays after weekStart.
 */
inline int dayOfMonthAfter(const char *weekStart, int offsetDays)
{
  int y, m, d;
  civilFromDays(parseDateToDays(weekStart) + offsetDays, y, m, d);
  return d;
}

#endif
