/*
* SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
*
* SPDX-License-Identifier: MIT
*/
#include "bmi270.h"
#include <esp_timer.h>
#include <esp_log.h>
#include <cstring>

#define NOP() asm volatile("nop")
#define TAG "IMU"
#define bmi2_error_codes_print_result(rslt) print_rslt(rslt)

static bmi2_dev* _bmi2_dev_ptr = nullptr;
static BMI270_Class* _bmi270_class_instance = nullptr;

bool BMI270_Class::init()
{
    _bmi270_class_instance = this;

    /* BMI270 init */
    bmi2.chip_id = BMI2_I2C_PRIM_ADDR;
    bmi2.read = bmi2_i2c_read;
    bmi2.write = bmi2_i2c_write;
    bmi2.delay_us = bmi2_delay_us;
    bmi2.intf = BMI2_I2C_INTF;
    bmi2.intf_ptr = &accel_gyro_dev_info;
    bmi2.read_write_len = 30;    // Limitation of the Wire library
    bmi2.config_file_ptr = NULL; // Use the default BMI270 config file

    accel_gyro_dev_info.dev_addr = bmi2.chip_id;

    int8_t rslt = bmi270_init(&bmi2);
    print_rslt(rslt);
    if (rslt != BMI2_OK)
        return false;

    rslt = configure_sensor(&bmi2);
    print_rslt(rslt);
    if (rslt != BMI2_OK)
        return false;

    _bmi2_dev_ptr = &bmi2;
    return true;
}

