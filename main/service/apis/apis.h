/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <ESPAsyncWebServer.h>

void load_cam_apis(AsyncWebServer& server);
void load_imu_apis(AsyncWebServer& server);
void cleanup_imu_ws_client();
void load_ir_apis(AsyncWebServer& server);
