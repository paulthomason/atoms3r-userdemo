/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "service.h"
#include <esp_log.h>
#include <mooncake.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../utils/assets/assets.h"
#include "../utils/shared/shared.h"
#include "apis/apis.h"
extern "C" {
#include "../utils/camera/camera_init.h"
}
// https://github.com/Aircoookie/ESPAsyncWebServer

static const char* TAG = "web_server";

static AsyncWebServer* web_server = nullptr;

void start_service_web_server()
{
    ESP_LOGI(TAG, "Start service web server");

    initArduino();

    // Create web server
    web_server = new AsyncWebServer(80);

    // Start ap
    String ap_ssid = "AtomS3R-CAM-WiFi";
    WiFi.softAP(ap_ssid, emptyString, 1, 0, 1, false);
    spdlog::info("ap ip: {}", WiFi.softAPIP().toString().c_str());

    // Load page
    web_server->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response =
            request->beginResponse_P(200, "text/html", AssetPool::GetImage().index_html_gz, 234419);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // Load apis
    load_cam_apis(*web_server);
    load_imu_apis(*web_server);
    load_ir_apis(*web_server);

    // Start web server
    spdlog::info("web server started");
    web_server->begin();

    ESP_LOGI(TAG, "Service web server started");
}
