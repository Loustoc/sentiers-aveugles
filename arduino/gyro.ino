// Bluetooth setup adapted from https://github.com/ugmurthy/CraftsmanBLE

#include "M5StickCPlus.h"
#include <OSCMessage.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "EasyButton.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

void dumpBLE(char* msg);
String blename;

#define BTN_A 37
#define BTN_B 39

int press_duration = 2000;
EasyButton Button_A(BTN_A,40);
EasyButton Button_B(BTN_B,40);
bool in_loop=false;

char* ssid;
char* password;
int oscPort;
static IPAddress hostIp;

bool oscPaused = false;

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
char message[100];

bool wifiOn = false;
bool oscOn = false;
char* stringIP;
int wifiCode = 0;
int step = 0;

float angleZ = 0.0;
unsigned long lastTime = millis();
WiFiUDP UDP;

void on_B_Pressed();
void on_A_Pressed();

#define SERVICE_UUID           "146DD431-38DE-4753-A83D-3F5B9F33A0FE" // UART service UUID
#define CHARACTERISTIC_UUID_RX "146DD431-38DE-4753-A83D-3F5B9F33A0FE"
#define CHARACTERISTIC_UUID_TX "146DD431-38DE-4753-A83D-3F5B9F33A0FE"

// on connect/disconnect callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE Connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE Disconnected");
    }
};

void connectWifi() {
 Serial.println("ssid :");
 Serial.println(ssid);
 Serial.println("password: ");
 Serial.println(password);
 WiFi.mode(WIFI_STA);
 WiFi.begin(ssid, password);
   int tryDelay = 500;
   int numberOfTries = 20;
    
    // Wait for the WiFi event
    while (true) {
        switch(WiFi.status()) {
          case WL_NO_SSID_AVAIL:
            Serial.println("[WiFi] SSID not found");
            wifiCode = 1;
            break;
          case WL_CONNECT_FAILED:
            Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
            wifiCode = 1;
            break;
          case WL_CONNECTION_LOST:
            Serial.println("[WiFi] Connection was lost");
            break;
          case WL_SCAN_COMPLETED:
            Serial.println("[WiFi] Scan is completed");
            break;
          case WL_DISCONNECTED:
            Serial.println("[WiFi] WiFi is disconnected");
            break;
          case WL_CONNECTED:
            Serial.println("[WiFi] WiFi is connected!");
            Serial.print("[WiFi] IP address: ");
            Serial.println(WiFi.localIP());
            wifiOn = true;
            break;
          default:
            Serial.print("[WiFi] WiFi Status: ");
            Serial.println(WiFi.status());
            break;
        }
       
        delay(tryDelay);
        
        if(numberOfTries <= 0){
          Serial.print("[WiFi] Failed to connect to WiFi!");
          // Use disconnect function to force stop trying to connect
          WiFi.disconnect();
          return;
        } else {
          numberOfTries--;
        }
    }
}

void trim(char *s) {
    char *d = s;
    // while leading spaces are found, move the pointer to the next char
    while (*d == ' ') {
        ++d;
    }
    // check if the cleaned string and the original are identical
    char *start = s;
    while (*s++ = *d++) {
    }

    // clean trailing spaces
    s -= 2;
    while (s >= start && (*s == ' ' || *s == '\n')) {
        *s-- = '\0';
    }
}

void splitAndCopy(char* ipString, uint32_t* ipArray, int size) {
    char* token = strtok(ipString, ".");
    int i = 0;

    while (token != nullptr && i < size) {
        ipArray[i++] = static_cast<uint32_t>(std::stoi(token));
        token = strtok(nullptr, ".");
    }
}


