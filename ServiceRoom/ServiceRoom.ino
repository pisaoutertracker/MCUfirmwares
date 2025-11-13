#include <M5Unified.h>
#include <WiFi.h>
#include "PubSubClient.h"
//#include "Free_Fonts.h"  // Comment out if causing issues with M5Unified

// WiFi network configuration
const char* ssid = "M5Stack_AP";
const char* password = "12345678";
const char* serverIP = "192.168.100.1";
const uint16_t serverPort = 1234;

// WiFi network configuration
const char* ssidLocal = "CMSPisa";
const char* passwordLocal = "silicon2003";

// Wifi network testing network
const char* ssidTest = "tbtc";
const char* passwordTest = "pippo345";

String clientName="B ";
// WiFi client
WiFiClient client;
#include "M5UnitENV.h"
SHT3X sht3x;
QMP6988 qmp;
PubSubClient clientMQTT;
float outSidePressure=0;
long long int lastPressure=0;
float calibrationOffset=0;
//G2
int pixelPin=G2;
int outerPin=G38;
int lastPixel=0;
int lastOuter=0;

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
  //connect M5 pushbutton to buttonPressed() function
  //M5.BtnA.setPressedHandler(buttonPressed);

  Serial.print(clientName);

  // Initialize the screen
  M5.Lcd.println("Connecting to WiFi...");

  // Connect to the Wi-Fi network
  int counter=0;
  WiFi.begin(ssid, password);
    delay(1000);

  while (WiFi.status() != WL_CONNECTED and counter++ < 3) {
    WiFi.begin(ssid, password);
    delay(2000);
    M5.Lcd.print(".");
    if (WiFi.status() != WL_CONNECTED) {
       //try local network
      WiFi.begin(ssidLocal, passwordLocal);
      delay(2000);
      M5.Lcd.print("L");
      if (WiFi.status() != WL_CONNECTED) {
        //try test network
        WiFi.begin(ssidTest, passwordTest);
        delay(2000);
        M5.Lcd.print("T");
      }
    }
  }
  //check ssid name
  if(WiFi.SSID() == ssidLocal) {
    mode=1;
    clientName="MAC"+WiFi.macAddress();
    M5.Lcd.println("\nConnected to WiFi!");
  M5.Lcd.print("IP Address: ");
  M5.Lcd.println(WiFi.localIP());
  } else if(WiFi.SSID() == ssidTest) {
    mode=2;
    M5.Lcd.println("\nConnected to WiFi!");
  }else {
    mode=-1;
    M5.Lcd.println("NO KNOWN NETWORK");
  }
  
  if(mode>0) {
    M5.Lcd.println("Connecting to MQTT...");
    clientMQTT.setClient(client);
//    clientMQTT.setMqttClientName("S3carrier");
    if(mode==1) {
        clientMQTT.setServer("192.168.0.45",1883);
        clientMQTT.setCallback(callback); 

        Serial.println("cmspisa");
        reconnect();


    } else {
        clientMQTT.setServer("test.mosquitto.org",1883);
        clientMQTT.setCallback(callback); 

        Serial.println("mosquitto");
        reconnect();


    }
   // clientMQTT.setCallback(callback); 

  
  }
 

  // Once connected
  
hasEnv=false;
/*  
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
    */
  //set input on the pins and pullup
  pinMode(pixelPin, INPUT_PULLUP);
  pinMode(outerPin, INPUT_PULLUP);

}

unsigned long lastUpdate = 0;


void callback(char* topic, byte* payload, unsigned int length) {
  //parse json message {"dewpoint":7.8,"temperature":23.2,"RH":37,"Pressure":1020}
  // get pressure

  // String message="";
  // for (int i=0;i<length;i++) {
  //   message+=(char)payload[i];
  // }
  // Serial.println(message);

  // int pressureIndex = message.indexOf("Pressure");
  // int numberIndex = message.indexOf(":",pressureIndex);
  // int commaIndex = message.indexOf(",",numberIndex);
  // int endIndex = message.indexOf("}",numberIndex);
  // int endPressure = (commaIndex>0)?commaIndex:endIndex;
  // String pressure = message.substring(numberIndex+1,endPressure);
  // outSidePressure = pressure.toFloat(); 
  // lastPressure= millis();
  
  
}

void reconnect(){
  if (clientMQTT.connect("S3Atom")) {
    Serial.println("connected");
    // //subscribe topic for control
    // clientMQTT.subscribe("/environment/HumAndTemp001");
    // Serial.println("subscribed");
  } else {
    Serial.println("Failed connecting");
  }

    
}

// void readEnv(){
   
//     //show on screen temp and humidity
//     //set background RED if hum > 50% green otherwise

//     if(sht3x.update() && qmp.update() )
//     {
//       if (sht3x.humidity>50) {
//         //M5.Lcd.clear();
//         M5.Lcd.fillScreen(RED);