bool BMI270_Class::initAuxBmm150()
{
    /* BMI270 Aux init */
    int8_t rslt;

    /* Pull-up resistor 2k is set to the trim regiter */
    uint8_t regdata = BMI2_ASDA_PUPSEL_2K;
    rslt = bmi2_set_regs(BMI2_AUX_IF_TRIM, &regdata, 1, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    bmi2_sens_config config;
    config.type = BMI2_AUX;

    /* Get default configurations for the type of feature selected. */
    rslt = bmi270_get_sensor_config(&config, 1, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    /* Rewrite */
    config.cfg.aux.odr = BMI2_AUX_ODR_100HZ;
    config.cfg.aux.aux_en = BMI2_ENABLE;
    config.cfg.aux.i2c_device_addr = BMM150_DEFAULT_I2C_ADDRESS;
    config.cfg.aux.fcu_write_en = BMI2_ENABLE;
    config.cfg.aux.man_rd_burst = BMI2_AUX_READ_LEN_3;
    config.cfg.aux.read_addr = BMM150_REG_DATA_X_LSB;
    config.cfg.aux.manual_en = BMI2_ENABLE;
    // config.cfg.aux.manual_en = BMI2_DISABLE;

    /* Set back */
    rslt = bmi270_set_sensor_config(&config, 1, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    _bmi2_dev_ptr = &bmi2;

    /* BMM150 init */
    bmm1.chip_id = BMM150_DEFAULT_I2C_ADDRESS;
    bmm1.read = aux_i2c_read;
    bmm1.write = aux_i2c_write;
    bmm1.delay_us = bmi2_delay_us;
    bmm1.intf = BMM150_I2C_INTF;
    bmm1.intf_ptr = &mag_dev_info;

    mag_dev_info.dev_addr = bmm1.chip_id;

    rslt = bmm150_init(&bmm1);
    print_rslt(rslt);
    if (rslt != BMI2_OK)
        return false;

    rslt = configure_sensor(&bmm1);
    print_rslt(rslt);
    if (rslt != BMI2_OK)
        return false;

    // /* Map data ready interrupt to interrupt pin. */
    // rslt = bmi2_map_data_int(BMI2_DRDY_INT, BMI2_INT1, &bmi2);
    // bmi2_error_codes_print_result(rslt);

    // printf("aux get chip id: 0x%X\n", bmm1.chip_id);
    if (bmm1.chip_id != BMM150_CHIP_ID)
        return false;
    return true;
}

void BMI270_Class::setContinuousMode()
{
    bmi2_set_fifo_config(BMI2_FIFO_GYR_EN | BMI2_FIFO_ACC_EN, 1, &bmi2);
    continuousMode = true;
}

void BMI270_Class::oneShotMode()
{
    bmi2_set_fifo_config(BMI2_FIFO_GYR_EN | BMI2_FIFO_ACC_EN, 0, &bmi2);
    continuousMode = false;
}

// default range is +-4G, so conversion factor is (((1 << 15)/4.0f))
#define INT16_to_G (8192.0f)

// Accelerometer
int BMI270_Class::readAcceleration(float& x, float& y, float& z)
{
    struct bmi2_sens_data sensor_data;
    auto ret = bmi2_get_sensor_data(&sensor_data, &bmi2);

    x = sensor_data.acc.x / INT16_to_G;
    y = sensor_data.acc.y / INT16_to_G;
    z = sensor_data.acc.z / INT16_to_G;
    return (ret == 0);
}

int BMI270_Class::accelerationAvailable()
{
    uint16_t status;
    bmi2_get_int_status(&status, &bmi2);
    int ret = ((status | _int_status) & BMI2_ACC_DRDY_INT_MASK);
    _int_status = status;
    _int_status &= ~BMI2_ACC_DRDY_INT_MASK;
    return ret;
}

float BMI270_Class::accelerationSampleRate()
{
    struct bmi2_sens_config sens_cfg;
    sens_cfg.type = BMI2_ACCEL;
    bmi2_get_sensor_config(&sens_cfg, 1, &bmi2);
    return (1 << sens_cfg.cfg.acc.odr) * 0.39;
}

// default range is +-2000dps, so conversion factor is (((1 << 15)/4.0f))
#define INT16_to_DPS (16.384f)

// Gyroscope
int BMI270_Class::readGyroscope(float& x, float& y, float& z)
{
    struct bmi2_sens_data sensor_data;
    auto ret = bmi2_get_sensor_data(&sensor_data, &bmi2);

    x = sensor_data.gyr.x / INT16_to_DPS;
    y = sensor_data.gyr.y / INT16_to_DPS;
    z = sensor_data.gyr.z / INT16_to_DPS;
    return (ret == 0);
}

int BMI270_Class::gyroscopeAvailable()
{
    uint16_t status;
    bmi2_get_int_status(&status, &bmi2);
    int ret = ((status | _int_status) & BMI2_GYR_DRDY_INT_MASK);
    _int_status = status;
    _int_status &= ~BMI2_GYR_DRDY_INT_MASK;
    return ret;
}

float BMI270_Class::gyroscopeSampleRate()
{
    struct bmi2_sens_config sens_cfg;
    sens_cfg.type = BMI2_GYRO;
    bmi2_get_sensor_config(&sens_cfg, 1, &bmi2);
    return (1 << sens_cfg.cfg.gyr.odr) * 0.39;
}

// Magnetometer
int BMI270_Class::readMagneticField(float& x, float& y, float& z)
{
    struct bmm150_mag_data mag_data;
    int const rc = bmm150_read_mag_data(&mag_data, &bmm1);
    x = mag_data.x;
    y = mag_data.y;
    z = mag_data.z;

    if (rc == BMM150_OK)
        return 1;
    else
        return 0;
}

int BMI270_Class::magneticFieldAvailable()
{
    bmm150_get_interrupt_status(&bmm1);
    return bmm1.int_status & BMM150_INT_ASSERTED_DRDY;
}

float BMI270_Class::magneticFieldSampleRate()
{
    struct bmm150_settings settings;
    bmm150_get_sensor_settings(&settings, &bmm1);
    switch (settings.data_rate)
    {
    case BMM150_DATA_RATE_10HZ:
        return 10;
    case BMM150_DATA_RATE_02HZ:
        return 2;
    case BMM150_DATA_RATE_06HZ:
        return 6;
    case BMM150_DATA_RATE_08HZ:
        return 8;
    case BMM150_DATA_RATE_15HZ:
        return 15;
    case BMM150_DATA_RATE_20HZ:
        return 20;
    case BMM150_DATA_RATE_25HZ:
        return 25;
    case BMM150_DATA_RATE_30HZ:
        return 30;
    }
    return 0;
}

int8_t BMI270_Class::configure_sensor(struct bmi2_dev* dev)
{
    int8_t rslt;
    uint8_t sens_list[2] = {BMI2_ACCEL, BMI2_GYRO};

    // struct bmi2_int_pin_config int_pin_cfg;
    // int_pin_cfg.pin_type = BMI2_INT1;
    // int_pin_cfg.int_latch = BMI2_INT_NON_LATCH;
    // int_pin_cfg.pin_cfg[0].lvl = BMI2_INT_ACTIVE_HIGH;
    // int_pin_cfg.pin_cfg[0].od = BMI2_INT_PUSH_PULL;
    // int_pin_cfg.pin_cfg[0].output_en = BMI2_INT_OUTPUT_ENABLE;
    // int_pin_cfg.pin_cfg[0].input_en = BMI2_INT_INPUT_DISABLE;

    struct bmi2_sens_config sens_cfg[2];
    sens_cfg[0].type = BMI2_ACCEL;
    sens_cfg[0].cfg.acc.bwp = BMI2_ACC_OSR2_AVG2;
    sens_cfg[0].cfg.acc.odr = BMI2_ACC_ODR_50HZ;
    sens_cfg[0].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;
    sens_cfg[0].cfg.acc.range = BMI2_ACC_RANGE_4G;
    sens_cfg[1].type = BMI2_GYRO;
    sens_cfg[1].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;
    sens_cfg[1].cfg.gyr.bwp = BMI2_GYR_OSR2_MODE;
    sens_cfg[1].cfg.gyr.odr = BMI2_ACC_ODR_50HZ;
    sens_cfg[1].cfg.gyr.range = BMI2_GYR_RANGE_2000;
    sens_cfg[1].cfg.gyr.ois_range = BMI2_GYR_OIS_2000;

    // rslt = bmi2_set_int_pin_config(&int_pin_cfg, dev);
    // if (rslt != BMI2_OK)
    //     return rslt;

    // rslt = bmi2_map_data_int(BMI2_DRDY_INT, BMI2_INT1, dev);
    // if (rslt != BMI2_OK)
    //     return rslt;

    rslt = bmi2_set_sensor_config(sens_cfg, 2, dev);
    if (rslt != BMI2_OK)
        return rslt;

    rslt = bmi2_sensor_enable(sens_list, 2, dev);
    if (rslt != BMI2_OK)
        return rslt;

    return rslt;
}

int8_t BMI270_Class::configure_sensor(struct bmm150_dev* dev)
{
    /* Status of api are returned to this variable. */
    int8_t rslt;
    struct bmm150_settings settings;

    /* Set powermode as normal mode */
    settings.pwr_mode = BMM150_POWERMODE_NORMAL;
    rslt = bmm150_set_op_mode(&settings, dev);

    if (rslt == BMM150_OK)
    {
        /* Setting the preset mode as Low power mode
         * i.e. data rate = 10Hz, XY-rep = 1, Z-rep = 2
         */
        settings.preset_mode = BMM150_PRESETMODE_REGULAR;
        rslt = bmm150_set_presetmode(&settings, dev);

        if (rslt == BMM150_OK)
        {
            /* Map the data interrupt pin */
            settings.int_settings.drdy_pin_en = 0x01;
            rslt = bmm150_set_sensor_settings(BMM150_SEL_DRDY_PIN_EN, &settings, dev);
        }
    }
    return rslt;
}

void BMI270_Class::setWristWearWakeup()
{
    /* Variable to define result */
    int8_t rslt;

    /* List the sensors which are required to enable */
    // uint8_t sens_list[2] = { BMI2_ACCEL, BMI2_WRIST_GESTURE };
    uint8_t sens_list[2] = {BMI2_ACCEL, BMI2_WRIST_WEAR_WAKE_UP};

    /* Structure to define the type of the sensor and its configurations */
    struct bmi2_sens_config config;

    /* Interrupt pin configuration */
    // struct bmi2_int_pin_config pin_config = { 0 };
    struct bmi2_int_pin_config pin_config;
    memset((void*)&pin_config, 0, sizeof(bmi2_int_pin_config));

    /* Configure type of feature */
    // config.type = BMI2_WRIST_GESTURE;
    config.type = BMI2_WRIST_WEAR_WAKE_UP;

    /* Enable the selected sensors */
    rslt = bmi270_sensor_enable(sens_list, 2, &bmi2);
    bmi2_error_codes_print_result(rslt);

    rslt = bmi2_get_int_pin_config(&pin_config, &bmi2);
    bmi2_error_codes_print_result(rslt);

    if (rslt == BMI2_OK)
    {
        /* Get default configurations for the type of feature selected */
        rslt = bmi270_get_sensor_config(&config, 1, &bmi2);
        bmi2_error_codes_print_result(rslt);

        if (rslt == BMI2_OK)
        {
            config.cfg.wrist_gest.wearable_arm = BMI2_ARM_RIGHT;

            /* Set the new configuration along with interrupt mapping */
            rslt = bmi270_set_sensor_config(&config, 1, &bmi2);
            bmi2_error_codes_print_result(rslt);

            /* Interrupt pin configuration */
            // pin_config.pin_type = BMI2_INT1;
            pin_config.pin_type = BMI2_INT_BOTH;
            // pin_config.pin_cfg[1].input_en = BMI2_INT_INPUT_DISABLE;
            // pin_config.pin_cfg[1].lvl = BMI2_INT_ACTIVE_LOW;
            // pin_config.pin_cfg[1].od = BMI2_INT_PUSH_PULL;
            // // pin_config.pin_cfg[1].od = BMI2_INT_OPEN_DRAIN;
            // pin_config.pin_cfg[1].output_en = BMI2_INT_OUTPUT_ENABLE;

            pin_config.pin_cfg[0].input_en = BMI2_INT_INPUT_DISABLE;
            pin_config.pin_cfg[0].lvl = BMI2_INT_ACTIVE_LOW;
            pin_config.pin_cfg[0].od = BMI2_INT_PUSH_PULL;
            // pin_config.pin_cfg[0].od = BMI2_INT_OPEN_DRAIN;
            pin_config.pin_cfg[0].output_en = BMI2_INT_OUTPUT_ENABLE;

            pin_config.int_latch = BMI2_INT_NON_LATCH;
            // pin_config.int_latch = BMI2_INT_LATCH;

            rslt = bmi2_set_int_pin_config(&pin_config, &bmi2);
            bmi2_error_codes_print_result(rslt);
        }
    }

    if (rslt == BMI2_OK)
    {
        /* Map the feature interrupt */
        struct bmi2_sens_int_config sens_int;
        // sens_int.type = BMI2_WRIST_GESTURE;
        sens_int.type = BMI2_WRIST_WEAR_WAKE_UP;
        sens_int.hw_int_pin = BMI2_INT1;
        rslt = bmi270_map_feat_int(&sens_int, 1, &bmi2);
        bmi2_error_codes_print_result(rslt);
    }
}

bool BMI270_Class::enableAnyMotionInterrupt()
{
    /* Status of api are returned to this variable. */
    int8_t rslt;

    /* Accel sensor and any-motion feature are listed in array. */
    uint8_t sens_list[2] = {BMI2_ACCEL, BMI2_ANY_MOTION};
    /* Enable the selected sensors. */
    rslt = bmi270_sensor_enable(sens_list, 2, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    /* Structure to define the type of sensor and its configurations. */
    struct bmi2_sens_config config;

    /* Interrupt pin configuration */
    struct bmi2_int_pin_config pin_config = {0};

    /* Configure the type of feature. */
    config.type = BMI2_ANY_MOTION;

    /* Get default configurations for the type of feature selected. */
    rslt = bmi270_get_sensor_config(&config, 1, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    rslt = bmi2_get_int_pin_config(&pin_config, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    /* NOTE: The user can change the following configuration parameters according to their requirement. */
    /* 1LSB equals 20ms. Default is 100ms, setting to 80ms. */
    config.cfg.any_motion.duration = 0x04;
    // config.cfg.any_motion.duration = 0x08;

    /* 1LSB equals to 0.48mg. Default is 83mg, setting to 50mg. */
    config.cfg.any_motion.threshold = 0x68;

    /* Set new configurations. */
    rslt = bmi270_set_sensor_config(&config, 1, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    /* Interrupt pin configuration */
    pin_config.pin_type = BMI2_INT1;
    pin_config.pin_cfg[0].input_en = BMI2_INT_INPUT_DISABLE;
    pin_config.pin_cfg[0].lvl = BMI2_INT_ACTIVE_LOW;
    pin_config.pin_cfg[0].od = BMI2_INT_PUSH_PULL;
    pin_config.pin_cfg[0].output_en = BMI2_INT_OUTPUT_ENABLE;
    pin_config.int_latch = BMI2_INT_NON_LATCH;

    rslt = bmi2_set_int_pin_config(&pin_config, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    /* Map the feature interrupt for no-motion. */
    /* Select features and their pins to be mapped to. */
    struct bmi2_sens_int_config sens_int = {.type = BMI2_ANY_MOTION, .hw_int_pin = BMI2_INT1};
    rslt = bmi270_map_feat_int(&sens_int, 1, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    return true;
}

bool BMI270_Class::enableGyroInterrupt()
{
    /* Status of api are returned to this variable. */
    int8_t rslt;

    /* Structure to define the type of sensor and its configurations. */
    struct bmi2_sens_config config;

    /* Interrupt pin configuration */
    struct bmi2_int_pin_config pin_config = {0};

    /* Configure the type of feature. */
    config.type = BMI2_GYRO;

    /* Get default configuration for hardware Interrupt */
    rslt = bmi2_get_int_pin_config(&pin_config, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    /* Get default configurations for the type of feature selected. */
    rslt = bmi2_get_sensor_config(&config, 1, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    /* Map data ready interrupt to interrupt pin. */
    rslt = bmi2_map_data_int(BMI2_DRDY_INT, BMI2_INT1, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    /* The user can change the following configuration parameters according to their requirement. */
    /* Set Output Data Rate */
    config.cfg.gyr.odr = BMI2_GYR_ODR_100HZ;

    /* Gyroscope Angular Rate Measurement Range.By default the range is 2000dps. */
    config.cfg.gyr.range = BMI2_GYR_RANGE_2000;

    /* Gyroscope bandwidth parameters. By default the gyro bandwidth is in normal mode. */
    config.cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;

    /* Enable/Disable the noise performance mode for precision yaw rate sensing
     * There are two modes
     *  0 -> Ultra low power mode(Default)
     *  1 -> High performance mode
     */
    config.cfg.gyr.noise_perf = BMI2_POWER_OPT_MODE;

    /* Enable/Disable the filter performance mode where averaging of samples
     * will be done based on above set bandwidth and ODR.
     * There are two modes
     *  0 -> Ultra low power mode
     *  1 -> High performance mode(Default)
     */
    config.cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;

    /* Interrupt pin configuration */
    pin_config.pin_type = BMI2_INT1;
    pin_config.pin_cfg[0].input_en = BMI2_INT_INPUT_DISABLE;
    pin_config.pin_cfg[0].lvl = BMI2_INT_ACTIVE_LOW;
    pin_config.pin_cfg[0].od = BMI2_INT_PUSH_PULL;
    pin_config.pin_cfg[0].output_en = BMI2_INT_OUTPUT_ENABLE;
    pin_config.int_latch = BMI2_INT_NON_LATCH;

    /* Set Hardware interrupt pin configuration */
    rslt = bmi2_set_int_pin_config(&pin_config, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    /* Set the gyro configurations. */
    rslt = bmi2_set_sensor_config(&config, 1, &bmi2);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK)
        return false;

    return true;
}

int BMI270_Class::getGesture()
{
    /* Variable to define result */
    int8_t rslt;
    /* Initialize status of wrist gesture interrupt */
    uint16_t int_status = 0;
    /* Sensor data structure */
    // struct bmi2_feat_sensor_data sens_data = {0};
    struct bmi2_feat_sensor_data sens_data;
    memset((void*)&sens_data, 0, sizeof(bmi2_feat_sensor_data));

    sens_data.type = BMI2_WRIST_GESTURE;

    /* Get the interrupt status of the wrist gesture */
    rslt = bmi2_get_int_status(&int_status, &bmi2);
    bmi2_error_codes_print_result(rslt);

    if ((rslt == BMI2_OK) && (int_status & BMI270_WRIST_GEST_STATUS_MASK))
    {
        /* Get wrist gesture output */
        rslt = bmi270_get_feature_data(&sens_data, 1, &bmi2);
        bmi2_error_codes_print_result(rslt);

        // printf("Wrist gesture = %d\r\n", sens_data.sens_data.wrist_gesture_output);
        return sens_data.sens_data.wrist_gesture_output;
    }
    return -1;
}

std::string BMI270_Class::getGestureName(uint8_t gesture)
{
    if (gesture > 5)
    {
        return "error";
    }

    const char* gesture_output[6] = {
        "unknown_gesture", "push_arm_down", "pivot_up", "wrist_shake_jiggle", "flick_in", "flick_out"};

    return gesture_output[gesture];
}

void BMI270_Class::enableStepCounter()
{
    /* Status of api are returned to this variable. */
    int8_t rslt;

    /* Accel sensor and step counter feature are listed in array. */
    uint8_t sensor_sel[2] = {BMI2_ACCEL, BMI2_STEP_COUNTER};

    /* Enable the selected sensor. */
    rslt = bmi270_sensor_enable(sensor_sel, 2, &bmi2);
    bmi2_error_codes_print_result(rslt);
}

uint32_t BMI270_Class::getSteps()
{
    /* Status of api are returned to this variable. */
    int8_t rslt;

    /* Structure to define type of sensor and their respective data. */
    struct bmi2_feat_sensor_data sensor_data;
    memset((void*)&sensor_data, 0, sizeof(bmi2_feat_sensor_data));

    /* Type of sensor to get step counter data. */
    sensor_data.type = BMI2_STEP_COUNTER;

    /* Get step counter output. */
    rslt = bmi270_get_feature_data(&sensor_data, 1, &bmi2);
    bmi2_error_codes_print_result(rslt);

    // /* Print the step counter output. */
    // printf("No of steps counted  = %lu\n",
    //         (long unsigned int)sensor_data.sens_data.step_counter_output);

    return sensor_data.sens_data.step_counter_output;
}

int8_t BMI270_Class::bmi2_i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t len, void* intf_ptr)
{
    if ((reg_data == NULL) || (len == 0) || (len > 32))
    {
        return -1;
    }

    // printf(">>>>> read: addr: 0x%02X size: %ld\n", reg_addr, len);

    if (_bmi270_class_instance->readRegister(reg_addr, reg_data, len))
        return 0;
    return -1;
}

int8_t BMI270_Class::bmi2_i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t len, void* intf_ptr)
{
    if ((reg_data == NULL) || (len == 0) || (len > 32))
    {
        return -1;
    }

    // printf(">>>>> send: addr: 0x%02X size: %ld data: ", reg_addr, len);
    // for (int i = 0; i < len; i++)
    // {
    //     printf("0x%02X ", reg_data[i]);
    // }
    // printf("\n");

    if (_bmi270_class_instance->writeRegister(reg_addr, reg_data, len))
        return 0;
    return -1;
}

void BMI270_Class::bmi2_delay_us(uint32_t period, void* intf_ptr)
{
    /* Copy from Arduino */
    uint64_t m = (uint64_t)esp_timer_get_time();
    if (period)
    {
        uint64_t e = (m + period);
        if (m > e)
        { // overflow
            while ((uint64_t)esp_timer_get_time() > e)
            {
                NOP();
            }
        }
        while ((uint64_t)esp_timer_get_time() < e)
        {
            NOP();
        }
    }
}

int8_t BMI270_Class::aux_i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr)
{
    return bmi2_read_aux_man_mode(reg_addr, reg_data, length, _bmi2_dev_ptr);
}

int8_t BMI270_Class::aux_i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length, void* intf_ptr)
{
    return bmi2_write_aux_man_mode(reg_addr, reg_data, length, _bmi2_dev_ptr);
}

void BMI270_Class::print_rslt(int8_t rslt)
{
    switch (rslt)
    {
    case BMI2_OK:
        return; /* Do nothing */
        break;
    case BMI2_E_NULL_PTR:
        ESP_LOGE(TAG, "Error [%d] : Null pointer", rslt);
        break;
    case BMI2_E_COM_FAIL:
        ESP_LOGE(TAG, "Error [%d] : Communication failure", rslt);
        break;
    case BMI2_E_DEV_NOT_FOUND:
        ESP_LOGE(TAG, "Error [%d] : Device not found", rslt);
        break;
    case BMI2_E_OUT_OF_RANGE:
        ESP_LOGE(TAG, "Error [%d] : Out of range", rslt);
        break;
    case BMI2_E_ACC_INVALID_CFG:
        ESP_LOGE(TAG, "Error [%d] : Invalid accel configuration", rslt);
        break;
    case BMI2_E_GYRO_INVALID_CFG:
        ESP_LOGE(TAG, "Error [%d] : Invalid gyro configuration", rslt);
        break;
    case BMI2_E_ACC_GYR_INVALID_CFG:
        ESP_LOGE(TAG, "Error [%d] : Invalid accel/gyro configuration", rslt);
        break;
    case BMI2_E_INVALID_SENSOR:
        ESP_LOGE(TAG, "Error [%d] : Invalid sensor", rslt);
        break;
    case BMI2_E_CONFIG_LOAD:
        ESP_LOGE(TAG, "Error [%d] : Configuration loading error", rslt);
        break;
    case BMI2_E_INVALID_PAGE:
        ESP_LOGE(TAG, "Error [%d] : Invalid page", rslt);
        break;
    case BMI2_E_INVALID_FEAT_BIT:
        ESP_LOGE(TAG, "Error [%d] : Invalid feature bit", rslt);
        break;
    case BMI2_E_INVALID_INT_PIN:
        ESP_LOGE(TAG, "Error [%d] : Invalid interrupt pin", rslt);
        break;
    case BMI2_E_SET_APS_FAIL:
        ESP_LOGE(TAG, "Error [%d] : Setting advanced power mode failed", rslt);
        break;
    case BMI2_E_AUX_INVALID_CFG:
        ESP_LOGE(TAG, "Error [%d] : Invalid auxiliary configuration", rslt);
        break;
    case BMI2_E_AUX_BUSY:
        ESP_LOGE(TAG, "Error [%d] : Auxiliary busy", rslt);
        break;
    case BMI2_E_SELF_TEST_FAIL:
        ESP_LOGE(TAG, "Error [%d] : Self test failed", rslt);
        break;
    case BMI2_E_REMAP_ERROR:
        ESP_LOGE(TAG, "Error [%d] : Remapping error", rslt);
        break;
    case BMI2_E_GYR_USER_GAIN_UPD_FAIL:
        ESP_LOGE(TAG, "Error [%d] : Gyro user gain update failed", rslt);
        break;
    case BMI2_E_SELF_TEST_NOT_DONE:
        ESP_LOGE(TAG, "Error [%d] : Self test not done", rslt);
        break;
    case BMI2_E_INVALID_INPUT:
        ESP_LOGE(TAG, "Error [%d] : Invalid input", rslt);
        break;
    case BMI2_E_INVALID_STATUS:
        ESP_LOGE(TAG, "Error [%d] : Invalid status", rslt);
        break;
    case BMI2_E_CRT_ERROR:
        ESP_LOGE(TAG, "Error [%d] : CRT error", rslt);
        break;
    case BMI2_E_ST_ALREADY_RUNNING:
        ESP_LOGE(TAG, "Error [%d] : Self test already running", rslt);
        break;
    case BMI2_E_CRT_READY_FOR_DL_FAIL_ABORT:
        ESP_LOGE(TAG, "Error [%d] : CRT ready for DL fail abort", rslt);
        break;
    case BMI2_E_DL_ERROR:
        ESP_LOGE(TAG, "Error [%d] : DL error", rslt);
        break;
    case BMI2_E_PRECON_ERROR:
        ESP_LOGE(TAG, "Error [%d] : PRECON error", rslt);
        break;
    case BMI2_E_ABORT_ERROR:
        ESP_LOGE(TAG, "Error [%d] : Abort error", rslt);
        break;
    case BMI2_E_GYRO_SELF_TEST_ERROR:
        ESP_LOGE(TAG, "Error [%d] : Gyro self test error", rslt);
        break;
    case BMI2_E_GYRO_SELF_TEST_TIMEOUT:
        ESP_LOGE(TAG, "Error [%d] : Gyro self test timeout", rslt);
        break;
    case BMI2_E_WRITE_CYCLE_ONGOING:
        ESP_LOGE(TAG, "Error [%d] : Write cycle ongoing", rslt);
        break;
    case BMI2_E_WRITE_CYCLE_TIMEOUT:
        ESP_LOGE(TAG, "Error [%d] : Write cycle timeout", rslt);
        break;
    case BMI2_E_ST_NOT_RUNING:
        ESP_LOGE(TAG, "Error [%d] : Self test not running", rslt);
        break;
    case BMI2_E_DATA_RDY_INT_FAILED:
        ESP_LOGE(TAG, "Error [%d] : Data ready interrupt failed", rslt);
        break;
    case BMI2_E_INVALID_FOC_POSITION:
        ESP_LOGE(TAG, "Error [%d] : Invalid FOC position", rslt);
        break;
    default:
        ESP_LOGE(TAG, "Error [%d] : Unknown error code", rslt);
        break;
    }
}
