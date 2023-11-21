try:
    import vga1_8x16 as font
   # import vga2_bold_16x16 as font
   # import vga2_bold_16x32 as font
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
    mip.install("https://github.com/arizzi/st7789py_mpy/raw/master/fonts/romfonts/vga1_8x16.py")
    import vga1_8x16 as font
    from mpu6886 import MPU6886,ACCEL_FS_SEL_16G
    from umqttsimple import *
    import sht31
    import qmp6988
    import st7789py

from AtomS3 import AtomS3
import ustruct
import os, sys, io
from machine import Pin,I2C
from time import sleep
import time
import math
import uasyncio as asyncio
import usocket as socket
import json
from array import array
#Zero suppressed Logger class
class ZSLogger: 
    def __init__(self, filename, maxInterval=600,resolution=0.3,cachesize=200):
        self.filename=filename
        self.maxInterval=maxInterval
        self.resolution=resolution
        self.cachesize=cachesize
        self.lastTime=0
        self.lastData=0
#        self.zsdata=[]
        self.index=0
        self.tdata=array("H",[0]*cachesize)
        self.ddata=array("b",[0]*cachesize)
        # find last file progressive number
        files = os.listdir("/data/")
        files.sort()
        last=0
        print(files)
        for f in files:
            if f.startswith(filename):
                try:
                    last=max(last,int(f[len(filename):-4]))
                except:
                    pass
        print("Last",last)
        self.last=last

    def log(self,t,data):
        if t - self.lastTime > self.maxInterval or abs(data-self.lastData) > self.resolution :
            print("log",t,data)
            if self.lastTime==0:
                delta=0
            else:
                delta=min(65535,(t-self.lastTime)*65536./self.maxInterval)
            scaled=min(128,max(data/self.resolution,-127))
#            self.zsdata.append((delta,scaled))
            self.tdata[self.index]=int(delta)
            self.ddata[self.index]=int(scaled)
            self.index+=1
            self.lastTime=t
            self.lastData=data
 #           print(t,data,delta,scaled)
            if self.index>=self.cachesize :
                self.flush()

    def flush(self):
        self.writeData()
        self.index=0
#        self.zsdata=[]

    def decode_data(self,data,t=0):
        intervals=[ (d[0]*self.maxInterval/65536.,d[1]*self.resolution) for d in data]
        out=[]
        for i in intervals:
            t+=i[0]
            out.append((t,i[1]))
        return out
    
    def writeData(self):
        # Open file in write binary mode incrementing last filename
        self.last+=1
        with open("/data/"+self.filename+"%03d"%(self.last)+".bin", 'ab') as f:
            print("flushing to file")
            # Iterate over the data
            for i in range(self.index):
                # Pack data into binary format (H is 2-byte unsigned int, b is 1-byte signed int)
                binary_data = ustruct.pack('Hb', self.tdata[i], self.ddata[i])
                # Write to file
                f.write(binary_data)

        # Close the file
        f.close()
    
    def read_bin_file(self,name):
        with open(name, 'rb') as f:
            raw_data = f.read()
        # Assuming every record in binary file is 3 bytes long.
        data = [ustruct.unpack('Hb', raw_data[i:i+3]) for i in range(0, len(raw_data), 3)]
        return data
    


def compatible(a,b):
    return abs((a-b)/(a+b)) < 0.01


class EnvironmentSensor:
    def __init__(self):
        i2c = I2C(0, scl=Pin(1), sda=Pin(2), freq=100000)
        self.pression = qmp6988.QMP6988(i2c)
        self.temphum = sht31.SHT31(i2c, addr=0x44)
        self.last_measurements = [0,0,0,0]
    def get_temp_humi(self):
        return self.temphum.get_temp_humi()
    def get_pressure(self):
        return self.pression.measure()[1]
    def calculate_dewpoint(self,t_air_c, rel_humidity): 
        A = 17.27
        B = 237.7
        alpha = ((A * t_air_c) / (B + t_air_c)) + math.log(rel_humidity/100.0)
        return (B * alpha) / (A - alpha)
    def get_dew_point(self):
        t,rh=self.temphum.get_temp_humi()
        return self.calculate_dewpoint(t,rh)  
    def read_all(self):
        t,rh=self.temphum.get_temp_humi()
        p=self.pression.measure()[1]
        dp=self.calculate_dewpoint(t,rh)
        return t,rh,p,dp
    
    def check_sensors(self):
        current=self.read_all()
        ret=False
        for i,v in enumerate(current):
           if not compatible(v,self.last_measurements[i]):
                self.last_measurements[i]=v
                ret=True
        return ret

        


