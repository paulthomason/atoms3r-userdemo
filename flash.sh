#!/bin/bash
### Use < usbipd wsl attach -a -b [BUSID] > on PowerShell to auto attach the board


# Configs
IDF_PATH=$HOME/esp/esp-idf-v5.1.4
SERIAL_PORT=/dev/ttyACM0


# Help shit
help() {
    sed -rn 's/^### ?//;T;p' "$0"
}

if [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
    help
    exit 1
fi

# Get idf
. ${IDF_PATH}/export.sh

# Build and flash and monitor
idf.py -p ${SERIAL_PORT} flash -b 1500000 monitor
