/**
 * @file config.h
 * @brief Configuration header for the BME280 sensor project.
 *
 * This file contains configuration settings and definitions used throughout the project.
 */

#ifndef CONFIG_H
#define CONFIG_H

/**
 * @brief I2C configuration for BME280 sensor
 */
#define BME280_SDA_PIN                  GPIO_NUM_21
#define BME280_SCL_PIN                  GPIO_NUM_22

/**
 * @brief WiFi configuration
 */
#define WIFI_SSID                       "SSID"
#define WIFI_PASSWORD                   "PASSWORD"

#define ADAFRUIT_IO_USERNAME            "USERNAME"
#define ADAFRUIT_IO_KEY                 "IO_KEY"
#define MQTT_BROKER_URI                 "mqtts://io.adafruit.com:8883"

#define INTERVAL_BETWEEN_READINGS_US    (15ULL * 60ULL * 1000000ULL)  // 15 minutes

/**
 * @brief LED configuration
 */
#define LED_PIN                         GPIO_NUM_2

#endif // CONFIG_H