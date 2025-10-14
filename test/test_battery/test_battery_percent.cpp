#include <unity.h>

// Mock Arduino.h for native tests
#ifndef UNIT_TEST
#include <Arduino.h>
#else
#include <stdint.h>
#include <math.h>
#endif

// Copy the function here to test it without hardware dependencies
uint32_t calcBatPercent(uint32_t v, uint32_t minv, uint32_t maxv)
{
  // normal
  uint32_t p = 105 - (105 / (1 + pow(1.724 * (v - minv)/(maxv - minv), 5.5)));
  return p >= 100 ? 100 : p;
}

void test_battery_percent_at_max_voltage() {
    // Battery at maximum voltage should return 100%
    uint32_t result = calcBatPercent(4200, 3000, 4200);
    TEST_ASSERT_EQUAL_UINT32(100, result);
}

void test_battery_percent_at_min_voltage() {
    // Battery at minimum voltage should return 0%
    uint32_t result = calcBatPercent(3000, 3000, 4200);
    TEST_ASSERT_EQUAL_UINT32(0, result);
}

void test_battery_percent_at_mid_voltage() {
    // Battery at mid voltage should return reasonable percentage
    uint32_t result = calcBatPercent(3600, 3000, 4200);
    // Should be somewhere between 20-60% based on the sigmoidal curve
    TEST_ASSERT_GREATER_THAN_UINT32(20, result);
    TEST_ASSERT_LESS_THAN_UINT32(60, result);
}

void test_battery_percent_near_max() {
    // Battery near maximum should be high percentage
    uint32_t result = calcBatPercent(4100, 3000, 4200);
    TEST_ASSERT_GREATER_THAN_UINT32(80, result);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(100, result);
}

void test_battery_percent_near_min() {
    // Battery near minimum should be low percentage
    uint32_t result = calcBatPercent(3100, 3000, 4200);
    TEST_ASSERT_LESS_THAN_UINT32(20, result);
}

void test_battery_percent_clamped_at_100() {
    // Even if voltage exceeds max, percentage should be clamped at 100
    uint32_t result = calcBatPercent(4300, 3000, 4200);
    TEST_ASSERT_EQUAL_UINT32(100, result);
}

void test_battery_percent_typical_voltages() {
    // Test some typical lithium battery voltages
    // 4.2V = 100%, 3.7V = ~50%, 3.0V = 0%

    // At 3700mV (nominal voltage), expect around 40-60%
    uint32_t result = calcBatPercent(3700, 3000, 4200);
    TEST_ASSERT_GREATER_THAN_UINT32(30, result);
    TEST_ASSERT_LESS_THAN_UINT32(70, result);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_battery_percent_at_max_voltage);
    RUN_TEST(test_battery_percent_at_min_voltage);
    RUN_TEST(test_battery_percent_at_mid_voltage);
    RUN_TEST(test_battery_percent_near_max);
    RUN_TEST(test_battery_percent_near_min);
    RUN_TEST(test_battery_percent_clamped_at_100);
    RUN_TEST(test_battery_percent_typical_voltages);

    return UNITY_END();
}
