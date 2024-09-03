#include <M5Unified.h>
#include <WiFi.h>

// WiFi network configuration
const char* ssid = "M5Stack_AP";
const char* password = "12345678";
const char* serverIP = "192.168.100.1";
const uint16_t serverPort = 80;
String clientName="B ";
// WiFi client
WiFiClient client;

struct Data {
  float x,y,z;
  unsigned long t;
};
const int MAX_DATA=10000;
Data buffer[10000];
int dataStart=0;
int dataLen=0;

void setup() {
  M5.begin();
  Serial.begin(115200);

  // Initialize the screen
  M5.Lcd.println("Connecting to WiFi...");

  // Connect to the Wi-Fi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5.Lcd.print(".");
  }

  // Once connected
  M5.Lcd.println("\nConnected to WiFi!");
  M5.Lcd.print("IP Address: ");
  M5.Lcd.println(WiFi.localIP());

  // Initialize MPU6886
  // use maximum g-range

  if (!M5.Imu.begin()) {
    M5.Lcd.println("IMU initialization failed!");
    while (1) {
      delay(1000);
    }
  }
//  M5.Imu.setAccelFsr(MPU6886_ACCEL_RANGE_16G);

  // if (!M5.Imu.begin()) {
  //   M5.Lcd.println("IMU initialization failed!");
  //   while (1) {
  //     delay(1000);
  //   }
  // }
}
float accelX_last = 0;
float accelY_last = 0;
float accelZ_last = 0;
unsigned long lastUpdate = 0;
void loop() {
  // Establish connection to the server
  if (!client.connected()) {
    M5.Lcd.println("Connecting to server...");
    if (!client.connect(serverIP, serverPort)) {
      M5.Lcd.println("Connection failed!");
      delay(2000);
      return;
    }
    M5.Lcd.println("Connected to server!");
  }

  // Get accelerometer data
  float accelX, accelY, accelZ;
  M5.Imu.getAccelData(&accelX, &accelY, &accelZ);
  //zero suppression, only report when the value changes more than 0.1
  if (abs(accelX_last - accelX) > 0.1 || abs(accelY_last - accelY) > 0.1 || abs(accelZ_last - accelZ) > 0.1) {
    accelX_last = accelX;
    accelY_last = accelY;
    accelZ_last = accelZ;
    buffer[(dataStart+dataLen)%MAX_DATA].x=accelX;
    buffer[(dataStart+dataLen)%MAX_DATA].y=accelY;
    buffer[(dataStart+dataLen)%MAX_DATA].z=accelZ;
    buffer[(dataStart+dataLen)%MAX_DATA].t=millis();
    dataLen++;
    dataLen%=MAX_DATA;
    
  } else {
    //no change, take the chance to send one data
    if (dataLen>0) {
      String data = clientName+String(buffer[dataStart].t, 10) + "," +
              String(buffer[dataStart].x, 2) + "," + String(buffer[dataStart].y, 2) + "," + String(buffer[dataStart].z, 2) + "\n";
      // Send data to the server
      client.print(data);
      dataLen--;
      dataStart++;
      dataStart%=MAX_DATA;
    }
    return;
  }
  
  // String data = clientName+String(accelX, 2) + "," + String(accelY, 2) + "," + String(accelZ, 2) + "\n";
  // // Send data to the server
  // client.print(data);

  // Display the sent data on screen
  // upadte LCD only at most every 0.3 seconds
  if (millis() - lastUpdate < 500 || lastUpdate > millis()) {
    return;
  }
  
  String data = clientName+String(buffer[(dataStart+dataLen-1)%MAX_DATA].t, 10) + "," +
              String(buffer[(dataStart+dataLen-1)%MAX_DATA].x, 2) + "," + String(buffer[(dataStart+dataLen-1)%MAX_DATA].y, 2)
               + "," + String(buffer[(dataStart+dataLen-1)%MAX_DATA].z, 2);
 
  M5.Lcd.println(data);
  //wrap LCD
  if (M5.Lcd.getCursorY() > 150) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
  }

}