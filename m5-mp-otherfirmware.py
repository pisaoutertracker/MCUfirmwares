import sht31
from umqttsimple import *
import qmp6988
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


lastDisplay = time.time()
lastReport = time.time()
mqtt_client = None
wlan = network.WLAN(network.STA_IF)
wlan.active(False)
wlan.active(True)
#wlan.config(reconnects=5)
wlan.connect(config["ssid"], config['wlanpass'])


while not wlan.isconnected():
    print("waiting for wlan")
    sleep(1)
print("WiFi is on:",wlan.isconnected())
mqtt_client = MQTTClient(config['mqttclient'], config['mqttserver'], port=1883, user='', password='', keepalive=30)
mqtt_client.connect(clean_session=True)

i2c = I2C(0, scl=Pin(22), sda=Pin(21), freq=100000)
prt = qmp6988.QMP6988(i2c)
sensor = sht31.SHT31(i2c, addr=0x44)
print(sensor.get_temp_humi())
maxmoda=0
dispUpdate=1
mqttUpdate=60
lt=0
lrh=0
ldp=0
from m5stack import lcd

while True:
  curr = time.time()
  if curr - lastDisplay < 0 or curr - lastDisplay > dispUpdate :
    lastDisplay = time.time()
    t,rh=sensor.get_temp_humi()
    dew_point=get_dew_point_c(t,rh)
    pressure=prt.measure()[1]
    if not compatible(t,lt) or not compatible(rh,lrh) or not compatible(dew_point,ldp) :
      lrh=rh
      lt=t
      ldp=dew_point

      #bgcolor= st7789py.color565(0,255,0) if dew_point < 20 else st7789py.color565(0,0,255)
      #display.fill(bgcolor)
      lcd.clear()
      lcd.text(0,0, "%.1f C"%t)
      lcd.text(0,30, "%.1f %%"%rh)
      lcd.text(0,60, "%.1f C"%dew_point)

      #display.text(font,"%.1f C"%t,0,0, st7789py.color565(0,0,0),bgcolor )
      #display.text(font,"%.1f %%"%rh,0,30, st7789py.color565(0,0,0),bgcolor )
      #display.text(font,"%.1f C"%dew_point,0,60, st7789py.color565(0,0,0),bgcolor )

  if curr - lastReport < 0 or curr - lastReport > mqttUpdate :
    lastReport = time.time()
    if wlan.isconnected() :
#      try:
         print("Report to MQTT server")
         mqtt_client.connect(clean_session=True)
         message= '{"dewpoint": %.1f, "temperature":%.1f,"RH":%.0f,"Pressure":%.0f}'%(dew_point,t,rh,pressure)
         print(message)
         mqtt_client.publish(config['mqtttopic'],message, qos=0)
#      except:
#         print("Failed")
    else:
      print("no internet")


