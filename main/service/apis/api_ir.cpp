/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "apis.h"
#include <mooncake.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include "../../utils/shared/shared.h"

static void on_ir_send(AsyncWebServerRequest* request, JsonVariant& json)
{
    uint16_t addr = 0;
    uint16_t cmd  = 0;

    if (!json["addr"].isNull()) addr = json["addr"];
    if (!json["cmd"].isNull()) cmd = json["cmd"];

    spdlog::info("ir send in addr: {} cmd: {}", addr, cmd);
    SharedData::IrSendNecMsg(addr, cmd);

    request->send(200, "application/json", "{\"msg\":\"ok\"}");
}

void load_ir_apis(AsyncWebServer& server)
{
    AsyncCallbackJsonWebHandler* ir_send_handler = new AsyncCallbackJsonWebHandler("/api/v1/ir_send", on_ir_send);
    server.addHandler(ir_send_handler);
}
