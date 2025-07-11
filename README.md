# AtomS3R-CAM User Demo

User demo source code of AtomS3R-CAM

## Build

### Fetch Dependencies

```bash
python ./fetch_repos.py
```

### Tool Chains

[ESP-IDF v5.1.4](https://docs.espressif.com/projects/esp-idf/en/v5.1.4/esp32s3/index.html)

### Build

```bash
idf.py build
```

### Flash

```bash
idf.py -p <YourPort> flash -b 1500000
```

#### Flash AssetPool

```bash
parttool.py --port <YourPort> write_partition --partition-name=assetpool --input "asset_pool_gen/output/AssetPool.bin"
```
