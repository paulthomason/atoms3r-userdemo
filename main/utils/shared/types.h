/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include "../bmi270/src/bmi270.h"

/* -------------------------------------------------------------------------- */
/*                                    Misc                                    */
/* -------------------------------------------------------------------------- */
#define APP_VERSION "V0.1"

/* -------------------------------------------------------------------------- */
/*                                Service mode                                */
/* -------------------------------------------------------------------------- */
namespace ServiceMode {
enum ServiceMode_t {
    mode_none = 0,
    mode_uvc,
    mode_web_server,
};
};

/* -------------------------------------------------------------------------- */
/*                                     IMU                                    */
/* -------------------------------------------------------------------------- */
namespace IMU {
struct ImuData_t {
    float accelX;
    float accelY;
    float accelZ;

    float gyroX;
    float gyroY;
    float gyroZ;

    float magX;
    float magY;
    float magZ;
};
}  // namespace IMU

namespace SHARED_DATA {
struct SharedData_t {
    // Add your shared data here
    // int shitNum = 114514;
    // ...

    ServiceMode::ServiceMode_t service_mode = ServiceMode::mode_none;

    // IMU
    BMI270_Class* imu = nullptr;
    bool is_bmm150_ok = false;
    IMU::ImuData_t imu_data;
};
}  // namespace SHARED_DATA
