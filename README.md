# LD2410C Zone Detection with OLED Display and WLED Control

This project utilizes the **LD2410C radar sensor** for zone-based presence detection and integrates it with **WLED** for lighting control. The system monitors movements and presence within defined zones, displays real-time status on an **OLED screen**, and dynamically controls WLED lighting based on detected activity.
![image](https://github.com/user-attachments/assets/f3f71609-12d4-4ca6-921f-cddf8c4168d7)

### Features:
- **Presence Detection**: Detects both stationary and moving targets using the LD2410C radar sensor.
- **OLED Display**: Displays zone status, target presence, and distance measurements.
- **WLED Integration**: Automatically turns WLED lighting on/off based on radar data.
- **EEPROM Storage**: Saves zone number and WLED IP address for persistence across device restarts.
- **Wi-Fi Configuration**: WiFiManager is used for easy Wi-Fi and device setup through a configuration portal.
- **Customizable Zones**: Dynamically adjust the number of zones to be detected.

### Components:
- **LD2410C Radar Sensor**: For presence and motion detection.
- **Carenuity_C3-Mini**: The microcontroller that handles Wi-Fi, radar communication, and WLED control. [STORE](https://www.chipglobe.shop/) 
- **OLED Display (SSD1306)**: Displays real-time detection and zone information.
- **WLED**: A lighting control system controlled via Wi-Fi.

### Setup:
1. **Radar Setup**: Connect LD2410C radar sensor to the ESP32 via specified UART pins.
2. **OLED Display**: Connect the SSD1306 OLED display to the I2C bus of the ESP32.
3. **WLED**: Configure the IP address of the WLED setup via the configuration portal.
4. **Wi-Fi Configuration**: Hold the reset button for 5 seconds to enter the WiFiManager portal and set Wi-Fi credentials, zone, and WLED IP.

### Usage:
- The system will continuously monitor presence within the specified zones on successful setup.
- It will control the WLED brightness based on whether a person is detected within the radar’s range.
- The OLED screen provides visual feedback on the zone's status, target presence, and distance.

### Applications:
- **Smart Home Automation**: Automatically control lighting in different rooms based on occupancy.
- **Security Systems**: Detect movement and control lighting in secure zones.
- **Energy Efficiency**: Ensure lighting is only active when someone is in the area.

### Dependencies:
- **WiFiManager**: For easy Wi-Fi configuration.
- **Adafruit SSD1306 and GFX Libraries**: To drive the OLED display.
- **HTTPClient**: To control WLED over a local network.
