/**
 * @file shared.cpp
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2024-05-11
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "shared.h"
#include <mooncake.h>
extern "C" {
#include "../ir_nec_transceiver/ir_nec_transceiver.h"
}

SharedData* SharedData::_shared_data = nullptr;

SharedData* SharedData::Get()
{
    return _shared_data;
}

bool SharedData::Check()
{
    return _shared_data != nullptr;
}

bool SharedData::Inject(SharedData* sharedData)
{
    if (_shared_data != nullptr) {
        spdlog::error("SharedData already exist");
        return false;
    }

    if (sharedData == nullptr) {
        spdlog::error("invalid SharedData ptr");
        return false;
    }

    _shared_data = sharedData;

    spdlog::info("SharedData injected, type: {}", sharedData->type());

    return true;
}

void SharedData::Destroy()
{
    if (_shared_data == nullptr) {
        spdlog::error("SharedData not exist");
        return;
    }

    delete _shared_data;
    _shared_data = nullptr;
}

void SharedData::UpdateImuData()
{
    if (GetData().imu == nullptr) {
        spdlog::info("bmi270 not available");
        return;
    }

    GetData().imu->readAcceleration(GetData().imu_data.accelY, GetData().imu_data.accelX, GetData().imu_data.accelZ);
    GetData().imu->readGyroscope(GetData().imu_data.gyroX, GetData().imu_data.gyroY, GetData().imu_data.gyroZ);

    if (!GetData().is_bmm150_ok) {
        spdlog::info("bmm150 not available");
        return;
    }

    GetData().imu->readMagneticField(GetData().imu_data.magX, GetData().imu_data.magY, GetData().imu_data.magZ);
    // Reverse
    GetData().imu_data.magX = -GetData().imu_data.magX;
    GetData().imu_data.magZ = -GetData().imu_data.magZ;
}

void SharedData::IrSendNecMsg(uint16_t addr, uint16_t command)
{
    ir_nec_transceiver_send(addr, command);
}
