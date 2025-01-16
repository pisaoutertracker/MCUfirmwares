#include <M5Unified.h>
#include <WiFi.h>

// WiFi network configuration
const char* ssid = "M5Stack_AP";
const char* password = "12345678";
const char* serverIP = "192.168.100.1";
const uint16_t serverPort = 1234;

// WiFi network configuration
const char* ssidLocal = "CMSPisa";
const char* passwordLocal = "silicon2003";


String clientName="B ";
// WiFi client
WiFiClient client;
#include "M5UnitENV.h"
SHT3X sht3x;
QMP6988 qmp;

struct Data {
  float x,y,z,total,maxTotal;
  unsigned long t;
};
const int MAX_DATA=10000;
Data buffer[10000];
int dataStart=0;
int dataLen=0;
bool hasEnv=true;
int mode=-1;

void setup() {
  M5.begin();
  Serial.begin(115200);
  
  clientName="MAC"+WiFi.macAddress();

  // Initialize the screen
  M5.Lcd.println("Connecting to WiFi...");

  // Connect to the Wi-Fi network
  WiFi.begin(ssid, password);
  int counter=0;
  while (WiFi.status() != WL_CONNECTED and counter++ < 10) {
    delay(1000);
    M5.Lcd.print(".");
    if (WiFi.status() != WL_CONNECTED) {
       //try local network
      WiFi.begin(ssidLocal, passwordLocal);
      delay(1000);
      M5.Lcd.print("L");
    }
  }
  //check ssid name
  if(WiFi.SSID() == ssidLocal) {
    mode=1;
    M5.Lcd.println("\nConnected to WiFi!");
  M5.Lcd.print("IP Address: ");
  M5.Lcd.println(WiFi.localIP());
  }else if(WiFi.SSID() == ssid) {
    mode=0;
    M5.Lcd.println("\nConnected to WiFi!");
  M5.Lcd.print("IP Address: ");
  M5.Lcd.println(WiFi.localIP());
  } else {
    mode=2;
    M5.Lcd.println("NO KNOWN NETWORK");
  }
  
 

  // Once connected
  

  
  M5.Lcd.println("Searching env sensor");
  if (!sht3x.begin(&Wire, SHT3X_I2C_ADDR, 2, 1, 400000U)) {
         M5.Lcd.println("Not found");
         hasEnv=false;
    }
  M5.Lcd.println("Searching QMP6988");
    if (!qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, 2, 1, 400000U)) {        
        M5.Lcd.println("Not found");
         hasEnv=false;
         delay(1000);

    }
    if(hasEnv) {

    }
// Initialize MPU6886
  if (!M5.Imu.begin()) {
    M5.Lcd.println("IMU initialization failed!");
    //restart
    delay(2000);
    ESP.restart();
  }

}
float accelX_last = 0;
float accelY_last = 0;
float accelZ_last = 0;
unsigned long lastUpdate = 0;
float maxTotal=0;
unsigned long lastMeasure=0;
int deltaT=0;
int counter=0;
int maxDataLen=0;
int cursorY=0;
long lastEnv=0;
String envData="";