#application class
# handling the application logic
class Application:
    def __init__(self) -> None:
        self.config=json.load(open('config.json'))
        self.hardware = AtomS3(self.config,self.buttonCallback)
        self.display=self.hardware.display
        self.env=EnvironmentSensor()
        self.logger = ZSLogger("acc")
        self.mqtt_client = MQTTClient(self.config['mqttclient'], self.config['mqttserver'], port=1883, user='', password='', keepalive=30)
        self.lastReport=0
        self.connected=False

    def buttonCallback(self):
        print("Flushing data")
        self.logger.flush()

    def mqttPublish(self,topic,dew_point,t,rh,pressure):
        self.lastReport = time.time()
        if self.hardware.wlan.isconnected() :
           # try:
                print("Report to MQTT server")
                self.mqtt_client.connect(clean_session=True)
                message= '{"dewpoint": %.1f, "temperature":%.1f,"RH":%.0f,"Pressure":%.0f}'%(dew_point,t,rh,pressure)
                print(message)
                self.mqtt_client.publish(self.config['mqtttopic'],message, qos=0)
           # except:
            #    print("Failed")
        else:
            print("no internet")

    async def environment_sensors_loop(self):
        while True:
            if self.env.check_sensors() or self.connected != self.hardware.wlan.isconnected():
                self.connected=self.hardware.wlan.isconnected()
                t,rh,p,dp=self.env.last_measurements
                self.update_display(t,rh,p,dp)
                # limit the rate of mqtt publication
                if time.time() - self.lastReport > self.config['mqttinterval'] or time.time() < self.lastReport:
                    self.mqttPublish(self.config['mqtttopic'],dp,t,rh,p)
                    self.lastReport = time.time()
            await asyncio.sleep_ms(1000)

    def update_display(self,t,rh,p,dp):
        bgcolor= st7789py.color565(0,255,0) if dp < self.config['alarmdp'] else st7789py.color565(0,0,255)
        self.display.fill(bgcolor)
        self.display.text(font, "Temp: {:.1f}".format(t), 0, 0, st7789py.BLACK , bgcolor)
        self.display.text(font, "R.H.: {:.1f}".format(rh), 0, 16,st7789py.BLACK, bgcolor)
        self.display.text(font, "Pres: {:.2f}".format(p/100.), 0, 32, st7789py.BLACK,bgcolor)
        self.display.text(font, "DewP: {:.1f}".format(dp), 0, 48, st7789py.BLACK,bgcolor)
        self.display.text(font, "Mem: %s"%(gc.mem_free()), 0, 96, st7789py.BLACK,bgcolor)

        if self.hardware.wlan.isconnected():  
            self.display.text(font, "%s"%(self.hardware.wlan.ifconfig()[0]), 0, 112, st7789py.BLACK,bgcolor)
        else:
            self.display.text(font, "No internet", 0, 112, st7789py.BLACK,st7789py.RED)

    async def acceleration_sensor_loop(self):
        while True:
            self.logger.log(time.time_ns()/1e9,self.hardware.acc_g())  
#            print(time.time_ns()/1e9,self.hardware.acc_g())  
#            print(self.hardware.acc_g())  
            await asyncio.sleep(0.0001)
        
    async def server_callback(self,reader, writer):
        print("server callback")
        request=b''
        while True:
            line = await reader.readline()
            request+=line
            if line == b'\r\n':
                break
        request=request.decode('utf-8')
        if '/data' in request:
            files = os.listdir('/data/')
            files.sort()
            #print(files)
            data = []
            writer.write('HTTP/1.0 200 OK\r\nContent-type: application/json\r\n\r\n[')
            entries=0
            last=0
            for f in files:
                #print(f)
                if f.endswith('.bin'):
                    fd = self.logger.read_bin_file('/data/' + f)
                    #logger decode data
                    data=self.logger.decode_data(fd,data[-1][0] if len(data) > 0 else 0 )
                    for i,d in enumerate(data):
                        if entries > 0:
                            writer.write(',')
                            writer.write("[%s,%s],"%(d[0],last))
                        writer.write("[%s,%s]"%(d[0],d[1]))
                        last=d[1]
                        entries+=1
                        
#            for i in range(self.logger.index):
#                if entries > 0:
#                   writer.write(',')
#                writer.write("[%s,%s]"%(self.logger.tdata[i],self.logger.ddata[i])) 
#                entries+=1

    #        print(data)
#            response = 'HTTP/1.0 200 OK\r\nContent-type: application/json\r\n\r\n' + json.dumps(data)
 #           print(response)
            writer.write(']')

        else :
            # open the file index.html and send it
            with open('index.html', 'rb') as f:
                writer.write(b'HTTP/1.0 200 OK\r\n')
                writer.write(b'Content-Type: text/html\r\n')
                writer.write(b'\r\n')
                content = f.read()
                writer.write(content)
#            writer.write(b'<h1>Hello from MicroPython!</h1>')
        print("drain")
        await writer.drain()
        print("close")
        await writer.wait_closed()

#        writer.close()
        print("closed")


async def main():
    app=Application()
    asyncio.create_task(app.environment_sensors_loop())
    asyncio.create_task(app.acceleration_sensor_loop())
    asyncio.create_task(asyncio.start_server(app.server_callback, "0.0.0.0", 80))
    while True:
        await asyncio.sleep_ms(1_000)


asyncio.run(main())


