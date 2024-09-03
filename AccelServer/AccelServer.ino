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
const int MAX_CLIENTS=3;
WiFiClient clients[MAX_CLIENTS] ;

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
      Serial.println("Max clients reached, rejecting new client.");
      newClient.stop();
    }
  }

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] && clients[i].connected()) {
      if (clients[i].available()) {
        String line = clients[i].readStringUntil('\n');
        //add millis() and IP of client to the data
        logToScreen(line);
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

void logToScreen(const String& data) {
  
  // Clear the screen if it's full
  if (M5.Lcd.getCursorY() > 200) {
    M5.Lcd.fillScreen(BLACK);
    //set cursor to top left corner
    M5.Lcd.setCursor(0, 0);
  }
  M5.Lcd.println(data);
}