/**
 * @file assets.h
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
#include "images/types.h"

/**
 * @brief A struct to define static binary asset
 *
 */
struct StaticAsset_t {
    ImagePool_t Image;
};

/**
 * @brief A Class to handle static asset
 *
 */
class AssetPool {
    /* -------------------------------------------------------------------------- */
    /*                                  Singleton                                 */
    /* -------------------------------------------------------------------------- */
private:
    static AssetPool* _asset_pool;

public:
    static AssetPool* Get();

private:
    struct Data_t {
        StaticAsset_t* static_asset = nullptr;
    };
    Data_t _data;

    /* ------------------------------ Static asset ------------------------------ */
public:
    StaticAsset_t* getStaticAsset();
    bool injectStaticAsset(StaticAsset_t* pool);

    /* ------------------------------- Static wrap ------------------------------ */
public:
    static StaticAsset_t* GetStaticAsset()
    {
        return Get()->getStaticAsset();
    }
    static bool InjectStaticAsset(StaticAsset_t* asset)
    {
        return Get()->injectStaticAsset(asset);
    }

    static const ImagePool_t& GetImage()
    {
        return GetStaticAsset()->Image;
    }

public:
    /* -------------------------- Generate static asset ------------------------- */
#ifdef PLATFORM_BUILD_DESKTOP
    static StaticAsset_t* CreateStaticAsset();
    static void CreateStaticAssetBin(StaticAsset_t* assetPool);
    static StaticAsset_t* GetStaticAssetFromBin();
#else
    // Dummy, or you just want simple asset pool
    static StaticAsset_t* CreateStaticAsset();
#endif
};
