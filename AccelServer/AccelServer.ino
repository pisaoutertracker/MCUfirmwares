#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <SD.h>
#include <SPI.h>
auto colorA = YELLOW;
auto colorB = RED;
// WiFi network configuration
const char* ssid = "M5Stack_AP";
const char* password = "12345678";
IPAddress local_ip({192,168,100,1});
IPAddress gateway=local_ip;
IPAddress subnet({255,255,255,0});
long lastLogTime = 0;
long xCoord = 0;
const int minY = 80;
const int maxY = 210;
const float minLog = 0.25;
const float maxLog = 16;
// Server for tcp connections
WiFiServer server(1234);

//http server with uri and header handling
#include <WebServer.h>
WebServer webServer(80);
String envData="no env data";
String envDataA="no env data";
String envDataB="no env data";

File logFile;
const int MAX_CLIENTS=2;
WiFiClient clients[MAX_CLIENTS] ;
String lastData[MAX_CLIENTS];
long int lastTime[MAX_CLIENTS];
String buffer;
int fileNumber = 0;
bool buttonApressed = false;
bool buttonBpressed = false;
bool buttonCpressed = false;
void setup() {
  M5.begin();
  Serial.begin(115200);
 if(buffer.reserve(5000)){
    Serial.println("Buffer reserved");
  } else
  {
    M5.Lcd.println("Failed to reserve buffer");
  }
  // Initialize the screen
  M5.Lcd.println("Starting...");

  // Initialize SD card
  if (!SD.begin()) {
    M5.Lcd.println("SD Card initialization failed!");
    delay(2000);
    ESP.restart();
  }
  // find the largest number for the file name and create data_NN+1.txt
  while (SD.exists("/data_" + String(fileNumber) + ".txt")) {
    fileNumber++;
  }
 logFile = SD.open("/data_" + String(fileNumber) + ".txt", FILE_WRITE);
 logFile.println("# starting at unknown time");
 logFile.flush();

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
  webServer.begin();
  webServer.on("/",  []() {
    //list files on SD card
    //html content

    File root = SD.open("/");
    String files = "";
    //send header
    

    while (File file = root.openNextFile()) {
      //http link
      files += "<a href='/";
      files += file.name();
      files += "'>";
      files += file.name();
      files += "</a> ";
      files += file.size();
      files += "<br>";
      file.close();
    }
   webServer.send(200, "text/html", files);

  });
  webServer.onNotFound( []() {
    String file = webServer.uri();
    File f = SD.open(file);
    if (f) {
      //read 10kb at time and send it
      while (f.available()) {
        char buffer[1024];
        int n = f.readBytes(buffer, 1024);
        webServer.sendContent(buffer, n);
      }
      
      
    } else {
      webServer.send(404, "text/plain", "File not found "+webServer.uri());
    }
   });
  // webServer.on("/current",  [](WebServer &server) {
  //   //send logFile
  //   if (logFile) {
  //     server.send(200, "text/plain", logFile.readString()+buffer);
  //   } else {
  //     server.send(404, "text/plain", "File not found");
  //   }

  // });
  // webServer.on("/status",  [](WebServer &server) {
  //   //send logFile
  //   String status = "File: " + String(fileNumber) + " Size: " + String(logFile.size()) + " Buffer: " + String(buffer.length());
  //   status+= " Clients: ";
  //   for (int i = 0; i < MAX_CLIENTS; i++) {
  //     if (clients[i]) {
  //       status += clients[i].remoteIP().toString() + " ";
  //     }
  //   }
  //   status+= " Last data: ";
  //   for (int i = 0; i < MAX_CLIENTS; i++) {
  //     if (lastTime[i]) {
  //       status += String((millis()-lastTime[i])/1000.) + " ";
  //       status+= lastData[i] + " ";
  //     }
  //   }
  //   server.send(200, "text/plain", status);
  // });
  M5.Lcd.println("Server started!");

 // add interrupt on buttons
  
}

