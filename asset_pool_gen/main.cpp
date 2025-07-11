/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <assets.h>

int main(int, char**)
{
    // Create and output to bin
    AssetPool::CreateStaticAssetBin(AssetPool::CreateStaticAsset());
}
