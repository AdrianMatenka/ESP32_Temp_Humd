/*
 * @file mqtt.c
 * @brief MQTT implementation for the BME280 sensor project.
 * This file contains the implementation for initializing, starting, stopping, and deinitializing the MQTT client.
 */

#include "mqtt.h"
#include "config.h"

#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdint.h>
#include <stdbool.h>

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool is_connected = false;
static SemaphoreHandle_t publish_ack_sem = NULL;

/**
 * @brief Handle MQTT events
 * @param handler_args Arguments for the event handler
 * @param base Base of the event
 * @param event_id ID of the event
 * @param event_data Data associated with the event
 */
static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data);

/**
 * @brief Initialize the MQTT client
 * @return ESP_OK on success, ESP_FAIL on failure
 */                               
esp_err_t mqtt_init(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,

        .credentials.username = ADAFRUIT_IO_USERNAME,
        .credentials.authentication.password = ADAFRUIT_IO_KEY,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT client initialized");

    
    publish_ack_sem = xSemaphoreCreateCounting(3, 0);

    if (publish_ack_sem == NULL) {
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = esp_mqtt_client_register_event(
        mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register MQTT event handler");

        vSemaphoreDelete(publish_ack_sem);
        publish_ack_sem = NULL;

        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;

        return ret;
    }

    return ESP_OK;
}

/**
 * @brief Start the MQTT client
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_start(void)
{
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_mqtt_client_start(mqtt_client);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "MQTT client started");

    return ESP_OK;
}

/**
 * @brief Stop the MQTT client
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_stop(void)
{
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_mqtt_client_stop(mqtt_client);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop MQTT client: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "MQTT client stopped");

    return ESP_OK;
}

/**
 * @brief Deinitialize the MQTT client
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_deinit(void)
{
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_mqtt_client_destroy(mqtt_client);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to destroy MQTT client: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    mqtt_client = NULL;

    if (publish_ack_sem != NULL) {
        vSemaphoreDelete(publish_ack_sem);
        publish_ack_sem = NULL;
    }

    is_connected = false;

    ESP_LOGI(TAG, "MQTT client deinitialized");

    return ESP_OK;
}

/**
 * @brief Publish a message on an MQTT feed
 * @param feed The MQTT feed to publish to
 * @param value The message to publish
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_publish(const char *feed, const char *value)
{
    if (mqtt_client == NULL || !is_connected) {
        ESP_LOGE(TAG, "MQTT client is not connected");
        return ESP_ERR_INVALID_STATE;
    }

    if(feed == NULL || value == NULL) {
        ESP_LOGE(TAG, "Feed or value is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    char topic[128];

    int len = snprintf(topic, sizeof(topic),
                   "%s/feeds/%s", ADAFRUIT_IO_USERNAME, feed);

    if (len < 0 || len >= sizeof(topic)) {
        ESP_LOGE(TAG, "MQTT topic is too long");
        return ESP_ERR_INVALID_SIZE;
    }

    int msg_id = esp_mqtt_client_publish(
        mqtt_client,
        topic,
        value,
        0,
        1,
        0
    );

    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish MQTT message");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Published to %s: %s", topic, value);

    return ESP_OK;
}

/**
 * @brief Check if the MQTT client is connected
 * @return true if connected, false otherwise
 */
bool mqtt_is_connected(void)
{
    return is_connected;
}

/**
 * @brief Wait for a specified number of messages to be published
 * @param count The number of messages to wait for
 * @param timeout_ms The timeout in milliseconds
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t mqtt_wait_for_published(uint8_t count, uint32_t timeout_ms)
{
    if (publish_ack_sem == NULL || count == 0 || count > 3) {
        return ESP_ERR_INVALID_ARG;
    }

    TickType_t start = xTaskGetTickCount();
    TickType_t timeout = pdMS_TO_TICKS(timeout_ms);

    for (uint8_t i = 0; i < count; i++) {

        TickType_t elapsed = xTaskGetTickCount() - start;

        if (elapsed >= timeout) {
            ESP_LOGE(TAG, "Timeout waiting for MQTT acknowledgements");
            return ESP_ERR_TIMEOUT;
        }

        TickType_t remaining = timeout - elapsed;

        if (xSemaphoreTake(publish_ack_sem, remaining) != pdTRUE) {
            ESP_LOGE(TAG, "Timeout waiting for MQTT acknowledgements");
            return ESP_ERR_TIMEOUT;
        }
    }

    ESP_LOGI(TAG, "Received all %u MQTT acknowledgements", count);

    return ESP_OK;
}

/**
 * @brief Handle MQTT events
 * @param handler_args Arguments for the event handler
 * @param base Base of the event
 * @param event_id ID of the event
 * @param event_data Data associated with the event
 */
static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            is_connected = true;
            ESP_LOGI(TAG, "Connected to MQTT broker");
            break;

        case MQTT_EVENT_DISCONNECTED:
            is_connected = false;
            ESP_LOGW(TAG, "Disconnected from MQTT broker");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT connection error");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Message published and acknowledged");

            if (publish_ack_sem != NULL) {
                xSemaphoreGive(publish_ack_sem);
            }

            break;

        default:
            break;
    }
}


