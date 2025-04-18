#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

#define SERVICE_UUID        ""
#define CHARACTERISTIC_UUID ""

const int buzzerPin = 26;
const int vibrationMotorPins[16] = {4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 27, 33};
bool deviceConnected = false;
BLECharacteristic* pCharacteristic = nullptr;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("Client connected");
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("Client disconnected");
    BLEDevice::startAdvertising();
  }
};

void setupVibrationMotors() {
  for (int i = 0; i < 16; i++) {
    pinMode(vibrationMotorPins[i], OUTPUT);
    digitalWrite(vibrationMotorPins[i], LOW);
  }
}

// Create a compact JSON string for all motors (id, int, dur)
String createMotorsJson() {
  StaticJsonDocument<1024> doc;
  JsonArray motorArray = doc.createNestedArray("motors");

  for (int i = 1; i <= 16; i++) {
    JsonObject motor = motorArray.createNestedObject();
    motor["id"] = i;
    motor["int"] = random(50, 256);
    motor["dur"] = random(100, 1001);
  }

  String jsonString;
  serializeJson(doc, jsonString);
  jsonString += ";";  // End marker
  return jsonString;
}

// Create JSON string for timestamp package
String createTimestampJson() {
  StaticJsonDocument<128> doc;
  doc["timestamp"] = millis();
  String jsonString;
  serializeJson(doc, jsonString);
  jsonString += ";";
  return jsonString;
}

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  setupVibrationMotors();

  BLEDevice::init("ESP32_Vibration");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE setup complete. Waiting for client...");
}

void loop() {
  if (deviceConnected) {
    // Send all motors data JSON
    String motorsJson = createMotorsJson();
    pCharacteristic->setValue(motorsJson.c_str());
    pCharacteristic->notify();
    Serial.print("Sent motors data: ");
    Serial.println(motorsJson);

    delay(100); // Small gap before sending timestamp

    // Send timestamp JSON for latency calculation
    String tsJson = createTimestampJson();
    pCharacteristic->setValue(tsJson.c_str());
    pCharacteristic->notify();
    Serial.print("Sent timestamp: ");
    Serial.println(tsJson);

    delay(5000);  // Send every 5 seconds
  } else {
    delay(100);
  }
}