void loop() {
  M5.update();
  webServer.handleClient();
  WiFiClient newClient = server.available();
  if (newClient) {
    Serial.println("New Client Connected.");
    bool added = false;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (!clients[i]) {
        clients[i] = newClient;
        clients[i].setTimeout(1000);
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
  logToScreen();
  if(M5.BtnC.wasPressed()){
    buttonApressed = false;
    Serial.println("Button C pressed");
    fileNumber++;
    logFile.println(buffer);
    logFile.flush();
    logFile.close();
    logFile = SD.open("/data_" + String(fileNumber) + ".txt", FILE_WRITE);
    if (!logFile) {
      M5.Lcd.println("Failed to open file for writing!");
    }
  }
  if(M5.BtnB.wasPressed()){
    buttonBpressed = false;
    logToSD(String(millis()) + " " + "Button B pressed");
    M5.Lcd.drawLine(xCoord,maxY,xCoord,minY,BLUE);
  }
  if(M5.BtnA.wasPressed()){
    buttonApressed = false;
    logToSD(String(millis()) + " " + "Button A pressed");
    M5.Lcd.drawLine(xCoord,maxY,xCoord,minY,GREEN);
  }
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] && clients[i].connected()) {
      if (clients[i].available()) {
        String line = clients[i].readStringUntil('\n');
        if(line.substring(0,3)=="HTP"){
          envData = line.substring(4);
          if(line.substring(0,4)=="HTPA")
            envDataA= envData;
           else
            envDataB=envData;
        } else {
          lastTime[i] = millis();
          lastData[i] = line;
        }
        //add millis() and IP of client to the data
        line = String(millis()) + " " + clients[i].remoteIP().toString() + " " + line;
      
        // Log data to SD card
        logToSD(line);
        // Display data on screen
        // Log data to Serial (for debugging)
        //Serial.println(line);
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
long lastSDTime = 0;
void logToSD(const String& data) {
  if (logFile) {
    if(data.length() + buffer.length() > 10000 or millis()-lastSDTime>30000 or millis()<lastSDTime){
      buffer+=data+"\n";
      logFile.print(buffer);
      logFile.flush();
      buffer="";
      lastSDTime=millis();
    //  M5.Lcd.println("SD");

    }
    else{
      buffer+=data+"\n";
    }
  } else {
    M5.Lcd.println("Failed to log data to SD card!");
  }
}



int logY(float y) {
  if(y<=0) return 210;
  return (int)(maxY - (maxY - minY) * ( (log(y) - log(minLog)) / ( log(maxLog) - log(minLog))));
}
int lastY[MAX_CLIENTS]={logY(1),logY(1)};
void logToScreen() {
  if(millis() - lastLogTime < 500 && millis() > lastLogTime){
    return;
  }
  lastLogTime = millis();
  
   if(xCoord > 300 || xCoord == 0){
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextSize(2);
      //Draw buttons
      //A: tagA
      //B: tagB
      //C: File+
      M5.Lcd.fillRect(0,maxY+1,320,40,BLACK);
      M5.Lcd.setTextColor(BLACK);
      M5.Lcd.fillRect(20,maxY+1,80,40,WHITE);
      M5.Lcd.setCursor(30,maxY+3);
      M5.Lcd.print("TagA");
      
      M5.Lcd.fillRect(120,maxY+1,80,40,WHITE);
      M5.Lcd.setCursor(130,maxY+3);
      M5.Lcd.print("TagB");
      //draw circle green/blue next to tagA and B
      M5.Lcd.fillCircle(90,maxY+10,3,GREEN);
      M5.Lcd.fillCircle(190,maxY+10,3,BLUE);

      M5.Lcd.fillRect(220,maxY+1,80,40,WHITE);
      M5.Lcd.setCursor(230,maxY+3);
      M5.Lcd.print("File+");
      M5.Lcd.setTextSize(1);
      // //Draw white line a 2g and 4g
      // M5.Lcd.drawLine(0, 240 - 2*30, 320, 240 - 2*30, WHITE);
      // M5.Lcd.drawLine(0, 240 - 4*30, 320, 240 - 4*30, WHITE);
      // M5.Lcd.drawLine(0, 240 - 6*30, 320, 240 - 6*30, WHITE);
      // //write on the right side "2g", "4g", "6g"
      // M5.Lcd.setCursor(300, 240 - 2*30);
      // M5.Lcd.print("2g");
      // M5.Lcd.setCursor(300, 240 - 4*30);
      // M5.Lcd.print("4g");
      // M5.Lcd.setCursor(300, 240 - 6*30);
      // M5.Lcd.print("6g");
      //Draw log scale with lines at 0.25, 1, 4, 16

      M5.Lcd.drawLine(0, logY(0.25), 320, logY(0.25), WHITE);
      M5.Lcd.drawLine(0, logY(1), 320, logY(1), WHITE);
      M5.Lcd.drawLine(0, logY(4), 320, logY(4), WHITE);
      M5.Lcd.drawLine(0, logY(16), 320, logY(16), WHITE);
      //write on the right side "0.25g", "1g", "4g", "16g"
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(300, logY(0.25));
      M5.Lcd.print("g/4");
      M5.Lcd.setCursor(300, logY(1));
      M5.Lcd.print("1g");
      M5.Lcd.setCursor(300, logY(4));
      M5.Lcd.print("4g");
      M5.Lcd.setCursor(300, logY(16));
      M5.Lcd.print("16g");

      
      //Draw Bounding box for the graph using min max
      M5.Lcd.drawLine(0, minY, 320, minY, WHITE);

     
      //Draw ticks on X axis every 30 seconds
      for(int i=0;i<5;i++){
        M5.Lcd.drawLine(60*i, maxY, 60*i, maxY-5, WHITE);
      }


      
      xCoord = 0;
    }

  
  //print time since last SD write
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.fillRect(180,0,140,40,BLACK);
  M5.Lcd.setCursor(200,0);
  M5.Lcd.print(int((millis()-lastSDTime)/1000.));
  M5.Lcd.print("s ");
  M5.Lcd.setCursor(250,0);
  M5.Lcd.print(buffer.length()/1000);
  M5.Lcd.print("k");
  M5.Lcd.setCursor(180,20);
  M5.Lcd.print("#");
  M5.Lcd.print(fileNumber);
  M5.Lcd.print(" ");
  M5.Lcd.setCursor(0,40);
  M5.Lcd.fillRect(0,40,320,20,BLACK);
  M5.Lcd.setTextColor(colorA);
  M5.Lcd.print(envDataA);
  M5.Lcd.setCursor(0,60);
  M5.Lcd.fillRect(0,60,320,20,BLACK);
  M5.Lcd.setTextColor(colorB);
  M5.Lcd.print(envDataB);
  M5.Lcd.setTextColor(WHITE);
  
  M5.Lcd.setCursor(250,20);
  if(logFile && logFile.size() > 0 ) {
    M5.Lcd.print(logFile.size()/1e6);  
    M5.Lcd.print("M");
  } else {
    M5.Lcd.print("empty");
  }
  for(int i=0;i<MAX_CLIENTS;i++){
    //print first 5 chars of the data
    //parse data "A 10.3 1234567890,1.23,4.56,7.89"
    
    float total = lastData[i].substring(2,7).toFloat();
    bool isA = (lastData[i].substring(0,1)=="A");
    //write data in a fixed position
    //increase font size
    if(isA) M5.Lcd.setTextColor(colorA);
    else M5.Lcd.setTextColor(colorB);
    if(lastTime[i] < millis()-3000) {
      //set background color to RED if the data is old
      M5.Lcd.fillRect(i*100,0,80,40,RED);
      M5.Lcd.setCursor(i*100,20);
      M5.Lcd.setTextColor(WHITE);

      M5.Lcd.println((millis()-lastTime[i])/1000.);
    } else {
      M5.Lcd.fillRect(i*100,0,80,40,BLACK);
      M5.Lcd.setCursor(i*100,20);
      M5.Lcd.print(isA?"  A ":" B ");
    }
    M5.Lcd.setCursor(i*100,0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print(total);
    M5.Lcd.print("g");
    //Draw graph
    //M5.Lcd.drawLine(xCoord+i*160, 240 , xCoord+i*160, 240 - total*50, (i>0)?GREEN:RED);
    //Draw points
    int y=240 - total*30;
    
    //M5.Lcd.drawPixel(xCoord+i,y, (i>0)?GREEN:RED);
    if(i==0)  xCoord+=2;
    //M5.Lcd.drawLine(xCoord-1, lastY[i] , xCoord, y, (isA)?colorA:colorB);
    // use log
    M5.Lcd.drawLine(xCoord-1, lastY[i] , xCoord, logY(total), (isA)?colorA:colorB);
    lastY[i]=logY(total);
   // lastY[i]=y;
   
  //M5.Lcd.println();
  // Clear the screen if it's full
}
}
