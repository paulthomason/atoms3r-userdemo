echo "upload asset pool"
parttool.py --port "/dev/ttyACM0" write_partition --partition-name=assetpool --input "asset_pool_gen/output/AssetPool.bin"
