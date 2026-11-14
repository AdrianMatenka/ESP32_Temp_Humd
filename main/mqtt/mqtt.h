/*
 * @file mqtt.h
 * @brief MQTT header for the BME280 sensor project.
 * This file contains function declarations for initializing, starting, stopping, and deinitializing the MQTT client.
 */

#ifndef MQTT_H
#define MQTT_H

#include "esp_err.h"
#include "stdbool.h"
#include <stdint.h>

/**
 * @brief Initialize the MQTT client
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_init(void);

/**
 * @brief Start the MQTT client
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_start(void);

/**
 * @brief Stop the MQTT client
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_stop(void);

/**
 * @brief Deinitialize the MQTT client
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_deinit(void);

/**
 * @brief Publish a message on an MQTT feed
 * @param feed The MQTT feed to publish to
 * @param value The message to publish
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_publish(const char *feed, const char *value);

/**
 * @brief Check if the MQTT client is connected
 * @return true if connected, false otherwise
 */
bool mqtt_is_connected(void);

/**
 * @brief Wait for a specified number of messages to be published
 * @param count The number of messages to wait for
 * @param timeout_ms The timeout in milliseconds
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_wait_for_published(uint8_t count, uint32_t timeout_ms);

#endif // MQTT_H