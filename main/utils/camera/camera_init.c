/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "camera_pin.h"
#include "esp_camera.h"
#include "usb_device_uvc.h"
#include "uvc_frame_config.h"
#include "driver/gpio.h"
#if CONFIG_CAMERA_MODULE_ESP_S3_EYE
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "freertos/event_groups.h"
static EventGroupHandle_t s_event_group = NULL;
#define EYES_CLOSE_BIT BIT0
#define EYES_OPEN_BIT  BIT1
#else
#pragma message("ESP-S3-EYE lcd animation not supported in ESP-IDF < v5.0")
#endif
#endif

static const char* TAG = "cam_init";

esp_err_t my_camera_init(int xclk_freq_hz, pixformat_t pixel_format, framesize_t frame_size, int jpeg_quality,
                         uint8_t fb_count)
{
    static bool inited                  = false;
    static uint32_t cur_xclk_freq_hz    = 0;
    static pixformat_t cur_pixel_format = PIXFORMAT_RGB565;
    static framesize_t cur_frame_size   = FRAMESIZE_QVGA;
    static int cur_jpeg_quality         = 0;
    static uint8_t cur_fb_count         = 0;

    if ((inited && cur_xclk_freq_hz == xclk_freq_hz && cur_pixel_format == pixel_format &&
         cur_frame_size == frame_size && cur_fb_count == fb_count && cur_jpeg_quality == jpeg_quality)) {
        ESP_LOGD(TAG, "camera already inited");
        return ESP_OK;
    } else if (inited) {
        esp_camera_return_all();
        esp_camera_deinit();
        inited = false;
        ESP_LOGI(TAG, "camera RESTART");
    }

    camera_config_t camera_config = {.pin_pwdn     = CAMERA_PIN_PWDN,
                                     .pin_reset    = CAMERA_PIN_RESET,
                                     .pin_xclk     = CAMERA_PIN_XCLK,
                                     .pin_sscb_sda = CAMERA_PIN_SIOD,
                                     .pin_sscb_scl = CAMERA_PIN_SIOC,

                                     .pin_d7    = CAMERA_PIN_D7,
                                     .pin_d6    = CAMERA_PIN_D6,
                                     .pin_d5    = CAMERA_PIN_D5,
                                     .pin_d4    = CAMERA_PIN_D4,
                                     .pin_d3    = CAMERA_PIN_D3,
                                     .pin_d2    = CAMERA_PIN_D2,
                                     .pin_d1    = CAMERA_PIN_D1,
                                     .pin_d0    = CAMERA_PIN_D0,
                                     .pin_vsync = CAMERA_PIN_VSYNC,
                                     .pin_href  = CAMERA_PIN_HREF,
                                     .pin_pclk  = CAMERA_PIN_PCLK,

                                     .xclk_freq_hz = xclk_freq_hz,
                                     .ledc_timer   = LEDC_TIMER_0,
                                     .ledc_channel = LEDC_CHANNEL_0,

                                     .pixel_format = pixel_format,
                                     .frame_size   = frame_size,

                                     .jpeg_quality = jpeg_quality,
                                     .fb_count     = fb_count,
                                     .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
                                     .fb_location  = CAMERA_FB_IN_PSRAM};

    // initialize the camera sensor
    esp_err_t ret = esp_camera_init(&camera_config);
    if (ret != ESP_OK) {
        return ret;
    }

    // Get the sensor object, and then use some of its functions to adjust the parameters when taking a photo.
    // Note: Do not call functions that set resolution, set picture format and PLL clock,
    // If you need to reset the appeal parameters, please reinitialize the sensor.
    sensor_t* s = esp_camera_sensor_get();
    s->set_vflip(s, 1);  // flip it back
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID) {
        s->set_brightness(s, 1);   // up the blightness just a bit
        s->set_saturation(s, -2);  // lower the saturation
    }

    if (s->id.PID == OV3660_PID || s->id.PID == OV2640_PID) {
        s->set_vflip(s, 1);  // flip it back
    } else if (s->id.PID == GC0308_PID) {
        s->set_hmirror(s, 0);
    } else if (s->id.PID == GC032A_PID) {
        s->set_vflip(s, 1);
    }

    cur_xclk_freq_hz = xclk_freq_hz;
    cur_pixel_format = pixel_format;
    cur_frame_size   = frame_size;
    cur_jpeg_quality = jpeg_quality;
    cur_fb_count     = fb_count;
    inited           = true;

    return ret;
}
