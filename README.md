# AtomS3
### Only needed the first time

```
esptool.py --chip esp32s3 --port /dev/ttyACM0 erase_flash
```

take an image from https://micropython.org/download/ESP32_GENERIC_S3/

```
wget https://micropython.org/resources/firmware/ESP32_GENERIC_S3-20231005-v1.21.0.bin 
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash -z 0x1000 ESP32_GENERIC_S3-20231005-v1.21.0.bin 
```



### Configure when settings change
Edit config.json then copy it to the device
```
mpremote fs cp config.json :config.json
```

### Update firmware when needed
```
mpremote fs cp  atom-mp.py :main.py
```


# M5Stack


### Install firmware

```
wget https://github.com/russhughes/ili9342c_mpy/raw/main/firmware/M5STACK_16M/firmware.bin
esptool.py --chip esp32 --port /dev/ttyACM0 write_flash -z 0x1000  firmware.bin
```
Reboot the device.

### Install needed packages
```
mpremote mip install https://github.com/kfricke/micropython-sht31/raw/master/sht31.py
mpremote mip install https://raw.githubusercontent.com/RuiSantosdotme/ESP-MicroPython/master/code/MQTT/umqttsimple.py
mpremote mip install https://github.com/gandro/micropython-m5stamp-c3u/raw/main/lib/qmp6988.py
```

### Edit configuration
Edit config.json and once correct upload it
```
mpremote  fs cp config.json :config.json
```

### Test or install py main 
To permanently install:
```
mpremote fs cp  m5-mp.py :main.py
```

To just test
```
mpremote run m5-mp.py
```

## Compiling custom firmware for M5Stack

Tools needed (on ubuntu)
```
 sudo apt install ncurses-base
 sudo apt-get install libncurses-dev
 sudo apt search flex
 sudo apt install flex
 sudo apt install bison
 sudo apt install gperf
```

Download and compile firmware
```
git clone https://github.com/m5stack/M5Stack_MicroPython.git
cd M5Stack_MicroPython/MicroPython_BUILD/
cp sdkconfig.defaults  sdkconfig
./BUILD.sh -v
```

Flash firmware
```
./BUILD.sh erase -p /dev/ttyACM0 
./BUILD.sh flash -p /dev/ttyACM0 
cd ../..
```



