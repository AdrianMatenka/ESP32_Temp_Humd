/*
 * @file bme280_sensor.c
 * @brief BME280 Sensor Implementation
 *
 * This file contains the implementation for initializing and reading data from the BME280 sensor.
 */

#include "bme280_sensor.h"
#include "bme280.h"
#include "bme280_defs.h"
#include "config.h"
#include "esp_rom_sys.h"

#include "driver/i2c_master.h"
#include "esp_log.h"


static const char *TAG = "BME280";
static bool is_initialized = false;

/* BME280 driver variables */
static struct bme280_dev dev;
static struct bme280_settings settings;

/* I2C master bus and device handles */
static i2c_master_bus_handle_t i2c_bus_handle;
static i2c_master_dev_handle_t bme280_i2c_handle;

/* BME280 I2C device configuration */
static const i2c_device_config_t bme280_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x76,
    .scl_speed_hz = 100000,
};

/* 
 * @brief I2C master initialization 
 */
static esp_err_t i2c_master_init(void)
{
    i2c_master_bus_config_t i2c_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = BME280_SDA_PIN,
        .scl_io_num = BME280_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&i2c_config, &i2c_bus_handle);
}

/**
 * @brief I2C read function for BME280 sensor
 * @param reg_addr Register address to read from
 * @param reg_data Buffer to store the read data
 * @param len Length of data to read
 * @param intf_ptr Interface pointer
 * @return BME280_INTF_RET_TYPE
 */
static BME280_INTF_RET_TYPE bme280_i2c_read(
    uint8_t reg_addr,
    uint8_t *reg_data,
    uint32_t len,
    void *intf_ptr)
{
    i2c_master_dev_handle_t dev_handle =
        *(i2c_master_dev_handle_t *)intf_ptr;

    esp_err_t err = i2c_master_transmit_receive(
        dev_handle,
        &reg_addr,
        1,
        reg_data,
        len,
        -1
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(err));
        return -1;
    }

    return BME280_INTF_RET_SUCCESS;
}

/**
 * @brief I2C write function for BME280 sensor
 * @param reg_addr Register address to write to
 * @param reg_data Buffer containing the data to write
 * @param len Length of data to write
 * @param intf_ptr Interface pointer
 * @return BME280_INTF_RET_TYPE
 */
static BME280_INTF_RET_TYPE bme280_i2c_write(
    uint8_t reg_addr,
    const uint8_t *reg_data,
    uint32_t len,
    void *intf_ptr)
{
    i2c_master_dev_handle_t dev_handle =
        *(i2c_master_dev_handle_t *)intf_ptr;

    uint8_t buffer[len + 1];

    buffer[0] = reg_addr;

    for (uint32_t i = 0; i < len; i++)
    {
        buffer[i + 1] = reg_data[i];
    }

    esp_err_t err = i2c_master_transmit(
        dev_handle,
        buffer,
        len + 1,
        -1
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(err));
        return -1;
    }

    return BME280_INTF_RET_SUCCESS;
}

/**
 * @brief Delay function for BME280 sensor
 * @param period Delay period in microseconds
 * @param intf_ptr Interface pointer
 */
static void bme280_delay_us(uint32_t period, void *intf_ptr)
{
    esp_rom_delay_us(period);
}

/**
 * @brief Initialize the BME280 sensor
 * @return esp_err_t
 */
esp_err_t bme280_sensor_init(void)
{
    int8_t rslt;

    // Initialize I2C master
    if (i2c_master_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize I2C master");
        return ESP_FAIL;
    }
    // Add BME280 device to I2C bus
    if (i2c_master_bus_add_device(i2c_bus_handle, &bme280_config, &bme280_i2c_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add BME280 device to I2C bus");
        return ESP_FAIL;
    }

    // Configure BME280 driver interface
    dev.intf = BME280_I2C_INTF;
    dev.intf_ptr = &bme280_i2c_handle;
    dev.read = bme280_i2c_read;
    dev.write = bme280_i2c_write;
    dev.delay_us = bme280_delay_us;

    // Initialize BME280 sensor
    if ((rslt = bme280_init(&dev)) != BME280_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize BME280 sensor: %d", rslt);
        return ESP_FAIL;
    }

    // Configure sensor settings
    settings.osr_t = BME280_OVERSAMPLING_2X;
    settings.osr_p = BME280_OVERSAMPLING_16X;
    settings.osr_h = BME280_OVERSAMPLING_1X;

    settings.filter = BME280_FILTER_COEFF_16;
    settings.standby_time = BME280_STANDBY_TIME_500_MS;

    // Apply sensor settings
    uint8_t settings_sel = BME280_SEL_OSR_TEMP |
                        BME280_SEL_OSR_PRESS |
                        BME280_SEL_OSR_HUM |
                        BME280_SEL_FILTER |
                        BME280_SEL_STANDBY;

    if ((rslt = bme280_set_sensor_settings(settings_sel, &settings, &dev)) != BME280_OK)
    {
        ESP_LOGE(TAG, "Failed to configure BME280: %d", rslt);
        return ESP_FAIL;
    }

    // Set sensor to normal mode
    if((rslt = bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &dev)) != BME280_OK)
    {
        ESP_LOGE(TAG, "Failed to set BME280 to normal mode: %d", rslt);
        return ESP_FAIL;
    }

    is_initialized = true;
    return ESP_OK;
};

