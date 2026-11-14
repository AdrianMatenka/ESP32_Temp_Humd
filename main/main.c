#include "app_controller/app_controller.h"
#include "esp_log.h"
#include "esp_err.h"
#include "stdio.h"
#include "config.h"
#include "esp_system.h"
#include "esp_sleep.h"

static const char *TAG = "MAIN";


void app_main(void)
{
    ESP_LOGI(TAG, "Starting application controller...");

        
    esp_err_t ret = app_controller_init();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Application controller initialization failed: %s", esp_err_to_name(ret));
        esp_restart();
    }

    ret = app_controller_start();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Application controller start failed: %s", esp_err_to_name(ret));
        esp_restart();
    }

    ret = app_controller_deinit();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Application controller deinitialization failed: %s", esp_err_to_name(ret));
        esp_restart();
    }

    esp_sleep_enable_timer_wakeup(INTERVAL_BETWEEN_READINGS_MS);
    esp_deep_sleep_start(); 
}