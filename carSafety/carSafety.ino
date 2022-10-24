//////////////////////////////////////////////pwm part ////////////////////////
int PWM_FREQUENCY = 1000;
int PWM_CHANNEL = 0;
int PWM_RESOUTION = 8;
int GPIOPIN = 10 ;
int dutyCycle = 127;

///////////////////////////////////////////////ble part//////////////////////////
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 72;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};



/////////////////////////////////////////ble part /////////////////////////////

#include "ESP32_C3_ISR_Servo.h"
#define heartPin 4
#define ServoPin 3
#define buzzerPin 1
bool heartRate = true;
bool doitonce = true;
bool doitone = true;
bool doitaccident = false;
#define CheckBlinkPin 6
#define CheckHeartPin 5
int servoIndex1  = -1;
bool blinkEye = true;
const long blinkOnDuration = 2500;
const long blinkOffDuration = 300;
unsigned long rememberBlinkTime = 0;
unsigned long sendTime = 0;
bool blinkState = false;
#define MIN_MICROS      800  //544
#define MAX_MICROS      2450
#define USE_ESP32_TIMER_NO          0
#define MAXSPEED 255
static int position = 0;
const long onDuration = 200;
const long offDuration = 800;
unsigned long rememberTime =  0;
bool ledState = HIGH;

long lastBuzzerTime = 0;
long lastMessageTime  = 0;
/*void IRAM_ATTR blinkISR()
  {
  // Toggle The LED
  // blinkEye = ~blinkEye;
   blinkEye = (blinkEye == LOW) ? HIGH : LOW;
  }
  void IRAM_ATTR heartISR()
  {
  // Toggle The LED
  // blinkEye = ~blinkEye;
   heartRate = (heartRate == LOW) ? HIGH : LOW;
  }*/
bool buttonState;
bool buttonState2;
bool lastEyeReading = LOW;
bool lastHeartReading = LOW;
bool sleeping = false;
unsigned long lastEyeDebounceTime = 0;
unsigned long lastHeartDebounceTime = 0;
unsigned long debounceDelay =  50;
void setup() {
  ///////////pwm part///////////////////////////////
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOUTION);
  ledcAttachPin(GPIOPIN, PWM_CHANNEL);
  for (int i = 0; i < MAXSPEED; i++) {
    ledcWrite(PWM_CHANNEL, i);
    delay(10);
  }
  /////////////////////////////////////////////////
  //ledcWrite(PWM_CHANNEL,255);
  Serial.begin(115200);
  ESP32_ISR_Servos.useTimer(USE_ESP32_TIMER_NO);
  servoIndex1 = ESP32_ISR_Servos.setupServo(ServoPin, MIN_MICROS, MAX_MICROS);
  ESP32_ISR_Servos.setPosition(servoIndex1, 45);
  delay(30);
  pinMode(heartPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(CheckBlinkPin, INPUT);
  //attachInterrupt(CheckBlinkPin,blinkISR, FALLING);
  pinMode(CheckHeartPin, INPUT);
  //attachInterrupt(CheckHeartPin,heartISR, FALLING);
  ///////////////////////////////////ble part //////////////////////////////
  // Create the BLE Device
  BLEDevice::init("UART Service");

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
  Serial.println("Waiting a client connection to notify...");
  //////////////////////////////////////ble part ///////////////////


}

