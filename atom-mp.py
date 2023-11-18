try:
    import vga2_bold_16x32 as font
    import qmp6988
    import sht31
    from umqttsimple import *
    from mpu6886 import MPU6886,ACCEL_FS_SEL_16G
    import st7789py
except:
    import mip
    print("First run, installing packages")
    mip.install("https://github.com/gandro/micropython-m5stamp-c3u/raw/main/lib/qmp6988.py")
    mip.install("https://github.com/kfricke/micropython-sht31/raw/master/sht31.py")
    mip.install("https://raw.githubusercontent.com/RuiSantosdotme/ESP-MicroPython/master/code/MQTT/umqttsimple.py")
    mip.install("https://github.com/tuupola/micropython-mpu6886/raw/master/mpu6886.py")
    mip.install("https://github.com/arizzi/st7789py_mpy/raw/master/lib/st7789py.py")
    mip.install("https://github.com/arizzi/st7789py_mpy/raw/master/fonts/romfonts/vga2_bold_16x32.py")

    from mpu6886 import MPU6886,ACCEL_FS_SEL_16G
    from umqttsimple import *
    import sht31
    import qmp6988
    import st7789py
    import vga2_bold_16x32 as fonts


import os, sys, io
from machine import Pin,I2C,SPI
import network
from time import sleep
import time
import math
def compatible(a,b):
    return abs((a-b)/(a+b)) < 0.01

def get_dew_point_c(t_air_c, rel_humidity):
    A = 17.27
    B = 237.7
    alpha = ((A * t_air_c) / (B + t_air_c)) + math.log(rel_humidity/100.0)
    return (B * alpha) / (A - alpha)

import json
config=json.load(open('config.json'))


spi = SPI(1, baudrate=31250000, sck=Pin(17), mosi=Pin(21))

display = st7789py.ST7789(
        spi,
        128,
        128,
        reset=Pin(34, Pin.OUT),
        cs=Pin(15, Pin.OUT),
        dc=Pin(33, Pin.OUT),
        backlight=Pin(16, Pin.OUT),
        rotation=0)

lastDisplay = time.time()
lastReport = time.time()
mqtt_client = None
wlan = network.WLAN(network.STA_IF)
wlan.active(False)
wlan.active(True)
wlan.config(reconnects=5)
wlan.connect(config["ssid"], config['wlanpass'])

from machine import SoftI2C
i2cs = SoftI2C(scl=Pin(39), sda=Pin(38))
accsensor = MPU6886(i2cs,accel_fs=ACCEL_FS_SEL_16G)

while not wlan.isconnected():
    print("waiting for wlan")
    sleep(1)
print("WiFi is on:",wlan.isconnected())
mqtt_client = MQTTClient(config['mqttclient'], config['mqttserver'], port=1883, user='', password='', keepalive=30)
mqtt_client.connect(clean_session=True)

i2c = I2C(0, scl=Pin(1), sda=Pin(2), freq=100000)
prt = qmp6988.QMP6988(i2c)
sensor = sht31.SHT31(i2c, addr=0x44)
print(sensor.get_temp_humi())
maxmoda=0
dispUpdate=1
mqttUpdate=60
lt=0
lrh=0
ldp=0
lmm=0
while True:
  curr = time.time()
  a=accsensor.acceleration
  moda=a[0]**2+a[1]**2+a[2]**2
  moda=moda**0.5
  if moda > maxmoda :
      maxmoda=moda
  if curr - lastDisplay < 0 or curr - lastDisplay > dispUpdate :
    lastDisplay = time.time()
    t,rh=sensor.get_temp_humi()
    dew_point=get_dew_point_c(t,rh)
    pressure=prt.measure()[1]
    if not compatible(t,lt) or not compatible(rh,lrh) or not compatible(dew_point,ldp) or not compatible(lmm,maxmoda) :
      lmm=maxmoda
      lrh=rh
      lt=t
      ldp=dew_point
      bgcolor= st7789py.color565(0,255,0) if dew_point < 20 else st7789py.color565(0,0,255)
      display.fill(bgcolor)
      display.text(font,"%.1f C"%t,0,0, st7789py.color565(0,0,0),bgcolor )
      display.text(font,"%.1f %%"%rh,0,30, st7789py.color565(0,0,0),bgcolor )
      display.text(font,"%.1f C"%dew_point,0,60, st7789py.color565(0,0,0),bgcolor )
      display.text(font,"%.1f g"%(maxmoda/9.81),0,90, st7789py.color565(0,0,0),bgcolor )
      maxmoda=0

  if curr - lastReport < 0 or curr - lastReport > mqttUpdate :
    lastReport = time.time()
    if wlan.isconnected() :
      try:
         print("Report to MQTT server")
         mqtt_client.connect(clean_session=True)
         message= '{"dewpoint": %.1f, "temperature":%.1f,"RH":%.0f,"Pressure":%.0f}'%(dew_point,t,rh,pressure)
         print(message)
         mqtt_client.publish(config['mqtttopic'],message, qos=0)
      except:
         print("Failed")
    else:
      print("no internet")


