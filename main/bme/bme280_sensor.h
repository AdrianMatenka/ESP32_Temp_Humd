/**
 * @file bme280_sensor.h
 * @brief BME280 Sensor Interface
 *
 * This header file defines the interface for initializing and reading data from the BME280 sensor.
 */

#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include "esp_err.h"
#include "stdint.h"
#include "stdbool.h"

/**
 * @brief Structure to hold BME280 sensor measurements
 */
typedef struct {
    float temperature;
    float humidity;
    float pressure;
} bme280_measurement_t;

/**
 * @brief Initialize the BME280 sensor
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t bme280_sensor_init(void);

/**
 * @brief Read data from the BME280 sensor
 * @param data Pointer to the structure to store the sensor data
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t bme280_sensor_read(bme280_measurement_t *data);

/**
 *  @brief Deinitialize the BME280 sensor
 *  @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t bme280_sensor_deinit(void);

/**
 *  @brief Put the BME280 sensor to sleep
 *  @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t bme280_sensor_sleep(void);

/**
 *  @brief Wake up the BME280 sensor
 *  @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t bme280_sensor_wake(void);

/**
 *  @brief Set the mode of the BME280 sensor
 *  @param mode The mode to set
 *  @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t bme280_sensor_set_mode(uint8_t mode);

#endif /* BME280_SENSOR_H */