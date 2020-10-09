/* instrukce
 *  AUTÍČKO / OVLADAČ
 *  v TFT_eSPI-master/User_Setup_Select.h používám #include <User_Setups/Setup11_RPi_touch_ILI9486.h>
 *  vývojová deska ESP32 Pico Kit
 *  
 */

/*
Based on:
Neil Kolban (Bluetooth Low Energy communication)
https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp

Eric 0015 (Bluetooth Low Energy communication)
https://github.com/0015/IdeasNProjects/tree/master/Esp32_BLE_to_BLE

Luboš M. (Bluetooth Low Energy communication)
https://navody.arduino-shop.cz/navody-k-produktum/esp32-a-bluetooth-low-energy-ble.html
*/

#include <SPI.h>

#include <TFT_eSPI.h> // Hardware-specific library

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define TFT_GREY 0x5AEB // New colour


static int offset = 30;
static int foundDeviceCnt = 0;
static int marginY = 6;

#define nextButtonPin 12
#define selectButtonPin 14

#define right 16
#define left 17
#define go 26
#define back 27

int cntPos = 1;
int prevPos = 1;

#include "BLEDevice.h"
//#include "BLEScan.h"

#define ARRAYSIZE 10
BLEAdvertisedDevice results[ARRAYSIZE];

// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);

    String strData;
    strData.reserve(length);
    for(int i=0; i<length; i++){
      strData +=(char)pData[i];
    }
    //printData(strData);
    test(strData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */

  public:
  MyAdvertisedDeviceCallbacks(){
    printStatus("SEARCHING...");
  }
   
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    printStatus("...RESULT");
    
    Serial.print("BLE Advertised Device found: ");
    //Serial.println(advertisedDevice.toString().c_str());

    String deviceName = advertisedDevice.getName().c_str();
    if(deviceName.length() >1){
      results[foundDeviceCnt] = advertisedDevice;
      foundDeviceCnt++;
      printContent(deviceName);
    }

    // We have found a device, let us now see if it contains the service we are looking for.
//    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
//
//      BLEDevice::getScan()->stop();
//      myDevice = new BLEAdvertisedDevice(advertisedDevice);
//      doConnect = true;
//      doScan = true;
//
//    } // Found our server
  } // onResult

  void printContent(String deviceName){
    int drawingOffsetY = offset * foundDeviceCnt;
    tft.fillRect(0, drawingOffsetY, 240, 320, TFT_WHITE);
    tft.setCursor(16, drawingOffsetY + marginY);
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(1);
    tft.print(deviceName);
  }

  void printStatus(String statusText){
    tft.fillRect(0,0,240,30, TFT_GREY);
    tft.setCursor(6, 6);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.print(statusText);
  }
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_WHITE);

  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(2);
  tft.print("Display test");

  pinMode(nextButtonPin, INPUT);
  pinMode(selectButtonPin, INPUT);

  pinMode(left, OUTPUT);
  pinMode(right, OUTPUT);
  pinMode(go, OUTPUT);
  pinMode(back, OUTPUT);

  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(3, false);
} // End of setup.


// This is the Arduino main loop function.
void loop() {


  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      printConnected(myDevice->getName().c_str());
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);
    Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    
    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  buttonsCallBack();
  delay(250); // Delay a second between loops.
} // End of loop

void buttonsCallBack(){
  int nextButtonState = digitalRead(nextButtonPin);
  int selectButtonState = digitalRead(selectButtonPin);

  if(selectButtonState == HIGH){
    myDevice = new BLEAdvertisedDevice(results[cntPos-1]);
    doConnect = true;

    int drawingOffsetY = offset * prevPos + 8;
    tft.drawCircle(6, drawingOffsetY, 6, TFT_WHITE);
  }

  if(nextButtonState == HIGH){
    cntPos ++;
  }

  if(cntPos > foundDeviceCnt){
    cntPos = 1;
  }

  if(!connected){
    if(prevPos != cntPos){
      int drawingOffsetY = offset * prevPos + 8;
      tft.drawCircle(6, drawingOffsetY, 6, TFT_WHITE);
    }

    if(cntPos != 0){
      int drawingOffsetY = offset * cntPos + 8;
      tft.drawCircle(6, drawingOffsetY, 6, TFT_RED);

      prevPos = cntPos;
    }
  }
}

void printConnected(String deviceName){
  tft.fillRect(0, 0, 240, 320, TFT_WHITE);
  tft.fillRect(0, 0, 240, 30, TFT_GREY);
  tft.setCursor(6,6);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.print("Connected");

  tft.setCursor(6, 36);
  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(2);
  tft.print(deviceName);
}

void printData(String notiData){
  tft.fillRect(0, 50, 240, 100, TFT_WHITE);
  tft.setCursor(6, 70);
  tft.setTextColor(TFT_RED);
  tft.setTextSize(6);
  tft.print(notiData);
}

void test(String notiData) {
  tft.fillScreen(TFT_WHITE);
  tft.setCursor(10, 250);
  tft.setTextColor(TFT_RED);
  tft.setTextSize(6);
  tft.print(notiData);

  if(notiData.indexOf("L") >= 0){
    tft.fillTriangle(10, 200, 40, 170, 40, 230, TFT_BLACK);
    tft.fillTriangle(150, 200, 120, 170, 120, 230, TFT_WHITE);
    tft.fillRect(50, 170, 60, 60, TFT_WHITE);
    Serial.println("LEFT");
    digitalWrite(left, HIGH);
    digitalWrite(right, LOW);
  }
if(notiData.indexOf("R") >= 0){
    tft.fillTriangle(10, 200, 40, 170, 40, 230, TFT_WHITE);
    tft.fillTriangle(150, 200, 120, 170, 120, 230, TFT_BLACK);
    tft.fillRect(50, 170, 60, 60, TFT_WHITE);
    Serial.println("RIGHT");
    digitalWrite(left, LOW);
    digitalWrite(right, HIGH);
  }
if(notiData.indexOf("S") >= 0){
    tft.fillTriangle(10, 200, 40, 170, 40, 230, TFT_WHITE);
    tft.fillTriangle(150, 200, 120, 170, 120, 230, TFT_WHITE);
    tft.fillRect(50, 170, 60, 60, TFT_WHITE);
    Serial.println("STRAIGHT");
    digitalWrite(left, LOW);
    digitalWrite(right, LOW);
  }
if(notiData.indexOf("F") >= 0){
    tft.fillTriangle(10, 200, 40, 170, 40, 230, TFT_WHITE);
    tft.fillTriangle(150, 200, 120, 170, 120, 230, TFT_WHITE);
    tft.fillRect(50, 170, 60, 60, TFT_BLACK);
    Serial.println("FORWARD");
    digitalWrite(go, HIGH);
    digitalWrite(back, LOW);
  }
if(notiData.indexOf("B") >= 0){
    Serial.println("BACK");
    digitalWrite(go, LOW);
    digitalWrite(back, HIGH);
  }
if(notiData.indexOf("H") >= 0){
    Serial.println("HOLH");
    digitalWrite(go, LOW);
    digitalWrite(back, LOW);
  }
}