void copyCharPointer(char*& destination, const char* source) {
    if (destination) {
        delete[] destination;
    }
    size_t length = strlen(source);
    destination = new char[length + 1]; // +1 -> null terminator
    strcpy(destination, source); 
}


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      std::string cmd;
      std::string argstr;
     
      cmd = rxValue.substr(0,1);
      argstr = rxValue.substr(1,3);
      
      switch (step) {
        case 0:
          trim(&rxValue[0]);
          copyCharPointer(ssid, &rxValue[0]);
          Serial.println(ssid);
          break;
        case 1:
          trim(&rxValue[0]);
          copyCharPointer(password, &rxValue[0]);
          Serial.println(password);
          connectWifi();
          break;
        case 2: {
            trim(&rxValue[0]);
            copyCharPointer(stringIP, &rxValue[0]);

            char* ipCopy = new char[strlen(stringIP) + 1];
            strcpy(ipCopy, stringIP);

            uint32_t ipArray[4]; 

            splitAndCopy(ipCopy, ipArray, 4);

            Serial.print("IP Address Parts: {");
            for (int i = 0; i < 4; ++i) {
                Serial.print(ipArray[i]);
                if (i < 3) Serial.print(", ");
            }
            Serial.println("}");

            hostIp = IPAddress(ipArray[0], ipArray[1], ipArray[2], ipArray[3]);
            delete[] ipCopy;
            break;
        }
        case 3: {
          char* tempPort = nullptr;
          trim(&rxValue[0]);
          tempPort = &rxValue[0];
          oscPort = std::stoi(tempPort);
          Serial.println(&rxValue[0]);
          oscOn = true;
          break;
         } 
        default:
          break;
       }
       step++;
    }
};

void on_A_pressedFor(){
  Serial.println("Resetting config");
  wifiCode = 0;
  M5.Lcd.fillScreen(BLACK);
  wifiOn = false;
  oscOn = false;
  step = 0;
  }

void on_A_Pressed() {
  Serial.println("A pressed !");
  oscPaused = !oscPaused;
}

void on_B_Pressed() {
   Serial.println("B pressed !");
}


void setup() {
    Serial.begin(115200);
    
    Button_A.begin();
    Button_A.onPressed(on_A_Pressed);
    Button_A.onPressedFor(press_duration,on_A_pressedFor);

    M5.begin();
    M5.IMU.Init(); // Initialize IMU
    uint64_t chipid = ESP.getEfuseMac();
    blename = "SentiersAveugles-Gyro-"+String((uint32_t)(chipid>>32),HEX);
    BLEDevice::init(blename.c_str());
  
    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
  
    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);
  
    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                        
    pTxCharacteristic->addDescriptor(new BLE2902());
  
    BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                         CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  
    pRxCharacteristic->setCallbacks(new MyCallbacks());
  
    // Start the service
    pService->start();
  
    // Start advertising
    pServer->getAdvertising()->start();
    Serial.println("Waiting for a client connection to send Acc data...");
    
}

float readGyroData() {
    float gyroX, gyroY, gyroZ;
    M5.IMU.getGyroData(&gyroX, &gyroY, &gyroZ);

    unsigned long currentTime = millis();
    float dt = (currentTime - lastTime) / 1000.0;
    lastTime = currentTime;

    angleZ += gyroZ * dt;

    angleZ = fmod(angleZ, 360.0);
    if (angleZ < 0) {
        angleZ += 360.0;
    }
    
    //Serial.print("Angle Z: ");
    //Serial.println(angleZ);
    return angleZ;
}

void dispBattery(){
   uint16_t vbatData = M5.Axp.GetVbatData();
   double vbat = vbatData * 1.1 / 1000;
   M5.Lcd.setCursor(10,10);
   M5.Lcd.printf("%f",100.0 * ((vbat - 3.0) / (4.07 - 3.0)));
  }

void loop() {
    dispBattery();
    M5.Lcd.setCursor(10,40);
    if (wifiOn) {
      M5.Lcd.printf("Connected to : %s",ssid);
    }
    else {
      M5.Lcd.printf("WiFi disconnected");
    }
    M5.Lcd.setCursor(10,80);
    if (wifiCode > 0)  M5.Lcd.printf("An error occured when attempting to connect. Wrong SSID/PWD ?");
    if (oscOn) {
      Serial.println(stringIP);
      M5.Lcd.printf("Sending to : %s, Port : %d", stringIP, oscPort);
    }
    else {
      M5.Lcd.printf("OSC Host undefined");
    }
    Button_A.read();
    Button_B.read();
    if (oscOn && wifiOn && !oscPaused) {
      OSCMessage* message = new OSCMessage("/gyro");
      M5.Lcd.setCursor(10,100);
      float gyroVal = readGyroData();
      M5.Lcd.printf("%f",gyroVal);
      message->add(gyroVal);
      UDP.beginPacket(hostIp, oscPort);
      message->send(UDP);
      UDP.endPacket();
      delete(message);
    }
        // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
          delay(500); // give the bluetooth stack the chance to get things ready
          pServer->startAdvertising(); // restart advertising
          Serial.println("start advertising");
          oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    delay(2);
}