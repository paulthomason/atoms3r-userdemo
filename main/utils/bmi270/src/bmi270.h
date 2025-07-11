/*
* SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
*
* SPDX-License-Identifier: MIT
*/
/*
  This file is part of the Arduino_LSM9DS1 library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "utilities/BMI270-Sensor-API/bmi270.h"
#include "utilities/BMM150-Sensor-API/bmm150.h"
#include <driver/i2c.h>
#include <iostream>
#include <string>
#include <M5Unified.h>

class BMI270_Class : public m5::I2C_Device
{
private:
    struct dev_info
    {
        uint8_t dev_addr;
    };

    int _interrupts = 0;
    struct dev_info accel_gyro_dev_info;
    struct dev_info mag_dev_info;
    struct bmi2_dev bmi2;
    struct bmm150_dev bmm1;
    uint16_t _int_status;

    bool continuousMode;

    void print_rslt(int8_t rslt);

protected:
    // can be modified by subclassing for finer configuration
    virtual int8_t configure_sensor(struct bmm150_dev* dev);
    virtual int8_t configure_sensor(struct bmi2_dev* dev);

public:
    BMI270_Class(std::uint8_t i2c_addr = BMI2_I2C_PRIM_ADDR, std::uint32_t freq = 400000, m5::I2C_Class* i2c = &m5::In_I2C)
        : I2C_Device(i2c_addr, freq, i2c)
    {
    }
    virtual ~BMI270_Class() = default;

    bool init();
    bool initAuxBmm150();

    void setContinuousMode();
    void oneShotMode();

    // Accelerometer
    int readAcceleration(float& x, float& y, float& z); // Results are in G (earth gravity).
    int accelerationAvailable();                        // Number of samples in the FIFO.
    float accelerationSampleRate();                     // Sampling rate of the sensor.

    // Gyroscope
    int readGyroscope(float& x, float& y, float& z); // Results are in degrees/second.
    int gyroscopeAvailable();                        // Number of samples in the FIFO.
    float gyroscopeSampleRate();                     // Sampling rate of the sensor.

    // Magnetometer
    int readMagneticField(float& x, float& y, float& z); // Results are in uT (micro Tesla).
    int magneticFieldAvailable();                        // Number of samples in the FIFO.
    float magneticFieldSampleRate();                     // Sampling rate of the sensor.

    // float getTemperature();

    void setWristWearWakeup();
    bool enableAnyMotionInterrupt();
    bool enableGyroInterrupt();

    int getGesture();
    std::string getGestureName(uint8_t gesture);

    void enableStepCounter();
    uint32_t getSteps();

    /* -------------------------------------------------------------------------- */
    /*                                    Port                                    */
    /* -------------------------------------------------------------------------- */
public:
    static int8_t bmi2_i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t len, void* intf_ptr);
    static int8_t bmi2_i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t len, void* intf_ptr);
    static void bmi2_delay_us(uint32_t period, void* intf_ptr);

    static int8_t aux_i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr);
    static int8_t aux_i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length, void* intf_ptr);
};
