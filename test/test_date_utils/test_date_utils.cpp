#include <unity.h>
#include "../../src/date_utils.h"

void setUp(void) {}
void tearDown(void) {}

void test_days_from_civil_epoch() {
    TEST_ASSERT_EQUAL_INT32(0, daysFromCivil(1970, 1, 1));
    TEST_ASSERT_EQUAL_INT32(1, daysFromCivil(1970, 1, 2));
}

void test_civil_round_trip() {
    for (long z = daysFromCivil(2024, 1, 1); z < daysFromCivil(2030, 1, 1); z++) {
        int y, m, d;
        civilFromDays(z, y, m, d);
        TEST_ASSERT_EQUAL_INT32(z, daysFromCivil(y, m, d));
    }
}

void test_day_number_same_month() {
    TEST_ASSERT_EQUAL_INT(1, dayNumberFromWeekStart("2026-10-05", "2026-10-05"));
    TEST_ASSERT_EQUAL_INT(14, dayNumberFromWeekStart("2026-10-18", "2026-10-05"));
}

void test_day_number_across_30_day_month() {
    // Regression: Sun 4 Oct 2026 was drawn on Monday of week 2
    TEST_ASSERT_EQUAL_INT(3, dayNumberFromWeekStart("2026-09-30", "2026-09-28"));
    TEST_ASSERT_EQUAL_INT(4, dayNumberFromWeekStart("2026-10-01", "2026-09-28"));
    TEST_ASSERT_EQUAL_INT(7, dayNumberFromWeekStart("2026-10-04", "2026-09-28"));
}

void test_day_number_across_february() {
    // 2027 is not a leap year, 2028 is
    TEST_ASSERT_EQUAL_INT(5, dayNumberFromWeekStart("2027-03-01", "2027-02-25"));
    TEST_ASSERT_EQUAL_INT(6, dayNumberFromWeekStart("2028-03-01", "2028-02-25"));
}

void test_day_number_across_year_end() {
    TEST_ASSERT_EQUAL_INT(4, dayNumberFromWeekStart("2026-01-01", "2025-12-29"));
    // Window starting late November reaching into December
    TEST_ASSERT_EQUAL_INT(8, dayNumberFromWeekStart("2026-12-07", "2026-11-30"));
}

void test_day_number_with_time_component() {
    TEST_ASSERT_EQUAL_INT(7, dayNumberFromWeekStart("2026-10-04T12:00:00+01:00", "2026-09-28"));
}

void test_day_number_before_week_start() {
    // Multi-day events can start before the displayed window
    TEST_ASSERT_EQUAL_INT(-1, dayNumberFromWeekStart("2026-09-26", "2026-09-28"));
}

void test_day_of_month_labels_across_30_day_month() {
    // Grid labels for week starting Mon 28 Sep 2026: 28 29 30 1 2 3 4 | 5 ...
    const int expected[] = {28, 29, 30, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    for (int i = 0; i < 14; i++) {
        TEST_ASSERT_EQUAL_INT(expected[i], dayOfMonthAfter("2026-09-28", i));
    }
}

void test_day_of_month_labels_across_february() {
    TEST_ASSERT_EQUAL_INT(28, dayOfMonthAfter("2027-02-22", 6));
    TEST_ASSERT_EQUAL_INT(1, dayOfMonthAfter("2027-02-22", 7));
    TEST_ASSERT_EQUAL_INT(29, dayOfMonthAfter("2028-02-21", 8));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_days_from_civil_epoch);
    RUN_TEST(test_civil_round_trip);
    RUN_TEST(test_day_number_same_month);
    RUN_TEST(test_day_number_across_30_day_month);
    RUN_TEST(test_day_number_across_february);
    RUN_TEST(test_day_number_across_year_end);
    RUN_TEST(test_day_number_with_time_component);
    RUN_TEST(test_day_number_before_week_start);
    RUN_TEST(test_day_of_month_labels_across_30_day_month);
    RUN_TEST(test_day_of_month_labels_across_february);

    return UNITY_END();
}
