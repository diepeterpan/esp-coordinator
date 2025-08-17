ZBOSS NCP Serial Protocol implementation for ESP32-C6/H2 module.

This implementation functional limited to coordinator role for usage with [zigbee2mqtt](https://www.zigbee2mqtt.io).

[Protocol specification](https://wiki.homed.dev/files/9/95/ZBOSS_NCP_Serial_Protocol.pdf)

Configuration examle:
```
serial:
  port: /dev/ttyACM0
  adapter: zboss
  rtscts: true
```

At [releases](https://github.com/diepeterpan/esp-coordinator/releases) example firmware build for [WeAct Studio ESP32-C6-mini module](https://aliexpress.ru/item/1005006800070921.html)

Flashing offsets:
```
0x0 bootloader.bin
0x20000 esp-coordinator.bin
0x8000 partition-table.bin
0xf000 ota_data_initial.bin
```
Example flash using esptool

```
wget https://github.com/diepeterpan/esp-coordinator/releases/download/v0.0.2/esp-coordinator-escp32-c6-usb-.zip

unzip esp-coordinator-escp32-c6-usb-.zip -d esp32c6coordinator
cd esp32c6coordinator

# Omit the erase_flash step if you are upgrading an existing ESP running esp-coordinator,
# otherwise you will lose your Zigbee network and form a new one, and all devices will have to be repaired
esptool --chip esp32-C6 --port /dev/ttyACM0 --baud 460800 --erase_flash

esptool --chip esp32-C6 \
--port /dev/ttyACM0 \
--baud 460800 \
--before default_reset \
--after hard_reset write_flash \
--flash_mode dio \
--flash_freq 40m \
--flash_size 4MB \
0x0 bootloader/bootloader.bin \
0x20000 esp-coordinator.bin \
0x8000 partition_table/partition-table.bin \
0xf000 ota_data_initial.bin
```

Local build setup on Linux

```
git clone -b v5.5 --recursive https://github.com/espressif/esp-idf.git esp-idf-v5.5

cd esp-idf-v5.5/
./install.sh
. ./export.sh

cd ..
git clone https://github.com/diepeterpan/esp-coordinator
cd esp-coordinator/

idf.py set-target esp32c6
idf.py build
```
