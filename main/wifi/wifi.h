/*
    * @file wifi.h
    * @brief WiFi Interface
    * 
    * This header file defines the interface for initializing and managing WiFi connections.
*/

#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
#include "stddef.h"

/*
 *  @brief Initialize WiFi
 *  @return ESP_OK if successful, otherwise an error code
 */
esp_err_t wifi_init(void);

/**
 *  @brief Connect to WiFi
 *  @return ESP_OK if successful, otherwise an error code
 */
esp_err_t wifi_connect(void);

/**
 *  @brief Disconnect from WiFi
 *  @return ESP_OK if successful, otherwise an error code
 */
esp_err_t wifi_disconnect(void);

/**
 *  @brief Get the IP address of the WiFi connection
 *  @param ip_buffer Buffer to store the IP address
 *  @param buffer_len Length of the buffer
 *  @return ESP_OK if successful, otherwise an error code
 */
esp_err_t wifi_get_ip(char *ip_buffer, size_t buffer_len);

/**
 *  @brief Deinitialize WiFi
 *  @return ESP_OK if successful, otherwise an error code
 */
esp_err_t wifi_module_deinit(void);

#endif // WIFI_H