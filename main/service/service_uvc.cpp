/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "service.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
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
extern "C" {
#include "../utils/camera/camera_init.h"
}
#include "../utils/shared/shared.h"

static const char* TAG = "usb_webcam";

#define CAMERA_XCLK_FREQ CONFIG_CAMERA_XCLK_FREQ
#define CAMERA_FB_COUNT  2

#if CONFIG_IDF_TARGET_ESP32S3
// #define UVC_MAX_FRAMESIZE_SIZE (75 * 1024)
#define UVC_MAX_FRAMESIZE_SIZE (90 * 1024)
#else
#define UVC_MAX_FRAMESIZE_SIZE (60 * 1024)
#endif

typedef struct {
    camera_fb_t* cam_fb_p;
    uvc_fb_t uvc_fb;
} fb_t;

static fb_t s_fb;

static void camera_stop_cb(void* cb_ctx)
{
    (void)cb_ctx;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#if CONFIG_CAMERA_MODULE_ESP_S3_EYE
    xEventGroupSetBits(s_event_group, EYES_CLOSE_BIT);
#endif
#endif
    ESP_LOGI(TAG, "Camera Stop");
}

static esp_err_t camera_start_cb(uvc_format_t format, int width, int height, int rate, void* cb_ctx)
{
    SharedData::BorrowData();

    (void)cb_ctx;
    ESP_LOGI(TAG, "Camera Start");
    ESP_LOGI(TAG, "Format: %d, width: %d, height: %d, rate: %d", format, width, height, rate);
    framesize_t frame_size = FRAMESIZE_QVGA;
    int jpeg_quality       = 14;

    if (format != UVC_FORMAT_JPEG) {
        ESP_LOGE(TAG, "Only support MJPEG format");
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (width == 320 && height == 240) {
        frame_size   = FRAMESIZE_QVGA;
        jpeg_quality = 10;
    } else if (width == 480 && height == 320) {
        frame_size   = FRAMESIZE_HVGA;
        jpeg_quality = 10;
    } else if (width == 640 && height == 480) {
        frame_size   = FRAMESIZE_VGA;
        jpeg_quality = 12;
    } else if (width == 800 && height == 600) {
        frame_size   = FRAMESIZE_SVGA;
        jpeg_quality = 14;
    } else if (width == 1280 && height == 720) {
        frame_size   = FRAMESIZE_HD;
        jpeg_quality = 16;
    } else if (width == 1920 && height == 1080) {
        frame_size   = FRAMESIZE_FHD;
        jpeg_quality = 16;
    } else {
        ESP_LOGE(TAG, "Unsupported frame size %dx%d", width, height);
        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_err_t ret = my_camera_init(CAMERA_XCLK_FREQ, PIXFORMAT_RGB565, frame_size, jpeg_quality, CAMERA_FB_COUNT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "camera init failed");
        SharedData::ReturnData();
        return ret;
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#if CONFIG_CAMERA_MODULE_ESP_S3_EYE
    xEventGroupSetBits(s_event_group, EYES_OPEN_BIT);
#endif
#endif

    SharedData::ReturnData();
    return ESP_OK;
}

static uvc_fb_t* camera_fb_get_cb(void* cb_ctx)
{
    if (SharedData::GetServiceMode() == ServiceMode::mode_web_server) {
        ESP_LOGW(TAG, "web server is streaming");
        return NULL;
    }
    SharedData::BorrowData();

    // ESP_LOGI(TAG, "start capture");
    (void)cb_ctx;
    s_fb.cam_fb_p = esp_camera_fb_get();
    if (!s_fb.cam_fb_p) {
        ESP_LOGE(TAG, "Capture failed");
        return NULL;
    }
    // ESP_LOGI(TAG, "Capture ok");

    // ESP_LOGI(TAG, "Free heap: %ld", esp_get_free_heap_size());
    if (s_fb.uvc_fb.buf != NULL) free(s_fb.uvc_fb.buf);

    if (!frame2jpg(s_fb.cam_fb_p, 60, &s_fb.uvc_fb.buf, &s_fb.uvc_fb.len)) {
        ESP_LOGE(TAG, "Convert to jpg failed");
        return NULL;
    }
    s_fb.uvc_fb.width     = s_fb.cam_fb_p->width;
    s_fb.uvc_fb.height    = s_fb.cam_fb_p->height;
    s_fb.uvc_fb.format    = UVC_FORMAT_JPEG;
    s_fb.uvc_fb.timestamp = s_fb.cam_fb_p->timestamp;

    if (s_fb.uvc_fb.len > UVC_MAX_FRAMESIZE_SIZE) {
        ESP_LOGE(TAG, "Frame size %d is larger than max frame size %d", s_fb.uvc_fb.len, UVC_MAX_FRAMESIZE_SIZE);
        esp_camera_fb_return(s_fb.cam_fb_p);
        return NULL;
    }

    SharedData::ReturnData();

    return &s_fb.uvc_fb;
}

static void camera_fb_return_cb(uvc_fb_t* fb, void* cb_ctx)
{
    (void)cb_ctx;
    assert(fb == &s_fb.uvc_fb);
    esp_camera_fb_return(s_fb.cam_fb_p);
}

void start_service_uvc()
{
    ESP_LOGI(TAG, "Start service uvc");

    s_fb.uvc_fb.buf = NULL;

    // if using esp-s3-eye board, show the GUI
#if CONFIG_CAMERA_MODULE_ESP_S3_EYE
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    bsp_display_start();
    bsp_display_backlight_on();
    lv_obj_t* img = eyes_init();
    s_event_group = xEventGroupCreate();
    xEventGroupSetBits(s_event_group, EYES_CLOSE_BIT);
#else
    ESP_LOGW(TAG, "ESP-S3-EYE lcd animation not supported in ESP-IDF < v5.0");
#endif
#endif
    ESP_LOGI(TAG, "Selected Camera Board %s", CAMERA_MODULE_NAME);
    uint8_t* uvc_buffer = (uint8_t*)malloc(UVC_MAX_FRAMESIZE_SIZE);
    if (uvc_buffer == NULL) {
        ESP_LOGE(TAG, "malloc frame buffer fail");
        return;
    }

    uvc_device_config_t config;
    config.uvc_buffer      = uvc_buffer;
    config.uvc_buffer_size = UVC_MAX_FRAMESIZE_SIZE;
    config.start_cb        = camera_start_cb;
    config.fb_get_cb       = camera_fb_get_cb;
    config.fb_return_cb    = camera_fb_return_cb;
    config.stop_cb         = camera_stop_cb;

    ESP_LOGI(TAG, "Format List");
    ESP_LOGI(TAG, "\tFormat(1) = %s", "MJPEG");
    ESP_LOGI(TAG, "Frame List");
    ESP_LOGI(TAG, "\tFrame(1) = %d * %d @%dfps", UVC_FRAMES_INFO[0][0].width, UVC_FRAMES_INFO[0][0].height,
             UVC_FRAMES_INFO[0][0].rate);
#if CONFIG_CAMERA_MULTI_FRAMESIZE
    ESP_LOGI(TAG, "\tFrame(2) = %d * %d @%dfps", UVC_FRAMES_INFO[0][1].width, UVC_FRAMES_INFO[0][1].height,
             UVC_FRAMES_INFO[0][1].rate);
    ESP_LOGI(TAG, "\tFrame(3) = %d * %d @%dfps", UVC_FRAMES_INFO[0][2].width, UVC_FRAMES_INFO[0][2].height,
             UVC_FRAMES_INFO[0][2].rate);
    ESP_LOGI(TAG, "\tFrame(3) = %d * %d @%dfps", UVC_FRAMES_INFO[0][3].width, UVC_FRAMES_INFO[0][3].height,
             UVC_FRAMES_INFO[0][3].rate);
#endif

    ESP_ERROR_CHECK(uvc_device_config(0, &config));
    ESP_ERROR_CHECK(uvc_device_init());

    ESP_LOGI(TAG, "Service uvc started");
}
