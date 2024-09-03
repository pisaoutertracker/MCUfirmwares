#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <SD.h>
#include <SPI.h>

// WiFi network configuration
const char* ssid = "M5Stack_AP";
const char* password = "12345678";
IPAddress local_ip({192,168,100,1});
IPAddress gateway=local_ip;
IPAddress subnet({255,255,255,0});

// Server configuration
WiFiServer server(80);
File logFile;
const int MAX_CLIENTS=2;
WiFiClient clients[MAX_CLIENTS] ;
String lastData[MAX_CLIENTS];
long int lastTime[MAX_CLIENTS];
void setup() {
  M5.begin();
  Serial.begin(115200);

  // Initialize the screen
  M5.Lcd.println("Starting...");

  // Initialize SD card
  if (!SD.begin()) {
    M5.Lcd.println("SD Card initialization failed!");
    while (1);
  }
  // find the largest number for the file name and create data_NN+1.txt
  int fileNumber = 0;
  while (SD.exists("/data_" + String(fileNumber) + ".txt")) {
    fileNumber++;
  }
 logFile = SD.open("/data_" + String(fileNumber) + ".txt", FILE_WRITE);

  if (!logFile) {
    M5.Lcd.println("Failed to open file for writing!");
  }

  // Set up Wi-Fi access point
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password);

  M5.Lcd.println("Access point set up!");
  M5.Lcd.print("SSID: ");
  M5.Lcd.println(ssid);
  M5.Lcd.print("Password: ");
  M5.Lcd.println(password);
  M5.Lcd.print("IP address: ");
  M5.Lcd.println(WiFi.softAPIP());

  // Start the server
  server.begin();
  M5.Lcd.println("Server started!");
}

void loop() {
  WiFiClient newClient = server.available();
  if (newClient) {
    Serial.println("New Client Connected.");
    bool added = false;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (!clients[i]) {
        clients[i] = newClient;
        added = true;
        break;
      }
    }
    if (!added) {
      Serial.println("Max clients reached, restarting.");
      newClient.stop();
      for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].stop(); 
      }

    //  ESP.restart();

    }
  }

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] && clients[i].connected()) {
      if (clients[i].available()) {
        String line = clients[i].readStringUntil('\n');
        lastTime[i] = millis();
        lastData[i] = line;
        //add millis() and IP of client to the data
        logToScreen();
        line = String(millis()) + " " + clients[i].remoteIP().toString() + " " + line;
        // Log data to SD card
        logToSD(line);
        // Display data on screen
        // Log data to Serial (for debugging)
        Serial.println(line);
      }
    } else {
      if (clients[i]) {
        clients[i].stop();
        clients[i] = NULL;
        // move up the other clients
        for (int j = i + 1; j < MAX_CLIENTS; j++) {
          clients[j - 1] = clients[j];
          clients[j] = NULL;
        }
        Serial.println("Client Disconnected.");
      }
    }
  }
  // WiFiClient client = server.available();

  // if (client) {
  //   Serial.println("New Client Connected.");
  //   while (client.connected()) {
  //     if (client.available()) {
  //       String line = client.readStringUntil('\n');
  //       //add millis() and IP of client to the data
  //       logToScreen(line);
  //       line = String(millis()) + " " + client.remoteIP().toString() + " " + line;
  //       // Log data to SD card
  //       logToSD(line);
  //       // Display data on screen
  //       // Log data to Serial (for debugging)
  //       Serial.println(line);
  //     }
  //   }
  //   client.stop();
  //   Serial.println("Client Disconnected.");
  // }

 

 

}

void logToSD(const String& data) {
  if (logFile) {
    logFile.println(data);
    logFile.flush();
  } else {
    M5.Lcd.println("Failed to log data to SD card!");
  }
}
long lastLogTime = 0;
long xCoord = 0;
int lastY[MAX_CLIENTS]={210,210};
void logToScreen() {
  if(millis() - lastLogTime < 500 && millis() > lastLogTime){
    return;
  }
  lastLogTime = millis();
  
   if(xCoord > 300 || xCoord == 0){
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextSize(1);

      //Draw white line a 2g and 4g
      M5.Lcd.drawLine(0, 240 - 2*30, 320, 240 - 2*30, WHITE);
      M5.Lcd.drawLine(0, 240 - 4*30, 320, 240 - 4*30, WHITE);
      M5.Lcd.drawLine(0, 240 - 6*30, 320, 240 - 6*30, WHITE);
      //write on the right side "2g", "4g", "6g"
      M5.Lcd.setCursor(300, 240 - 2*30);
      M5.Lcd.print("2g");
      M5.Lcd.setCursor(300, 240 - 4*30);
      M5.Lcd.print("4g");
      M5.Lcd.setCursor(300, 240 - 6*30);
      M5.Lcd.print("6g");
      
      //Draw Bounding box for the graph
      M5.Lcd.drawRect(1, 40, 319, 199, WHITE);
      //Draw ticks on X axis every 30 seconds
      for(int i=0;i<5;i++){
        M5.Lcd.drawLine(60*i, 238, 60*i, 236, WHITE);
      }


      
      xCoord = 0;
    }

  

  for(int i=0;i<MAX_CLIENTS;i++){
    //print first 5 chars of the data
    //parse data "A 10.3 1234567890,1.23,4.56,7.89"
    float total = lastData[i].substring(2,7).toFloat();
    bool isA = (lastData[i].substring(0,1)=="A");
    //write data in a fixed position
    //increase font size
    if(isA) M5.Lcd.setTextColor(YELLOW);
    else M5.Lcd.setTextColor(BLUE);
    if(lastTime[i] < millis()-3000) {
      //set background color to RED if the data is old
      M5.Lcd.fillRect(i*160,0,160,40,RED);
      M5.Lcd.setCursor(i*160,20);

      M5.Lcd.println((millis()-lastTime[i])/1000.);
    } else {
      M5.Lcd.fillRect(i*160,0,160,40,BLACK);
    }
    M5.Lcd.setCursor(i*160,0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print(total);
    M5.Lcd.print("g");
    //Draw graph
    //M5.Lcd.drawLine(xCoord+i*160, 240 , xCoord+i*160, 240 - total*50, (i>0)?GREEN:RED);
    //Draw points
    int y=240 - total*30;
    
    //M5.Lcd.drawPixel(xCoord+i,y, (i>0)?GREEN:RED);
    if(i==0)  xCoord+=2;
    M5.Lcd.drawLine(xCoord-1, lastY[i] , xCoord, y, (isA)?YELLOW:BLUE);
    lastY[i]=y;
   
  //M5.Lcd.println();
  // Clear the screen if it's full
}
}