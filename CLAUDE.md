# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a PlatformIO embedded project targeting the DFRobot FireBeetle 2 ESP32-E development board using the Arduino framework. The project appears to be a calendar application for embedded systems.

## Development Commands

### Building and Running
- `pio run` - Build the project
- `pio run -t upload` - Build and upload to the connected device
- `pio run -t monitor` - Start serial monitor to view output
- `pio run -t upload -t monitor` - Upload and immediately start monitoring

### Testing and Analysis
- `pio test` - Run unit tests
- `pio check` - Run static code analysis
- `pio run -t clean` - Clean build files

### Device Management
- `pio device list` - List connected devices
- `pio device monitor` - Monitor serial output from device

## Project Structure

- `src/main.cpp` - Main application entry point with setup() and loop() functions
- `include/` - Project header files (.h files)
- `lib/` - Project-specific libraries (compiled to static libraries)
- `test/` - Unit test files
- `platformio.ini` - PlatformIO configuration file defining build environment and dependencies

## Target Hardware

- **Board**: DFRobot FireBeetle 2 ESP32-E
- **Platform**: Espressif ESP32
- **Framework**: Arduino

## Development Environment

The project is configured to work with VS Code and the PlatformIO IDE extension. The recommended extension is `platformio.platformio-ide`.