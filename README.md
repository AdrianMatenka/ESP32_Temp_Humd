# ESP32 Temperature & Humidity Monitor

An IoT environmental monitoring project based on **ESP32** and the **BME280** sensor. The device measures temperature, humidity and atmospheric pressure, sends the readings to **Adafruit IO** using MQTT over TLS, and enters Deep Sleep to reduce power consumption.

## Features

- Temperature, humidity and atmospheric pressure measurement
- BME280 communication over I2C
- Wi-Fi connectivity using ESP-IDF
- MQTT communication with Adafruit IO over TLS
- QoS 1 message delivery with acknowledgement handling
- Automatic Wi-Fi and MQTT connection management
- Deep Sleep between measurement cycles
- Configurable measurement interval

## Hardware

| Component | Description |
|---|---|
| ESP32 DevKit V1 | Main microcontroller |
| BME280 | Temperature, humidity and pressure sensor |

### Wiring

| BME280 | ESP32 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| CSB | 3.3V |
| SDO | GND |

The sensor uses I2C address `0x76`.

## Software and Technologies

- C
- ESP-IDF v6.1
- FreeRTOS
- I2C
- Wi-Fi
- MQTT
- TLS
- Adafruit IO
- Bosch BME280 SensorAPI

## How It Works

1. ESP32 wakes up from Deep Sleep.
2. The Wi-Fi module initializes and connects to the network.
3. The MQTT client establishes a secure connection to Adafruit IO.
4. The BME280 sensor is initialized and measurements are collected.
5. Temperature, humidity and pressure are published to separate Adafruit IO feeds.
6. The device waits for MQTT QoS 1 acknowledgements.
7. Peripherals and communication modules are deinitialized.
8. ESP32 enters Deep Sleep until the next measurement cycle.

## Configuration

Before building the project, configure your credentials in `main/config.h`.

You need to provide:
- Wi-Fi SSID and password
- Adafruit IO username
- Adafruit IO key
- Measurement interval

**Important:** Do not commit real Wi-Fi credentials or Adafruit IO keys to a public repository. Keep your private configuration out of Git.

## Building and Flashing

The project requires ESP-IDF v6.1 and its toolchain.

Build the project:

```bash
idf.py build
```

Flash the firmware:

```bash
idf.py -p COM4 flash
```

Monitor serial output:

```bash
idf.py -p COM4 monitor
```

## Project Status

The basic functionality has been implemented and tested:

- BME280 sensor communication
- Wi-Fi connection
- Secure MQTT connection to Adafruit IO
- Publishing sensor readings
- MQTT acknowledgement handling
- Deep Sleep measurement cycle

Long-term stability and power consumption testing are ongoing.

## License

Add a project license here if you intend to distribute the source code.