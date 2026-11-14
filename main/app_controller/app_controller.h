/*
 * @file app_controller.h
 * @brief Application controller for the BME280 sensor project.
 * This file contains the declaration for the application controller functions.
 */

#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "esp_err.h"

/**
 * @brief Initialize the application controller
 * @return ESP_OK on success, error code on failure
 */
esp_err_t app_controller_init(void);

/**
 * @brief Start the application controller
 * @return ESP_OK on success, error code on failure
 */
esp_err_t app_controller_start(void);

/**
 * @brief Deinitialize the application controller
 * @return ESP_OK on success, error code on failure
 */
esp_err_t app_controller_deinit(void);

#endif // APP_CONTROLLER_H