void loopTransport() {

  if(dataLen>maxDataLen) {
    maxDataLen=dataLen;
  }
  // Get accelerometer data
  float accelX, accelY, accelZ;
  if(counter++%1000==0) {
    
    M5.Lcd.fillRect(0, 80, 80, 40, BLACK);
    M5.Lcd.setCursor(0, 80);
    deltaT=millis()-lastMeasure;
    float rate=1000.0/(deltaT);
    lastMeasure=millis();
    M5.Lcd.println("Rate: "+String(rate,2)+ " kHz");
    M5.Lcd.println("t: "+String(deltaT,2)+ " ms");
    M5.Lcd.println("Buffer: "+String(dataLen)+" Max:"+String(100.*maxDataLen/MAX_DATA,2)+ "%");
    M5.Lcd.println(envData);
    maxDataLen=0;
  }
  
  M5.Imu.getAccelData(&accelX, &accelY, &accelZ);
  
  float total = sqrt(accelX * accelX + accelY * accelY + (accelZ) * (accelZ));
  
  //zero suppression, only report when the value changes more than 0.1
  if (abs(accelX_last - accelX) > 0.1 || abs(accelY_last - accelY) > 0.1 || abs(accelZ_last - accelZ) > 0.1
  || millis() - lastUpdate > 500 || millis() < lastUpdate || (
    (accelX > 2 && accelX != accelX_last)
    || (accelY > 2 && accelY != accelY_last)
    || (accelZ > 2 && accelZ != accelZ_last)
    ) ) {
    accelX_last = accelX;
    accelY_last = accelY;
    accelZ_last = accelZ;
    buffer[(dataStart+dataLen)%MAX_DATA].x=accelX;
    buffer[(dataStart+dataLen)%MAX_DATA].y=accelY;
    buffer[(dataStart+dataLen)%MAX_DATA].z=accelZ;
    buffer[(dataStart+dataLen)%MAX_DATA].t=millis();
    buffer[(dataStart+dataLen)%MAX_DATA].total=total;

    //find maximum over last 1000 updates
    // maxTotal=0;
    // for (int i=0; i<1000 && i <dataLen; i++) {
    //   if (buffer[(dataStart+dataLen-i)%MAX_DATA].total>maxTotal) {
    //     maxTotal=buffer[(dataStart+dataLen-i)%MAX_DATA].total;
    //   }
    // }
    //find maximum over last 1s, break after you go out of interval
    maxTotal=0;
    for (int i=0; i <dataStart+dataLen; i++) {
      if (buffer[(dataStart+dataLen-i)%MAX_DATA].t>millis()-1000) {
        if (buffer[(dataStart+dataLen-i)%MAX_DATA].total>maxTotal) {

          maxTotal=buffer[(dataStart+dataLen-i)%MAX_DATA].total;
        }
      } else {
        break;
      }
    }
    buffer[(dataStart+dataLen)%MAX_DATA].maxTotal=maxTotal;
    dataLen++;
    dataLen%=MAX_DATA;
    
  } else {
      //check wifi, reboot if lost
  if (WiFi.status() != WL_CONNECTED) {
    M5.Lcd.println("WiFi connection lost, rebooting...");
    delay(2000);
    ESP.restart();
  }
  // Establish connection to the server
  if (!client.connected()) {
    M5.Lcd.println("Connecting to server...");
    if (!client.connect(serverIP, serverPort)) {
      M5.Lcd.println("Connection failed!");
      delay(2000);
      return;
    }
    //clear the screen
    M5.Lcd.fillScreen(BLACK);
    //set to bottom
    M5.Lcd.setCursor(0, 120);
    M5.Lcd.println("Connected to server!");
  }
    //no change, take the chance to send one data
    if (dataLen>0) {
      String data = clientName+String(maxTotal, 2) + "  " +
              String(buffer[dataStart].maxTotal, 2) + "," +
              String(buffer[dataStart].t, 10) + "," +
              String(buffer[dataStart].x, 2) + "," + String(buffer[dataStart].y, 2) + "," + String(buffer[dataStart].z, 2) + "\n";
      // Send data to the server
      client.print(data);
      dataLen--;
      dataStart++;
      dataStart%=MAX_DATA;
    }
    if(hasEnv) {
        if (millis()-lastEnv>60000 || millis()<lastEnv || lastEnv==0) {
          if(sht3x.update() && qmp.update() )
          {
            //sent env data to server
            lastEnv=millis();
            envData = String(sht3x.cTemp, 1) + " " +
                String(sht3x.humidity, 0) + " " +
                String(qmp.cTemp, 1) + " " +
                String(qmp.pressure/100, 0) + " " +
                String(qmp.altitude, 0) + "\n";
            // Send data to the server
            client.print(String("HTP")+clientName+envData);
            } else {
              envData="Env data not ready";
            }
      
        }
        // Serial.print(sht3x.cTemp);
        // Serial.print(sht3x.humidity);
        // Serial.print(qmp.cTemp);
        // Serial.print(qmp.pressure);
        // Serial.print(qmp.altitude);

  }
    return;
  }
  
  // String data = clientName+String(accelX, 2) + "," + String(accelY, 2) + "," + String(accelZ, 2) + "\n";
  // // Send data to the server
  // client.print(data);

  // Display the sent data on screen
  // upadte LCD only at most every 0.3 seconds
  if (millis() - lastUpdate < 500 && lastUpdate < millis()) {
    return;
  }
  lastUpdate = millis();
  
  String data = clientName+String(maxTotal, 2) ;//+ "  " +String(buffer[(dataStart+dataLen-1)%MAX_DATA].t, 10) + "," +
              //String(buffer[(dataStart+dataLen-1)%MAX_DATA].x, 2) + "," + String(buffer[(dataStart+dataLen-1)%MAX_DATA].y, 2)
              // + "," + String(buffer[(dataStart+dataLen-1)%MAX_DATA].z, 2);
 
  M5.Lcd.fillRect(0, cursorY, 100, 15, BLACK);
  M5.Lcd.setCursor(0, cursorY);
  M5.Lcd.println(data);
  cursorY+=10;
  if (cursorY > 60) {
    cursorY = 0;
  }
  //wrap LCD
  // if (M5.Lcd.getCursorY() > 150) {
  //   M5.Lcd.fillScreen(BLACK);
  //   M5.Lcd.setCursor(0, 0);
  // }

}

void readEnv(){
   
    //show on screen temp and humidity
    //set background RED if hum > 50% green otherwise

    if(sht3x.update() && qmp.update() )
    {
      if (sht3x.humidity>50) {
        //M5.Lcd.clear();
        M5.Lcd.fillScreen(RED);

        M5.Lcd.fillRect(0, 40, 320, 40, RED);
        M5.Lcd.setTextColor(YELLOW);
      } else {
        //M5.Lcd.clear();
        M5.Lcd.fillScreen(GREEN);

        M5.Lcd.fillRect(0, 40, 320, 40, GREEN);
        M5.Lcd.setTextColor(BLACK);
      }
    }
    //print temperature and humidity
    //set larger font size
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 20);
    M5.Lcd.print("Temp:\n");
    M5.Lcd.print(sht3x.cTemp, 1);
    M5.Lcd.print(" C");
    M5.Lcd.setCursor(0, 70);
    M5.Lcd.print("Hum:\n");
    M5.Lcd.print(sht3x.humidity, 0);
    M5.Lcd.print("%");
    
}

void reportToMQTT() {

}

void loopLocal(){
   readEnv();
   delay(1000);
    if (millis() - lastUpdate < 60000 && lastUpdate < millis()) {
      return;
    }
    lastUpdate = millis();
   reportToMQTT();

}

void loopNoNetwork() {
     readEnv();
     delay(1000);
 
}


void loop() {
  if (mode==0) {
    loopTransport();
  } else if (mode==1) {
    loopLocal();
  } else {
    loopNoNetwork();
  }
}