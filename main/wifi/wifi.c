/*
 * @file wifi.c
 * @brief WiFi Implementation
 * This file contains the implementation for initializing and managing WiFi connections.
 */

#include "wifi.h"
#include "config.h"
#include "stdbool.h"
#include <stdio.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif_ip_addr.h"

static const char *TAG = "WiFi";
static bool is_initialized = false;
static bool is_connected = false;
static bool reconnect_enabled = true;
static bool is_started = false;

static esp_netif_t *sta_netif = NULL;

static esp_event_handler_instance_t wifi_event_instance;
static esp_event_handler_instance_t ip_event_instance;

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started");

                esp_err_t ret = esp_wifi_connect();

                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Connection failed: %s",
                            esp_err_to_name(ret));
                }

                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "WiFi disconnected");

                is_connected = false;

                if (reconnect_enabled) {
                    esp_err_t ret = esp_wifi_connect();

                    if (ret != ESP_OK) {
                        ESP_LOGE(TAG, "Reconnect failed: %s",
                                esp_err_to_name(ret));
                    }
                }

                break;

            default:
                break;
        }
    }
    else if (event_base == IP_EVENT &&
            event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        is_connected = true;

        ESP_LOGI(TAG, "Got IP: " IPSTR,
                IP2STR(&event->ip_info.ip));
    }
}

/*
 * @brief Initialize WiFi
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t wifi_init(void) {
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_netif_init();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Network interface initialization failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_loop_create_default();

    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Event loop creation failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    sta_netif = esp_netif_create_default_wifi_sta();

    if (sta_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi station interface");
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ret = esp_wifi_init(&cfg);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi driver initialization failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        &wifi_event_instance
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi event handler registration failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        &ip_event_instance
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "IP event handler registration failed: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_set_mode(WIFI_MODE_STA);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s",
                esp_err_to_name(ret));
        return ret;
    }

    is_initialized = true;

    ESP_LOGI(TAG, "WiFi initialized successfully");

    return ESP_OK;
}

/**
 * @brief Connect to WiFi
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t wifi_connect(void)
{
    if (!is_initialized) {
        ESP_LOGE(TAG, "WiFi is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (is_connected) {
        ESP_LOGI(TAG, "Already connected to WiFi");
        return ESP_OK;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        }
    };

    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi configuration: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    reconnect_enabled = true;

    if (!is_started) {
        ret = esp_wifi_start();

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start WiFi: %s",
                     esp_err_to_name(ret));
            return ret;
        }

        is_started = true;
        return ESP_OK;
    }

    ret = esp_wifi_connect();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect to WiFi: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief Disconnect from WiFi
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t wifi_disconnect(void)
{
    if (!is_initialized) {
        ESP_LOGE(TAG, "WiFi is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!is_started) {
        ESP_LOGW(TAG, "WiFi is already stopped");
        return ESP_OK;
    }

    reconnect_enabled = false;

    esp_err_t ret = esp_wifi_disconnect();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disconnect WiFi: %s",
                esp_err_to_name(ret));
        return ret;
    }

    is_connected = false;

    return ESP_OK;
}

/**
 * @brief Get the IP address of the WiFi station
 * @param ip_buffer Buffer to store the IP address
 * @param buffer_len Length of the buffer
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t wifi_get_ip(char *ip_buffer, size_t buffer_len)
{
    if (!is_initialized || !is_connected) {
        ESP_LOGE(TAG, "WiFi is not connected");
        return ESP_ERR_INVALID_STATE;
    }

    if (ip_buffer == NULL || buffer_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_netif_ip_info_t ip_info;

    esp_err_t ret = esp_netif_get_ip_info(sta_netif, &ip_info);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get IP information: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    int len = snprintf(ip_buffer, buffer_len, IPSTR,
                       IP2STR(&ip_info.ip));

    if (len < 0 || (size_t)len >= buffer_len) {
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

/**
 * @brief Deinitialize WiFi
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t wifi_module_deinit(void) {
    if (!is_initialized) {
        ESP_LOGE(TAG, "WiFi is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    reconnect_enabled = false;

    esp_err_t ret = ESP_OK;

    if (is_started) {
        ret = esp_wifi_stop();

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to stop WiFi: %s",
                    esp_err_to_name(ret));
            return ret;
        }

        is_started = false;
    }

    ret = esp_event_handler_instance_unregister(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        ip_event_instance
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unregister IP handler: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_instance_unregister(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        wifi_event_instance
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unregister WiFi handler: %s",
                esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_deinit();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize WiFi: %s",
                esp_err_to_name(ret));
        return ret;
    }

    esp_netif_destroy_default_wifi(sta_netif);
    sta_netif = NULL;

    is_initialized = false;
    is_connected = false;
    reconnect_enabled = true;
    is_started = false;

    ESP_LOGI(TAG, "WiFi deinitialized successfully");

    return ESP_OK;
}