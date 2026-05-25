# Radar-Scanner

ESP32 + HC-SR04 ultrasonic radar scanner with live browser dashboard

# ESP32 Radar Scanner

An ESP32-based ultrasonic radar scanner using an HC-SR04 sensor, SG90 servo, OLED display, and live browser dashboard.

The servo sweeps the ultrasonic sensor through 180 degrees. At each angle, the ESP32 measures distance and sends the live angle/distance data to a web dashboard. The OLED also shows the current scan status.

## Current Status

✅ Wokwi simulation working  
✅ ESP32 web dashboard working  
✅ OLED display working  
✅ Servo sweep working  
✅ Ultrasonic distance reading working  
✅ Clean wiring layout completed

## Demo Preview

### Web Dashboard

_Add dashboard screenshot here._

### Wokwi Circuit

_Add Wokwi wiring screenshot here._

### OLED Display

_Add OLED screenshot here._

## Features

- 180 degree servo sweep
- HC-SR04 ultrasonic distance measurement
- Live radar-style browser dashboard
- Object detection status
- OLED display showing:
  - Current angle
  - Distance
  - Object status
  - WiFi dashboard status
- Wokwi simulation support
- PlatformIO project structure

## Components Used

| Component                 | Purpose                                  |
| ------------------------- | ---------------------------------------- |
| ESP32 Dev Board           | Main microcontroller and WiFi web server |
| HC-SR04 Ultrasonic Sensor | Measures object distance                 |
| SG90 Servo                | Rotates the ultrasonic sensor            |
| 0.96 inch I2C OLED        | Displays live scan data                  |
| Breadboard                | Power and signal distribution            |
| Jumper Wires              | Circuit connections                      |

## Pin Connections

| Module     |    Pin | ESP32 Pin |
| ---------- | -----: | --------: |
| HC-SR04    |   TRIG |    GPIO 5 |
| HC-SR04    |   ECHO |   GPIO 18 |
| SG90 Servo | Signal |   GPIO 13 |
| OLED       |    SDA |   GPIO 21 |
| OLED       |    SCL |   GPIO 22 |
| OLED       |    VCC |      3.3V |
| OLED       |    GND |       GND |

## How It Works

The ESP32 controls the SG90 servo and moves it from 0 to 180 degrees, then back again.

At each angle, the ESP32 triggers the HC-SR04 ultrasonic sensor. The sensor returns an echo pulse, which is used to calculate distance:

```text
distance = (time x speed of sound) / 2
```
