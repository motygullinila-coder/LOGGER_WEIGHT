# TOREX DATA LOGGER

## ESP32-based Weight Measurement and Data Logging System

TOREX DATA LOGGER is an embedded system designed for continuous weight measurement, real-time monitoring, and automatic data recording using ESP32 microcontrollers.

The system allows receiving data from industrial RS232 weight scales, displaying measurement results on a local TFT display, providing a web-based control interface, and saving measurement data into internal flash memory.

The project is built using two ESP32 boards:

- **ESP32-2432S028R** — main controller responsible for visualization, web server, data processing, and data storage.
- **ESP32 DevKit** — communication module that receives data from RS232 weight scales and transfers it to the main controller.

---

# System Overview

The ESP32-2432S028R board is used as the central unit of the logger.

Main functions:

- Receiving weight data from ESP32 DevKit through UART
- Real-time weight visualization
- TFT display control using LVGL
- Wi-Fi access point creation
- Web server operation
- Experiment start/stop control
- CSV data logging
- Automatic file management
- Synchronization between display and web interface


System architecture:

```

RS232 Weight Scale
|
|
RS232-TTL Converter
|
|
ESP32 DevKit
|
| UART 115200
|
ESP32-2432S028R
|
+----------------+
|                |
TFT Display      Wi-Fi Server
|                |
|                |
LVGL          Web Browser
|
LittleFS

```

---

# Features

## Real-Time Weight Monitoring

The system continuously receives weight values from external scales and displays them in real time.

Measurement data is available on:

- Built-in TFT display
- Web interface
- Serial monitor


---

## Web Interface

The ESP32-2432S028R creates its own Wi-Fi access point.

Default settings:

```

SSID:
Torex

Password:
Torex12345

```

After connecting to the network, the user can open the web interface and control the logger.

The web interface provides:

- Current time display
- Current weight value
- Logger status
- Real-time graph
- Latest measurements table
- Experiment control buttons
- CSV file download


---

# Experiment Control

The logger supports experiment-based data recording.

## START

When the START button is pressed:

- A new CSV file is created
- Data recording begins
- Logger status changes to RUN
- TFT indicator changes to green


## STOP

When the STOP button is pressed:

- Data recording stops
- Current file is closed
- Logger status changes to STOP
- TFT indicator changes to red


## DOWNLOAD

The user can download the latest experiment file directly from the web interface.

---

# Local TFT Display

The ESP32-2432S028R uses an integrated 2.8-inch TFT display.

The graphical interface is implemented using:

- LVGL framework
- LovyanGFX display driver


Displayed information:

- Current date
- Current time
- Weight value
- Measurement chart
- Logger status


Status indication:

| Status | Indicator |
|--------|-----------|
| STOP | Red |
| RUN | Green |


---

# Data Storage

Measurement data is stored using the ESP32 internal flash filesystem:

```

LittleFS

```

Each experiment creates a separate CSV file.

Example:

```

/experiments/

exp_20260708_120000.csv
exp_20260708_153500.csv
exp_20260708_181200.csv

````

CSV file format:

```csv
time,weight
2026-07-08 12:00:01,125.430
2026-07-08 12:00:02,125.432
2026-07-08 12:00:03,125.431
````

---

# Automatic File Management

To prevent memory overflow, the system automatically removes old experiment files.

Storage rules:

* Maximum file lifetime: **3 days**
* Old CSV files are automatically deleted
* Cleanup is performed before starting a new experiment

---

# Time Synchronization

The system supports real-time timestamp generation.

The browser sends the current date and time to the ESP32 controller.

The device stores time using the internal ESP32 RTC system.

Every recorded measurement contains:

* Date
* Time
* Weight value

---

# Communication Protocol

## ESP32 DevKit → ESP32-2432S028R

UART communication:

```
Baud rate: 115200
Format: 8N1
```

Data transmission example:

```
125.430
```

The main controller receives the value, processes it, updates the interface, and stores it if logging is active.

---

# Hardware Configuration

## Main Controller

Board:

```
ESP32-2432S028R
```

Functions:

* TFT display
* Wi-Fi server
* LVGL interface
* Data logger

UART:

```
RX: GPIO22
TX: GPIO27

Baud rate:
115200
```

---

## Communication Controller

Board:

```
ESP32 DevKit
```

Functions:

* RS232 communication
* Data forwarding

UART to ESP32-2432S028R:

```
RX: GPIO25
TX: GPIO26

Baud rate:
115200
```

UART to weight scale:

```
RX: GPIO16
TX: GPIO17

Baud rate:
9600
```

---

# Software Architecture

The project uses FreeRTOS multitasking capabilities.

Main tasks:

## Display Task

Responsible for:

* LVGL rendering
* Display updates
* Chart refresh
* Interface synchronization

## Main Loop

Responsible for:

* UART data processing
* Weight parsing
* Data recording

---

# Synchronization

The project uses FreeRTOS mutexes to protect shared resources.

| Mutex      | Purpose                     |
| ---------- | --------------------------- |
| lvgl_mutex | Protect LVGL operations     |
| file_mutex | Protect LittleFS operations |
| data_mutex | Protect weight data         |

This prevents conflicts between:

* Web server callbacks
* Display task
* Data acquisition process

---

# Required Libraries

The project uses:

* ESP32 Arduino Core
* LVGL
* LovyanGFX
* ESPAsyncWebServer
* AsyncTCP
* LittleFS

---

# Project Structure

```
TOREX_DATA_LOGGER

│
├── ESP32_2432S028R
│
│   ├── LVGL Interface
│   ├── Web Server
│   ├── LittleFS Logger
│   ├── RTC Management
│   └── Data Processing
│
│
└── ESP32_DevKit
│
└── RS232 Communication Bridge
```

---

# Future Improvements

Possible future extensions:

* Touchscreen control
* SD card support
* MQTT integration
* Cloud data synchronization
* Multiple sensor support
* OTA firmware update
* Automatic calibration system
* Database export

---

# License

This project is released under the MIT License.

---

# Author

TOREX DATA LOGGER

ESP32-based industrial weight monitoring and data acquisition system.

```
```
