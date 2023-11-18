# Only needed the first time

```
esptool.py --chip esp32s3 --port /dev/ttyACM0 erase_flash
```

take an image from https://micropython.org/download/ESP32_GENERIC_S3/

```
wget https://micropython.org/resources/firmware/ESP32_GENERIC_S3-20231005-v1.21.0.bin 
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash -z 0x1000 ESP32_GENERIC_S3-20231005-v1.21.0.bin 
```



# Configure when settings change
Edit config.json then copy it to the device
```
mpremote fs cp config.json :config.json
```

#Update firmware when needed
```
mpremote fs cp  atom-mp.py :main.py
```

