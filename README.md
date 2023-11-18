# MCUfirmwares

esptool.py --chip esp32s3 --port /dev/ttyACM0 erase_flash

image from https://micropython.org/download/ESP32_GENERIC_S3/

wget https://micropython.org/resources/firmware/ESP32_GENERIC_S3-20231005-v1.21.0.bin 
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash -z 0x1000 ESP32_GENERIC_S3-20231005-v1.21.0.bin 




mpremote fs cp config.json :config.json
mpremote fs cp  atom-mp.py :main.py