//         M5.Lcd.fillRect(0, 40, 320, 40, RED);
//         M5.Lcd.setTextColor(YELLOW);
//       } else {
//         //M5.Lcd.clear();
//         if(mode==1 and qmp.pressure >  outSidePressure + calibrationOffset + 5)
//         {
//           M5.Lcd.fillScreen(GREEN);
//           M5.Lcd.fillRect(0, 40, 320, 40, GREEN);
//         } else {
//           M5.Lcd.fillScreen(YELLOW);
//           M5.Lcd.fillRect(0, 40, 320, 40, YELLOW);
//         }

//         M5.Lcd.setTextColor(BLACK);
//       }
//     }
//     //print temperature and humidity
//     //set larger font size
//     M5.Lcd.setTextSize(1);
//     M5.Lcd.setFreeFont(FSSB12);
//     M5.Lcd.setCursor(0, 0);
//     M5.Lcd.print("T:");
//     M5.Lcd.print(sht3x.cTemp, 1);
//     M5.Lcd.print(" C\n");
//     M5.Lcd.print("H:");
//     M5.Lcd.print(sht3x.humidity, 0);
//     M5.Lcd.print("%\n");
//     M5.Lcd.print("Dew:");
//     M5.Lcd.print(get_dew_point_c(sht3x.cTemp,sht3x.humidity), 1);
//     M5.Lcd.print("\n");
//     M5.Lcd.setFreeFont(FSSB9);
//     M5.Lcd.print("DP:");
//     M5.Lcd.print(qmp.pressure-outSidePressure-calibrationOffset, 0);
//     M5.Lcd.print("\n");
//     M5.Lcd.print("Lastupd:");
//     M5.Lcd.print((millis() - lastUpdate)/1000 );
//     M5.Lcd.print("s\n");
    
// }
void onConnectionEstablished()
{
  Serial.println("Connesso"); 
}

// float get_dew_point_c(float t_air_c, float rel_humidity) {
//   float A = 17.27;
//   float B = 237.7;
//   float alpha = ((A * t_air_c) / (B + t_air_c)) + log(rel_humidity/100.0);
//   return (B * alpha) / (A - alpha);
// }

void reportToMQTT() {

   if (clientMQTT.connected()) {
      Serial.println("connected");
      auto topic=String("/serviceroom/status");
      //format          message= '{"dewpoint": %.1f, "temperature":%.1f,"RH":%.0f,"Pressure":%.0f}'%(dew_point,t,rh,pressure)
//      auto message=String("{temp=")+String(sht3x.cTemp, 1)+String(",humidity=")+String(sht3x.humidity, 0)+String(",pressure=")+String(qmp.pressure/100, 0)+String(",altitude=")+String(qmp.altitude, 0)+String("}");
      // auto message=String("{\"dewpoint\":")+
      //              String(get_dew_point_c(sht3x.cTemp,sht3x.humidity), 1)+
      //              String(",\"temperature\":")+String(sht3x.cTemp, 1)+
      //              String(",\"RH\":")+String(sht3x.humidity, 0)+
      //              String(",\"Pressure\":")+String(qmp.pressure, 0)+
      //              String("}");
      auto message=String("{\"pixel\":")+
                   String(lastPixel)+
                    String(",\"outer\":")+String(lastOuter)+
                    String("}");
      clientMQTT.publish(topic.c_str(),message.c_str());
      Serial.println("Reporting");
      Serial.println(message);
   }else{
    Serial.println("NOT CONNECTED");
   }
}
void updateScreen() {
    //clear pixel and outer area
    M5.Lcd.fillRect(0, 0, 320, 40, BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.print("Pixel:");
    M5.Lcd.print(lastPixel);
    M5.Lcd.print(" Outer:");
    M5.Lcd.print(lastOuter);
}

void loopLocal(){

   //readEnv();
   M5.update();
  //  if(M5.BtnA.wasReleased()){   
  //     // Restart the device
  //     Serial.println("Button pressed");
  //     //calibrate pressure
  //     calibrationOffset=qmp.pressure-outSidePressure;
  //     Serial.println(calibrationOffset);
  //     Serial.println(qmp.pressure);
  //     Serial.println(outSidePressure);
  //   }
   //delay(30);
   
   if (!clientMQTT.connected()) {
    reconnect();
   }
   clientMQTT.loop();
   //report on change maximum every 500ms
   if( (lastPixel != digitalRead(pixelPin) || lastOuter != digitalRead(outerPin)) &&  (millis() - lastUpdate > 50 || lastUpdate > millis()) ) {
          reportToMQTT();
          lastPixel=digitalRead(pixelPin);
          lastOuter=digitalRead(outerPin); 
          lastUpdate = millis();  
          updateScreen();
   }
  //report every minute
   if (millis() - lastUpdate > 5000 || lastUpdate > millis()) {
          lastPixel=digitalRead(pixelPin);
          lastOuter=digitalRead(outerPin);
          reportToMQTT();
          lastUpdate = millis();
          updateScreen();
   }
   
  delay(100);
  Serial.print(digitalRead(pixelPin));
  Serial.print(" <-Out | Pix-> ");
  Serial.println(digitalRead(outerPin));
}

void loopNoNetwork() {
     if(millis() > 180*1000) ESP.restart(); //max three minutes in local

     //readEnv();
     delay(1000);
 
}


void loop() {
  if (mode>0) {
    loopLocal();
  } else {
    loopNoNetwork();
  }
}
