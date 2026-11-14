/*
 * @file app_controller.c
 * @brief Application controller for the BME280 sensor project.
 * This file contains the implementation for the application controller functions.
 */

#include "app_controller.h"
#include "bme280_sensor.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"

#include "stdbool.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stdio.h"

static const char *TAG = "APP_CONTROLLER";

static bme280_measurement_t data;

/*
 * @brief Initialize the application controller.
 * @return ESP_OK if successful, otherwise an error code.
 */
esp_err_t app_controller_init(void)
{
     esp_err_t ret = wifi_init();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Connecting to WiFi...");

    ret = wifi_connect();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connect failed: %s", esp_err_to_name(ret));
        return ret;
    }

    char ip[16] = {0};
    bool connected = false;

    for (int i = 0; i < 20; i++) {
        ret = wifi_get_ip(ip, sizeof(ip));

        if (ret == ESP_OK) {
            connected = true;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (!connected) {
        ESP_LOGE(TAG, "Failed to obtain WiFi IP");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "My IP address: %s", ip);

    ret = mqtt_init();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MQTT initialization failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ret = mqtt_start();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MQTT start failed: %s",
                esp_err_to_name(ret));
        mqtt_deinit();
        return ret;
    }

    bool mqtt_connected = false;

    for (int i = 0; i < 20; i++) {
        if (mqtt_is_connected()) {
            mqtt_connected = true;
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (!mqtt_connected) {
        ESP_LOGE(TAG, "MQTT connection timeout");
        return ESP_FAIL;
    }

    ret = bme280_sensor_init();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BME280 sensor initialization failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Application controller initialized");
    return ESP_OK;
}


/*
 * @brief Start the application controller.
 * @return ESP_OK if successful, otherwise an error code.
 */
esp_err_t app_controller_start(void)
{
    char temperature_str[16];
    char pressure_str[16];
    char humidity_str[16];

    esp_err_t ret;

    for (int i = 0; i < 3; i++)
    {
        ret = bme280_sensor_read(&data);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "BME280 read failed: %s", esp_err_to_name(ret));
            return ret;
        }

        ESP_LOGI(TAG, "Reading %d: T=%.2f C, H=%.2f %%, P=%.2f hPa",
                i + 1, data.temperature, data.humidity, data.pressure);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    snprintf(temperature_str, sizeof(temperature_str), "%.2f", data.temperature);
    ret = mqtt_publish("temperature", temperature_str);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MQTT publish failed: %s",
                esp_err_to_name(ret));
        return ret;
    } else {
        ESP_LOGI(TAG, "Published temperature data to MQTT");
    }

    snprintf(pressure_str, sizeof(pressure_str), "%.2f", data.pressure);
    ret = mqtt_publish("pressure", pressure_str);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MQTT publish failed: %s",
                esp_err_to_name(ret));
        return ret;
    } else {
        ESP_LOGI(TAG, "Published pressure data to MQTT");
    }

    snprintf(humidity_str, sizeof(humidity_str), "%.2f", data.humidity);
    ret = mqtt_publish("humidity", humidity_str);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MQTT publish failed: %s",
                esp_err_to_name(ret));
        return ret;
    } else {
        ESP_LOGI(TAG, "Published humidity data to MQTT");
    }
    
    ret = mqtt_wait_for_published(3, 10000);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MQTT publish confirmation failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Application controller started");
    return ESP_OK;
}

/**
 * @brief Deinitialize the application controller.
 * @return ESP_OK if successful, otherwise an error code.
 */
esp_err_t app_controller_deinit(void)
{
    esp_err_t ret = bme280_sensor_deinit();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BME280 sensor deinitialization failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ret = mqtt_stop();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MQTT stop failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ret = mqtt_deinit();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MQTT deinitialization failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ret = wifi_disconnect();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi disconnect failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ret = wifi_module_deinit();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi deinitialization failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Application controller deinitialized");
    return ESP_OK;
}
