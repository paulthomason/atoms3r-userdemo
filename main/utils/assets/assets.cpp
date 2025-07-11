/**
 * @file assets.cpp
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2024-05-11
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "assets.h"
#include <mooncake.h>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <string>
#ifndef ESP_PLATFORM
#include "images/images.h"
#include <iostream>
#include <fstream>
#endif

AssetPool* AssetPool::_asset_pool = nullptr;

AssetPool* AssetPool::Get()
{
    if (_asset_pool == nullptr) _asset_pool = new AssetPool;
    return _asset_pool;
}

StaticAsset_t* AssetPool::getStaticAsset()
{
    if (_data.static_asset == nullptr) {
        spdlog::error("static asset not exsit");
        return nullptr;
    }
    return _data.static_asset;
}

bool AssetPool::injectStaticAsset(StaticAsset_t* asset)
{
    if (_data.static_asset != nullptr) {
        spdlog::error("static asset already exist");
        return false;
    }

    if (asset == nullptr) {
        spdlog::error("invalid static asset ptr");
        return false;
    }

    _data.static_asset = asset;

    // // Default local text map
    // setLocalTextTo(_data.locale_code);

    spdlog::info("static asset injected");
    return true;
}

/* -------------------------------------------------------------------------- */
/*                            Static asset generate                           */
/* -------------------------------------------------------------------------- */
#ifdef PLATFORM_BUILD_DESKTOP

/**
 * @brief Copy file into target as binary
 *
 * @param filePath
 * @param target
 * @return true
 * @return false
 */
static bool _copy_file(std::string filePath, uint8_t* target)
{
    spdlog::info("try open {}", filePath);

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        spdlog::error("open failed!", filePath);
        return false;
    }
    std::streampos file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    spdlog::info("file binary size {}", file_size);

    // Copy and go
    if (target != nullptr) file.read(reinterpret_cast<char*>(target), file_size);
    file.close();
    return true;
}

/**
 * @brief Convert png image into rgb565 and copy into target as binary
 *
 * @param filePath
 * @param target
 * @return true
 * @return false
 */
#ifdef ENABLE_PNG_CONVERTOR
static bool _copy_png_image(std::string filePath, uint16_t* target)
{
    spdlog::info("try convert: {}", filePath);
    size_t output_length = 0;
    int width = 0, height = 0;

    ImageConversionError result = convertPNGToR5G6B5(filePath.c_str(), target, &output_length, &width, &height);

    if (result != ImageConversionError::Success) {
        spdlog::error("convert failed: {}", static_cast<int>(result));
        return false;
    }

    spdlog::info("ok, image size: {} x {}, array length: {}", width, height, output_length);
    return true;
}
#endif

StaticAsset_t* AssetPool::CreateStaticAsset()
{
    auto asset_pool = new StaticAsset_t;

    _copy_file("../../main/utils/assets/images/index.html.gz", asset_pool->Image.index_html_gz);
    _copy_file("../../main/utils/assets/images/m5.jpg", asset_pool->Image.m5_logo);

    return asset_pool;
}

void AssetPool::CreateStaticAssetBin(StaticAsset_t* assetPool)
{
    /* -------------------------------------------------------------------------- */
    /*                                Output to bin                               */
    /* -------------------------------------------------------------------------- */
    std::string bin_path = "../output/AssetPool.bin";

    std::ofstream outFile(bin_path, std::ios::binary);
    if (!outFile) spdlog::error("open {} failed", bin_path);

    outFile.write(reinterpret_cast<const char*>(assetPool), sizeof(StaticAsset_t));
    outFile.close();
    spdlog::info("output asset pool to: {}", bin_path);
}

StaticAsset_t* AssetPool::GetStaticAssetFromBin()
{
    auto asset_pool = new StaticAsset_t;

    // Read from bin
    std::string bin_path = "../output/AssetPool.bin";

    std::ifstream inFile(bin_path, std::ios::binary);
    if (!inFile) spdlog::error("open {} failed", bin_path);

    inFile.read(reinterpret_cast<char*>(asset_pool), sizeof(StaticAsset_t));
    inFile.close();

    // // Test
    // for (int i = 0; i < 10; i++)
    // {
    //     spdlog::info(
    //         "0x{:X} 0x{:X}", asset_pool->Font.montserrat_semibold_14[i],
    //         asset_pool->Font.montserrat_semibolditalic_72[i]);
    // }

    spdlog::info("load asset pool from: {}", bin_path);
    return asset_pool;
}
#else

StaticAsset_t* AssetPool::CreateStaticAsset()
{
    auto asset_pool = new StaticAsset_t;

    // ...

    return asset_pool;
}

#endif
