/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_partition.h>
#include <spi_flash_mmap.h>
#include <nvs_flash.h>
#include <mooncake.h>
#include <utility/I2C_Class.hpp>
#include "utils/shared/shared.h"
#include "utils/assets/assets.h"
#include "service/service.h"
#include "hal_config.h"
#include "service/apis/apis.h"
extern "C" {
#include "utils/camera/camera_init.h"
#include "utils/ir_nec_transceiver/ir_nec_transceiver.h"
}

static void enable_camera_power()
{
    gpio_reset_pin((gpio_num_t)18);
    gpio_set_direction((gpio_num_t)18, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode((gpio_num_t)18, GPIO_PULLDOWN_ONLY);

    spdlog::info("enable camera power");
    vTaskDelay(pdMS_TO_TICKS(200));
}

static void i2c_scan(m5::I2C_Class& i2c_class)
{
    spdlog::info("start scan:");
    bool scan_list[120];
    i2c_class.scanID(scan_list);
    uint8_t device_num = 0;
    for (int i = 8; i < 0x78; i++) {
        if (scan_list[i]) {
            device_num++;
            spdlog::info("get 0x{0:x}", i);
        }
    }
    spdlog::info("device num: {}", device_num);
}

static void i2c_init()
{
    spdlog::info("i2c bus init");

    /* -------------------------------- Internal -------------------------------- */
    spdlog::info("internal bus:");
    m5::In_I2C.begin(I2C_NUM_0, HAL_PIN_I2C_INTER_SDA, HAL_PIN_I2C_INTER_SCL);
    i2c_scan(m5::In_I2C);

    // /* -------------------------------- External -------------------------------- */
    // spdlog::info("external bus:");
    // m5::Ex_I2C.begin(I2C_NUM_1, HAL_PIN_I2C_EXTER_SDA, HAL_PIN_I2C_EXTER_SCL);
    // i2c_scan(m5::Ex_I2C);
}

static void imu_init()
{
    spdlog::info("imu init");

    assert(SharedData::GetData().imu == nullptr);
    SharedData::GetData().imu = new BMI270_Class();
    if (!SharedData::GetData().imu->init()) {
        delete SharedData::GetData().imu;
        SharedData::GetData().imu = nullptr;
        spdlog::error("bmi270 init failed");
    } else {
        spdlog::info("bmi270 init ok");
    }

    if (!SharedData::GetData().imu->initAuxBmm150()) {
        SharedData::GetData().is_bmm150_ok = false;
        spdlog::error("bmm150 init failed");
    } else {
        SharedData::GetData().is_bmm150_ok = true;
        spdlog::info("bmm150 init ok");
    }
}

static void ir_init()
{
    spdlog::info("ir init");
    ir_nec_transceiver_init(HAL_PIN_IR_TX);
}

static void camera_init()
{
    spdlog::info("camera init");
    esp_err_t ret = my_camera_init(CONFIG_CAMERA_XCLK_FREQ, PIXFORMAT_RGB565, FRAMESIZE_QVGA, 14, 2);
    if (ret != ESP_OK) spdlog::error("camera init failed");
}

static void asset_pool_injection()
{
    char* static_asset;
    const esp_partition_t* part;
    spi_flash_mmap_handle_t handler;
    esp_err_t err;
    nvs_flash_init();
    part = esp_partition_find_first((esp_partition_type_t)233, (esp_partition_subtype_t)0x23, NULL);
    if (part == 0) {
        spdlog::error("asset pool partition not found!\n");
        return;
    }
    err = esp_partition_mmap(part, 0, 2 * 1024 * 1024, ESP_PARTITION_MMAP_DATA, (const void**)&static_asset, &handler);
    if (err != ESP_OK) {
        spdlog::error("map asset pool failed!\n");
        return;
    }
    spdlog::info("asset pool maped at: {}", (void*)static_asset);

    AssetPool::InjectStaticAsset((StaticAsset_t*)static_asset);
}

static void shared_data_injection()
{
    SharedData::Inject(new SharedData_StdMutex);
    SharedData::SetServiceMode(ServiceMode::mode_none);
}

extern "C" int app_main(void)
{
    spdlog::set_pattern("[%H:%M:%S] [%L] %v");

    /* ----------------------------------- DI ----------------------------------- */
    shared_data_injection();
    asset_pool_injection();

    /* ------------------------------ Hardware init ----------------------------- */
    enable_camera_power();
    i2c_init();
    imu_init();
    ir_init();
    camera_init();

    /* ------------------------------ Start service ----------------------------- */
    start_service_uvc();
    start_service_web_server();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        cleanup_imu_ws_client();
    }
}
