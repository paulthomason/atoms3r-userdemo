/**
 * @file shared.h
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2024-05-11
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include "types.h"

/**
 * @brief Provide a singleton for sharing dynamic data between apps(or layers, not recommended)
 *
 */
class SharedData {
private:
    static SharedData* _shared_data;

public:
    /**
     * @brief Get SharedData instance
     *
     * @return SharedData*
     */
    static SharedData* Get();

    /**
     * @brief Check if SharedData is valid
     *
     * @return true
     * @return false
     */
    static bool Check();

    /**
     * @brief SharedData injection
     *
     * @param sharedData
     * @return true
     * @return false
     */
    static bool Inject(SharedData* sharedData);

    /**
     * @brief Destroy SharedData instance
     *
     */
    static void Destroy();

    /**
     * @brief Sharing data
     *
     */
protected:
    SHARED_DATA::SharedData_t _data;

    /**
     * @brief Base class
     *
     */
public:
    SharedData()
    {
    }
    virtual ~SharedData()
    {
    }

    static std::string Type()
    {
        return Get()->type();
    }
    virtual std::string type()
    {
        return "Base";
    }

    /**
     * @brief Get data directly
     *
     * @return SHARED_DATA::SharedData_t&
     */
    static SHARED_DATA::SharedData_t& GetData()
    {
        return Get()->getData();
    }
    virtual SHARED_DATA::SharedData_t& getData()
    {
        return _data;
    }

    /**
     * @brief Borrow data, override to lock mutex or whatever
     *
     * @return SHARED_DATA::SharedData_t&
     */
    static SHARED_DATA::SharedData_t& BorrowData()
    {
        return Get()->borrowData();
    }
    virtual SHARED_DATA::SharedData_t& borrowData()
    {
        return _data;
    }

    /**
     * @brief Return data, override to unlock mutex or whatever
     *
     */
    static void ReturnData()
    {
        Get()->returnData();
    }
    virtual void returnData()
    {
    }

    /* -------------------------------------------------------------------------- */
    /*                               Helper getters                               */
    /* -------------------------------------------------------------------------- */
public:
    static std::string AppVersion()
    {
        return Get()->appVersion();
    }
    virtual std::string appVersion()
    {
        return APP_VERSION;
    }

    // Add your hepler getter here
    // static int GetShitNum() { return GetData().shitNum; }
    // ...

    /* -------------------------------------------------------------------------- */
    /*                                Service mode                                */
    /* -------------------------------------------------------------------------- */
    static ServiceMode::ServiceMode_t GetServiceMode()
    {
        ServiceMode::ServiceMode_t ret = BorrowData().service_mode;
        ReturnData();
        return ret;
    }

    static void SetServiceMode(ServiceMode::ServiceMode_t mode)
    {
        BorrowData().service_mode = mode;
        ReturnData();
    }

    /* -------------------------------------------------------------------------- */
    /*                                     IMU                                    */
    /* -------------------------------------------------------------------------- */
    static void UpdateImuData();
    static const IMU::ImuData_t& GetImuData()
    {
        return Get()->_data.imu_data;
    }

    /* -------------------------------------------------------------------------- */
    /*                                     IR                                     */
    /* -------------------------------------------------------------------------- */
    static void IrSendNecMsg(uint16_t addr, uint16_t command);
};

/* -------------------------------------------------------------------------- */
/*                      Shared data with std::mutex lock                      */
/* -------------------------------------------------------------------------- */
#include <thread>
#include <mutex>

class SharedData_StdMutex : public SharedData {
private:
    std::mutex _mutex;

public:
    std::string type() override
    {
        return "StdMutex";
    }

    SHARED_DATA::SharedData_t& borrowData() override
    {
        _mutex.lock();
        return _data;
    }

    void returnData() override
    {
        _mutex.unlock();
    }
};
