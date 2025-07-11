/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "apis.h"
#include <mooncake.h>
#include <Arduino.h>
#include <esp_camera.h>
#include <ESPAsyncWebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ArduinoJson.h>
#include "../../utils/shared/shared.h"

static AsyncWebSocket* imu_data_ws = nullptr;
static std::mutex mutex;
static bool is_imu_data_ws_connected = false;
static bool imu_data_ws_is_connected()
{
    static bool ret = false;
    mutex.lock();
    ret = is_imu_data_ws_connected;
    mutex.unlock();
    return ret;
}

static void imu_data_ws_daemon(void* param)
{
    spdlog::info("start imu data ws daemon");

    JsonDocument doc;
    std::string json_buffer;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (imu_data_ws_is_connected()) {
            SharedData::UpdateImuData();

            // Encode json
            doc["ax"] = SharedData::GetImuData().accelX;
            doc["ay"] = SharedData::GetImuData().accelY;
            doc["az"] = SharedData::GetImuData().accelZ;
            doc["gx"] = SharedData::GetImuData().gyroX;
            doc["gy"] = SharedData::GetImuData().gyroY;
            doc["gz"] = SharedData::GetImuData().gyroZ;
            doc["mx"] = SharedData::GetImuData().magX;
            doc["my"] = SharedData::GetImuData().magY;
            doc["mz"] = SharedData::GetImuData().magZ;
            serializeJson(doc, json_buffer);
            // printf("%s\n", json_buffer.c_str());

            imu_data_ws->textAll(json_buffer.c_str());
        }
    }

    vTaskDelete(NULL);
}

static void imu_ws_on_event(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg,
                            uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        // client connected
        spdlog::info("[imu data ws] client {} connected", client->id());
        client->ping();

        mutex.lock();
        is_imu_data_ws_connected = true;
        mutex.unlock();
    } else if (type == WS_EVT_DISCONNECT) {
        // client disconnected
        spdlog::info("[imu data ws] client {} disconnected", client->id());

        mutex.lock();
        is_imu_data_ws_connected = false;
        mutex.unlock();
    }
}

void load_imu_apis(AsyncWebServer& server)
{
    imu_data_ws = new AsyncWebSocket("/api/v1/ws/imu_data");
    imu_data_ws->onEvent(imu_ws_on_event);
    server.addHandler(imu_data_ws);

    xTaskCreate(imu_data_ws_daemon, "imuws", 4000, NULL, 5, NULL);
}

void cleanup_imu_ws_client()
{
    imu_data_ws->cleanupClients();
}
