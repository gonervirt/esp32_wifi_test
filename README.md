# ESP32 WiFi Diagnostic Tool

A professional-grade diagnostic tool for ESP32, ESP32-S3, and ESP32-C3 microcontrollers. This application creates a standalone WiFi Access Point with a modern, responsive web dashboard to validate hardware performance, signal integrity, and network throughput.

## Features

*   **Web-Based Dashboard**: Responsive UI hosted directly on the ESP32 (no internet required).
*   **Throughput Testing**:
    *   **Download Speed**: Tests Server-to-Client TCP throughput.
    *   **Upload Speed**: Tests Client-to-Server TCP throughput.
*   **Latency Testing**: Measures Round-Trip Time (RTT) and packet loss.
*   **Signal Analysis**:
    *   Real-time RSSI monitoring for connected clients.
    *   WiFi Network Scanner (Site Survey).
*   **Hardware Diagnostics**:
    *   **TX Power Control**: Adjust radio power (2dBm - 19.5dBm) to diagnose power supply brownouts.
    *   **System Stats**: Monitor CPU Frequency, Free Heap, and Uptime.
    *   **LwIP Stats**: View TCP retransmissions to detect packet corruption.

## Getting Started

### Prerequisites

*   [Visual Studio Code](https://code.visualstudio.com/)
*   [PlatformIO IDE Extension](https://platformio.org/)

### Installation

1.  Clone this repository.
2.  Open the folder in VS Code.
3.  Select your environment in `platformio.ini` (default is `lolin_c3_mini`, switch to `esp32-s3` if needed).
4.  Build and Upload the firmware.

### Usage

1.  Power on the ESP32.
2.  Connect your computer or phone to the WiFi network:
    *   **SSID**: `ESP32_WiFi_Test`
    *   **Password**: `12345678`
3.  Open a web browser and navigate to:
    *   `http://192.168.4.1`

## Troubleshooting Low Throughput

If you experience low speeds (e.g., < 1 Mbps) despite good signal strength:

1.  **Check Antenna Selection**: Ensure the 0-ohm resistor on your board is set to the correct antenna (PCB vs IPEX).
2.  **Check Power Supply**: Use the "Set TX Power" buttons in the dashboard. If reducing power to "Low (2dBm)" increases speed, your power supply or USB cable is likely causing voltage drops.
3.  **Distance**: Ensure the testing device is at least 1 meter away from the ESP32 to avoid receiver saturation.

## Configuration

You can modify the default AP settings in `src/main.cpp`:

```cpp
const char* AP_SSID = "ESP32_WiFi_Test";
const char* AP_PASS = "12345678";
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.