void loop() {
  // put your main code here, to run repeatedly:
  //ESP32_ISR_Servos.setPosition(servoIndex1,0);

  // ESP32_ISR_Servos.setPosition(servoIndex1, 90);
  int eyeReading = digitalRead(CheckBlinkPin);
  if (eyeReading != lastEyeReading) {
    lastEyeDebounceTime = millis();
  }
  if ((millis() - lastEyeDebounceTime) > debounceDelay) {
    if (eyeReading != buttonState) {
      buttonState = eyeReading;
      if (buttonState == HIGH) {
        blinkEye = !blinkEye;
      }
    }
  }
  lastEyeReading = eyeReading;

  int heartReading = digitalRead(CheckHeartPin);
  if (heartReading != lastHeartReading) {
    lastHeartDebounceTime = millis();
    // Serial.println("heartDebounceExecuted");
  }
  if ((millis() - lastHeartDebounceTime) > debounceDelay) {
    if (heartReading != buttonState2) {
      buttonState2 = heartReading;
      if (buttonState2 == HIGH) {
        heartRate = !heartRate;
      }
    }
  }
  lastHeartReading = heartReading;

  if (blinkEye) {
    digitalWrite(buzzerPin, LOW);
    doitonce = true;
    sleeping = true;
    if (blinkState == HIGH) {
      if (millis() - rememberBlinkTime >= blinkOnDuration) {
        blinkState = LOW;
        rememberBlinkTime = millis();
      }
    }
    else {
      if (millis() - rememberBlinkTime >= blinkOffDuration) {
        blinkState = HIGH;
        rememberBlinkTime = millis();
      }
    }
    lastBuzzerTime = millis();
    lastMessageTime = millis();
    ESP32_ISR_Servos.setPosition(servoIndex1, blinkState ? 0 : 90);
    delay(30);
    
    if (doitaccident) {
      doitaccident = false;
      for (int i = 0; i < MAXSPEED; i++) {
        ledcWrite(PWM_CHANNEL, i);
        delay(1);
      }
    }
    bitSet(txValue,1);
  }
  else
  {
    doitaccident = true;
    
     // Serial.println(doitonce);
    if (doitonce) {
      if (millis() - lastBuzzerTime >= 10000) {
        digitalWrite(buzzerPin, HIGH);
        lastBuzzerTime = millis();
        Serial.println("buzzerTurnedOn");
      }
      if (millis() - lastMessageTime >= 12000) {
        digitalWrite(buzzerPin, HIGH);
        lastMessageTime = millis();
        Serial.println("sendMessageToPhone: eye closed");
        bitClear(txValue,1);
        for (int i = MAXSPEED; i >= 0; i--) {
          delay(5);
          ledcWrite(PWM_CHANNEL, i);
        }
        doitonce = false;
      }
      
    }
    //
    if(sleeping){
      
        ESP32_ISR_Servos.setPosition(servoIndex1, 45);
        delay(1);
     
      sleeping = false;
    }
    ESP32_ISR_Servos.setPosition(servoIndex1, 90);
    //   delay(10);
  }
  if (heartRate) {
    doitone = true;
    if (ledState == HIGH) {
      if (millis() - rememberTime >= onDuration) {
        ledState = LOW;
        rememberTime = millis();
      }
    }
    else {
      if (millis() - rememberTime >= offDuration) {
        ledState = HIGH;
        rememberTime = millis();
      }
    }
    digitalWrite(heartPin, ledState);
    bitSet(txValue,0);
  }
  else
  {
    if (ledState == HIGH) {
      if (millis() - rememberTime >= 50) {
        ledState = LOW;
        rememberTime = millis();
      }
    }
    else {
      if (millis() - rememberTime >= 100) {
        ledState = HIGH;
        rememberTime = millis();
      }
    }
    digitalWrite(heartPin, ledState);
    if (doitone) {
      // delay(100);
      Serial.println("heartRate is abnormal send message to phone");
      bitClear(txValue,0);
   
      doitone = false;
    }

  }
  //Serial.println(blinkEye);
  ///////////////////////////////////ble part ////////////////////////////////
  if (millis() - sendTime >= 1000) {
    if (deviceConnected) {
      pTxCharacteristic->setValue(&txValue, 1);
      pTxCharacteristic->notify();
      //txValue++;
      delay(10); // bluetooth stack will go into congestion, if too many packets are sent
      //  Serial.print(heartRate);
          Serial.println(txValue);
      //   Serial.print(blinkEye);
      //   Serial.println();
    }
    sendTime = millis();
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
  ///////////////////////////////////////ble part //////////////////////////////

}