/**
 * @brief Read data from the BME280 sensor
 * @param data Pointer to store the read data
 * @return esp_err_t
 */
esp_err_t bme280_sensor_read(bme280_measurement_t *data)
{
    if (!is_initialized)
    {
        ESP_LOGE(TAG, "BME280 sensor not initialized");
        return ESP_FAIL;
    }

    int8_t rslt;
    struct bme280_data comp_data;

    if (data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if ((rslt = bme280_get_sensor_data(
        BME280_ALL,
        &comp_data,
        &dev
    )) != BME280_OK)
    {
        ESP_LOGE(TAG, "Failed to read BME280 data: %d", rslt);
        return ESP_FAIL;
    }

    data->temperature = comp_data.temperature;
    data->humidity = comp_data.humidity;
    data->pressure = comp_data.pressure / 100.0f;

    return ESP_OK;
}

/**
 *  @brief Deinitialize the BME280 sensor
 *  @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t bme280_sensor_deinit(void)
{
    if (!is_initialized)
    {
        ESP_LOGE(TAG, "BME280 sensor not initialized");
        return ESP_FAIL;
    }
    
    int8_t rslt;

    if ((rslt = bme280_soft_reset(&dev)) != BME280_OK)
    {
        ESP_LOGE(TAG, "Failed to reset BME280 sensor: %d", rslt);
        return ESP_FAIL;
    }


    if(i2c_master_bus_rm_device(bme280_i2c_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to remove BME280 device from I2C bus");
        return ESP_FAIL;
    }

    if(i2c_del_master_bus(i2c_bus_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to delete I2C master bus");
        return ESP_FAIL;
    }

    is_initialized = false;
    return ESP_OK;
}

/**
 *  @brief Put the BME280 sensor to sleep
 *  @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t bme280_sensor_sleep(void)
{
    if (!is_initialized)
    {
        ESP_LOGE(TAG, "BME280 sensor not initialized");
        return ESP_FAIL;
    }

    int8_t rslt;

    if ((rslt = bme280_set_sensor_mode(BME280_POWERMODE_SLEEP, &dev)) != BME280_OK)
    {
        ESP_LOGE(TAG, "Failed to put BME280 sensor to sleep: %d", rslt);
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 *  @brief Wake up the BME280 sensor
 *  @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t bme280_sensor_wake(void)
{
    if (!is_initialized)
    {
        ESP_LOGE(TAG, "BME280 sensor not initialized");
        return ESP_FAIL;
    }

    int8_t rslt;

    if ((rslt = bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &dev)) != BME280_OK)
    {
        ESP_LOGE(TAG, "Failed to wake up BME280 sensor: %d", rslt);
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 *  @brief Set the mode of the BME280 sensor
 *  @param mode The mode to set
 *  @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t bme280_sensor_set_mode(uint8_t mode)
{
    if (!is_initialized)
    {
        ESP_LOGE(TAG, "BME280 sensor not initialized");
        return ESP_FAIL;
    }

    if (mode != BME280_POWERMODE_SLEEP && mode != BME280_POWERMODE_NORMAL && mode != BME280_POWERMODE_FORCED)
    {
        ESP_LOGE(TAG, "Invalid BME280 sensor mode: %d", mode);
        return ESP_ERR_INVALID_ARG;
    }

    int8_t rslt;

    if ((rslt = bme280_set_sensor_mode(mode, &dev)) != BME280_OK)
    {
        ESP_LOGE(TAG, "Failed to set BME280 sensor mode: %d", rslt);
        return ESP_FAIL;
    }

    return ESP_OK;
}

