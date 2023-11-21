from time import sleep
import network
import st7789py
import vga1_8x16 as font
import button
from machine import Pin, SPI, SoftI2C
from mpu6886 import MPU6886,ACCEL_FS_SEL_16G

from math import sqrt

class AtomS3:
    def __init__(self, config,callback ) -> None:
        spi = SPI(1, baudrate=31250000, sck=Pin(17), mosi=Pin(21))
        self.display = st7789py.ST7789(
                spi,
                128,
                128,
                reset=Pin(34, Pin.OUT),
                cs=Pin(15, Pin.OUT),
                dc=Pin(33, Pin.OUT),
                backlight=Pin(16, Pin.OUT),
                rotation=2)
        i2cs = SoftI2C(scl=Pin(39), sda=Pin(38))
        self.mpu = MPU6886(i2cs,accel_fs=ACCEL_FS_SEL_16G)
        self.wlan = network.WLAN(network.STA_IF)
        self.wlan.active(False)
        self.wlan.active(True)
        self.wlan.config(reconnects=5)
        self.wlan.connect(config["ssid"], config['wlanpass'])
        self.display.fill(st7789py.BLACK)
        # Usage:
        self.button_handler = button.ButtonHandler(41, callback)

        # while not self.wlan.isconnected():
        #     self.display.text(font, "waiting for wlan", 0, 0, st7789py.WHITE)
        #     sleep(1)
        # self.display.fill(st7789py.BLACK)
        # self.display.text(font, "WiFi is on", 0, 0, st7789py.WHITE)
  
        
    def acceleration(self):
            return self.mpu.acceleration
        
    # return the module of acceleartion in unit of g
    def acc_g(self):
            a=self.acceleration()
            return sqrt(a[0]**2+a[1]**2+a[2]**2)/9.81
