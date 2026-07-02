# LOGGER_WEIGHT
Project Description: Smart Weight Scale Terminal on ESP32-2432S028R

This project is an intelligent weight scale terminal based on the ESP32-2432S028R microcontroller with a 2.8-inch color TFT display (240×320 resolution). The device reads real-time data from industrial weighing scales via UART/RS-232 interface at 115200 baud, displays measurements on the built-in screen using the LVGL (Light and Versatile Graphics Library), and provides a web-based interface for remote monitoring and control through a Wi-Fi access point.
Users can visually monitor the weighing process, observe weight change dynamics on a real-time chart, and control data logging to the LittleFS file system with automatic file rotation (deleting data older than 3 days).

Key Features

1. Data Acquisition
Reads weight data from industrial scales via UART (Serial1) at 115200 baud
RS-232 to TTL conversion via external module
Real-time data parsing using character-based approach (isdigit(), atof())
Timer-based interrupt reading at 100 Hz (10 ms interval) for accurate data capture
Volatile global variable data_weight stores current weight value

2. Local Display (LVGL GUI)
Touchscreen UI built with LVGL library (version 8.x)
Header panel: Displays "TOREX LOGGER" title with current date and time
Main panel: Real-time weight display with large, readable numbers (grams)
Status indicator: Visual circle indicator (green for stable, red for stop)
Dynamic weight chart: 100-point rolling chart showing measurement history
Control buttons: START, STOP, SAVE (currently placeholder)
Responsive layout: 320×240 resolution optimized for 2.8" display

3. Web Server Interface
Embedded AsyncWebServer running on ESP32
Device acts as a Wi-Fi Access Point (SSID: Torex, password: Torex12345)
REST API endpoints:
GET / — Main HTML dashboard with real-time weight updates
GET /getData — Returns current weight as plain text
GET /start — Starts experiment and creates new log file
GET /stop — Stops experiment and closes log file
GET /downloadData — Downloads current experiment CSV file
GET /getDate — Sets ESP32 RTC time from client

Web dashboard features:
Live weight display (auto-refresh every 500 ms)
Real-time weight chart (HTML5 Canvas with auto-scaling)
Experiment status indicator (RUN/STOP with color coding)
Last 15 measurements displayed in table
One-click Start/Stop/Download buttons
Fully responsive design (mobile-friendly)
Time synchronization with client browser

4. Data Logging System
Logs weight data to LittleFS (flash file system)
CSV format: time,weight (e.g., 2026-06-30 10:15:23,15.400)
File naming: exp_YYYYMMDD_HHMMSS.csv (timestamped at creation)
Thread-safe logging using mutex semaphores (file_mutex)
Automatic file creation on experiment start
Real-time data flushing to ensure data integrity
Manual download via web interface

5. Auto-Deletion System
Runs automatically when starting a new experiment
Scans /experiments/ directory for .csv files
Parses filename timestamps (format: exp_YYYYMMDD_HHMMSS.csv)
Deletes files older than 3 days (259,200 seconds)
Prevents storage overflow on 4MB flash ESP32
