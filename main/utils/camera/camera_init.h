/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <esp_camera.h>
#include "camera_pin.h"

esp_err_t my_camera_init(int xclk_freq_hz, pixformat_t pixel_format, framesize_t frame_size, int jpeg_quality,
                         uint8_t fb_count);
