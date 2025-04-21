#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

#define SERVICE_UUID        ""
#define CHARACTERISTIC_UUID ""

const int buzzerPin = 25;
const int vibrationMotorPins[16] = {4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 26, 27, 33};
bool deviceConnected = false;
BLECharacteristic* pCharacteristic = nullptr;

// Unified NodeInfo structure that matches JSON structure
struct NodeInfo {
  int id;          // Motor ID (1-16)
  int intense;     // Intensity value (0-255)
  int dur;         // Duration in milliseconds
  bool motorActive; // Current state of the motor
  
  // Default constructor
  NodeInfo() : id(0), intense(0), dur(0), motorActive(false) {}
  
  // Constructor with parameters
  NodeInfo(int _id, int _intense, int _dur) : 
    id(_id), intense(_intense), dur(_dur), motorActive(false) {}
};

// Array to store information for all motors
NodeInfo motors[16];

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
    ledcAttachChannel(vibrationMotorPins[i], 5000, 8, i);// Set up PWM channel
    ledcWrite(i, 0); // Set initial duty cycle to 0
    
    // Initialize motor data structure (ID is 1-based, array is 0-based)
    motors[i] = NodeInfo(i+1, 0, 0);
    
    Serial.print("Motor ");
    Serial.print(i+1);
    Serial.print(" initialized on pin ");
    Serial.println(vibrationMotorPins[i]);
  }
}

// Update motor data with random values and create JSON
String createMotorsJson() {
  StaticJsonDocument<1024> doc;
  JsonArray motorArray = doc.createNestedArray("motors");
  
  for (int i = 0; i < 16; i++) {
    // Update motor data
    motors[i].intense = random(50, 256);
    motors[i].dur = random(100, 1001);
    
    // Create JSON object from motor data
    JsonObject motor = motorArray.createNestedObject();
    motor["id"] = motors[i].id;
    motor["intense"] = motors[i].intense;
    motor["dur"] = motors[i].dur;
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

// Function to activate a specific motor with the stored parameters
void activateMotor(int motorIndex) {
  if (motorIndex < 0 || motorIndex >= 16) return;
  
  NodeInfo& motor = motors[motorIndex];
  int channel = motorIndex; // PWM channel matches array index
  
  // Set motor active
  motor.motorActive = true;
  
  // Apply intensity via PWM
  ledcWrite(channel, motor.intense);
  
  // Schedule deactivation after duration
  // Note: In a real implementation, you'd want to use a non-blocking approach
  delay(motor.dur);
  
  // Turn off motor
  ledcWrite(channel, 0);
  motor.motorActive = false;
}

// Function to parse incoming JSON and update motor data
bool parseMotorCommand(String jsonString) {
  // Remove end marker if present
  if (jsonString.endsWith(";")) {
    jsonString = jsonString.substring(0, jsonString.length() - 1);
  }
  
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return false;
  }
  
  // Check if it's a single motor command
  if (doc.containsKey("id") && doc.containsKey("intense") && doc.containsKey("dur")) {
    int id = doc["id"];
    if (id >= 1 && id <= 16) {
      int arrayIndex = id - 1; // Convert 1-based ID to 0-based index
      motors[arrayIndex].intense = doc["intense"];
      motors[arrayIndex].dur = doc["dur"];
      
      // Activate motor
      activateMotor(arrayIndex);
      return true;
    }
  } 
  // Check if it's a multi-motor command
  else if (doc.containsKey("motors")) {
    JsonArray motorArray = doc["motors"];
    for (JsonObject motorObj : motorArray) {
      if (motorObj.containsKey("id") && motorObj.containsKey("intense") && motorObj.containsKey("dur")) {
        int id = motorObj["id"];
        if (id >= 1 && id <= 16) {
          int arrayIndex = id - 1;
          motors[arrayIndex].intense = motorObj["intense"];
          motors[arrayIndex].dur = motorObj["dur"];
          
          // In a real application, you'd queue these commands rather than activating immediately
          // For demonstration, we'll just update the values
        }
      }
    }
    return true;
  }
  
  return false;
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
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  
  pCharacteristic->addDescriptor(new BLE2902());
  
  // Add write callback to handle incoming commands
  class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        String jsonString = String(value.c_str());
        Serial.print("Received: ");
        Serial.println(jsonString);
        parseMotorCommand(jsonString);
      }
    }
  };
  
  pCharacteristic->setCallbacks(new CharacteristicCallbacks());
  
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