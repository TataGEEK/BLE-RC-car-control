/* instrukce
 *  HODINKY
 *  v TFT_eSPI-master/User_Setup_Select.h používám #include <User_Setups/Setup26_TTGO_T_Wristband.h>
 *  vývojová deska ESP32 Dev Module
 *  
 */

/*
Based on:
Xinyuan-LilyGO (T-Wristband example)
https://github.com/Xinyuan-LilyGO/LilyGO-T-Wristband

Neil Kolban (Bluetooth Low Energy communication)
https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp

Eric 0015 (Bluetooth Low Energy communication)
https://github.com/0015/IdeasNProjects/tree/master/Esp32_BLE_to_BLE

Luboš M. (Bluetooth Low Energy communication)
https://navody.arduino-shop.cz/navody-k-produktum/esp32-a-bluetooth-low-energy-ble.html
*/


/****************************/
/* T-WRISTBAND LILYGO ESP32 */
/****************************/
#define TP_PIN_PIN          33
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#define IMU_INT_PIN         38
#define RTC_INT_PIN         34
#define BATT_ADC_PIN        35
#define VBUS_PIN            36
#define TP_PWR_PIN          25
#define LED_PIN             4
#define CHARGE_PIN          32

/***********/
/* BATTERY */
/***********/
int vref = 1100;

/*****************************/
/* GYROSCOPE & ACCELEROMETER */
/*****************************/
#include <SPI.h>
#include <Wire.h>
#include "sensor.h"
char buff[256];
String instrukce;
String smer;
String last_instruction;

/****************/
/* TOUCH BUTTON */
/****************/
bool pressed = false;
uint32_t pressedTime = 0;
bool charge_indication = false;
uint8_t func_select = 0;

uint32_t targetTime = 0;       // for next 1 second timeout
bool initial = 1;


/***********/
/* DISPLAY */
/***********/
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

/*************/
/* BLUETOOTH */
/*************/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};



void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  
  tft.init();
  tft.setRotation(1);

  targetTime = millis() + 1000;

  pinMode(TP_PIN_PIN, INPUT);
  //! Must be set to pull-up output mode in order to wake up in deep sleep mode
  pinMode(TP_PWR_PIN, PULLUP);
  digitalWrite(TP_PWR_PIN, HIGH);
  pinMode(LED_PIN, OUTPUT);


  // Create the BLE Device
  BLEDevice::init("ESP32 RC-CAR");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

String getVoltage()
{
    uint16_t v = analogRead(BATT_ADC_PIN);
    float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    return String(battery_voltage) + " V";
}

String ovladani()
{
  smer = "";    

//zastavíme
  if (((int)1000 * IMU.ax > -300) & ((int)1000 * IMU.ax < 300)) {
    smer = smer + "H";
  }
//jedeme zpět
  if (((int)1000 * IMU.ax < -300)) {
    smer = smer + "B";
  }
//jedeme dopředu
  if (((int)1000 * IMU.ax > 300)) {
    smer = smer + "F";
  }
  
//zatočíme vlevo
  if ((int)1000 * IMU.ay < -200) {
    smer = smer + "L";
  }
//zatočíme vpravo
  if ((int)1000 * IMU.ay > 200) {
    smer = smer + "R";
  }
//jedeme rovně
  if (((int)1000 * IMU.ay <= 200) & ((int)1000 * IMU.ay >= -200)) {
    smer = smer + "S";
  }


  return String(smer);
        
}


void loop() {

  readMPU9250();
//pouze pro ladění, pak přesunout na psávné místo
instrukce = ovladani();

  if (last_instruction != instrukce) {
    tft.fillScreen(TFT_BLACK);
    last_instruction = instrukce;
    Serial.println(instrukce);
  }

//zatočíme vlevo
if(instrukce.indexOf("L") >= 0){
  tft.fillTriangle(10, 40, 30, 20, 30, 60, TFT_WHITE); //left triangle
  tft.fillTriangle(150, 40, 120, 20, 120, 60, TFT_BLACK); //right triangle
}
//zatočíme vpravo
if(instrukce.indexOf("R") >= 0){
  tft.fillTriangle(10, 40, 30, 20, 30, 60, TFT_BLACK); //left triangle
  tft.fillTriangle(150, 40, 120, 20, 120, 60, TFT_WHITE); //right triangle
}
if(instrukce.indexOf("S") >= 0){
  tft.fillTriangle(10, 40, 30, 20, 30, 60, TFT_BLACK); //left triangle
  tft.fillTriangle(150, 40, 120, 20, 120, 60, TFT_BLACK); //right triangle
}
if(instrukce.indexOf("B") >= 0){
  tft.fillTriangle(80, 20, 60, 40, 100, 40, TFT_BLACK);
  tft.fillTriangle(80, 60, 60, 40, 100, 40, TFT_WHITE);
}
if(instrukce.indexOf("F") >= 0){
  tft.fillTriangle(80, 20, 60, 40, 100, 40, TFT_WHITE);
  tft.fillTriangle(80, 60, 60, 40, 100, 40, TFT_BLACK);
}
if(instrukce.indexOf("H") >= 0){
  tft.fillTriangle(80, 20, 60, 40, 100, 40, TFT_BLACK);
  tft.fillTriangle(80, 60, 60, 40, 100, 40, TFT_BLACK);
}

tft.drawCentreString(getVoltage(), 130, 70, 1); // Next size up font 2

snprintf(buff, sizeof(buff), "x %.2f  %.2f  %.2f", (int)1000 * IMU.ax, IMU.gx, IMU.mx);
    tft.drawString(buff, 0, 70);

    // klepnutí na hodinky blikne diodou
    if (IMU.gx > 200) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
    }

    if (digitalRead(TP_PIN_PIN) == HIGH) {
      if (!pressed) {
        initial = 1;
        targetTime = millis() + 1000;
        func_select = func_select + 1 > 3 ? 0 : func_select + 1;
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        pressed = true;
        pressedTime = millis();
      } else {
        if (millis() - pressedTime > 3000) {
          tft.fillScreen(TFT_BLACK);
          tft.fillScreen(TFT_BLACK);
          tft.drawString("Dlouhý stisk",  50, tft.height() / 2 - 10);
          tft.drawString("Go to Sleep",  53, tft.height() / 2 + 10);
          Serial.println("Go to Sleep");
          delay(3000);
          tft.writecommand(ST7735_SLPIN);
          tft.writecommand(ST7735_DISPOFF);
          esp_sleep_enable_ext1_wakeup(GPIO_SEL_33, ESP_EXT1_WAKEUP_ANY_HIGH);
          esp_deep_sleep_start();
        }
      }
    } else {
      pressed = false;
    }
  
  // notify changed value
  if (deviceConnected) {

    tft.drawString("Connected",  tft.width() / 2 - 30, 0);
    

    // vytvoření dočasné proměnné, do které je převedna zpráva na znaky
    char instrukceChar[instrukce.length() + 1];
    instrukce.toCharArray(instrukceChar, instrukce.length() + 1);
    // přepsání zprávy do BLE služby
    pCharacteristic->setValue(instrukceChar);
    pCharacteristic->notify();
    delay(500); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  } else {
    tft.drawString("Cekam na pripojeni bluetooth",  0, 0);
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising

    // přidáno
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("start advertising",  0, tft.height() / 2 - 20);

    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
