# Testing Guide

This project includes unit tests for the embedded calendar application. The tests run natively on your PC (not on the microcontroller) for fast feedback during development.

## Test Structure

Tests are organized in the `test/` directory:

```
test/
├── test_battery/
│   └── test_battery_percent.cpp     # Tests for battery percentage calculation
├── test_datetime/
│   └── test_parse_datetime.cpp      # Tests for date/time parsing
└── test_ha_client/
    └── test_json_parsing.cpp        # Tests for JSON parsing using sample data
```

## What's Tested

### ✅ Pure Logic Functions (Native Tests)

These functions have no hardware dependencies and can be tested on your PC:

1. **Battery Calculations** ([utilities.cpp:112](src/utilities.cpp#L112))
   - `calcBatPercent()` - Battery percentage calculation using sigmoidal approximation
   - Tests edge cases: full battery, empty battery, mid-range values, clamping

2. **Date/Time Parsing** ([utilities.cpp:225](src/utilities.cpp#L225))
   - `parseHADateTime()` - Parses Home Assistant date/time strings
   - Tests valid dates, edge cases (New Year's, Dec 31), invalid input
   - Tests day-of-week calculation (Zeller's congruence algorithm)

3. **JSON Parsing** ([ha_client.cpp:60](src/ha_client.cpp#L60))
   - HAClient JSON response parsing using sample data
   - Tests metadata extraction, event parsing, timed vs full-day events
   - Tests helper functions: `extractTime()`, `isFullDayEvent()`, `calculateDayNumber()`

## Prerequisites

To run native tests on Windows, you need a C/C++ compiler:

### Option 1: Install MinGW-w64
1. Download from: https://www.mingw-w64.org/downloads/
2. Install and add to PATH
3. Verify: `gcc --version`

### Option 2: Install MSYS2 (Recommended)
1. Download from: https://www.msys2.org/
2. Install and run MSYS2
3. Install toolchain: `pacman -S mingw-w64-x86_64-toolchain`
4. Add `C:\msys64\mingw64\bin` to your PATH

### Option 3: Use WSL (Windows Subsystem for Linux)
1. Install WSL2
2. Install build tools: `sudo apt install build-essential`
3. Run tests from within WSL

## Running Tests

Once you have a compiler installed:

### Run All Tests
```bash
pio test -e native
```

### Run Specific Test Suite
```bash
pio test -e native -f test_battery
pio test -e native -f test_datetime
pio test -e native -f test_ha_client
```

### Run with Verbose Output
```bash
pio test -e native -vvv
```
