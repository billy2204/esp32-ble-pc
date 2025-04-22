#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

#define SERVICE_UUID ""
#define CHARACTERISTIC_UUID ""

int buzzerPin = 25;
int vibrationMotorPins[16] = { 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 26, 27, 33 };
bool deviceConnected = false;
BLECharacteristic* pCharacteristic = nullptr;

// Unified NodeInfo structure that matches JSON structure
struct NodeInfo {
  int id;            // Motor ID (1-16)
  int intensity;     // Intensity value (0-255)
  int duration;      // Duration in milliseconds
  int order;  // Current state of the motor

  // Default constructor
  NodeInfo()
    : id(0), intensity(0), duration(0), order(0) {}

  // Constructor with parameters
  NodeInfo(int _id, int _intensity, int _duration, int _ )
    : id(_id), intensity(_intensity), duration(_duration), motorActive(false) {}
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
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
    delay(500);
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
    BLEDevice::startAdvertising();
  }
};

void setupVibrationMotors() {
  for (int i = 0; i < 16; i++) {
    pinMode(vibrationMotorPins[i], OUTPUT);
    ledcAttachChannel(vibrationMotorPins[i], 5000,8, i); // Attach pin to channel
    ledcWrite(i, 0);        // Set initial duty cycle to 0

    // Initialize motor data structure (ID is 1-based, array is 0-based)
    motors[i] = NodeInfo(i + 1, 0, 0);

    Serial.print("Motor ");
    Serial.print(i + 1);
    Serial.print(" initialized on pin ");
    Serial.println(vibrationMotorPins[i]);
  }
}

// Track motor activation times
unsigned long motorActivationTimes[16] = { 0 };

// Function to activate either a specific motor or all motors
void activateMotor(int motorIndex, bool isActiveAll = false) {
  // Check if we need to activate all motors
  if (isActiveAll) {
    Serial.println("Activating all motors");
    // Activate all motors with their stored parameters
    for (int i = 0; i < 16; i++) {
      NodeInfo& motor = motors[i];

      // Set motor active
      motor.motorActive = true;

      // Apply intensity via PWM
      ledcWrite(i, motor.intensity);

      // Record activation time to deactivate later
      motorActivationTimes[i] = millis();

      Serial.print("Motor ");
      Serial.print(motor.id);
      Serial.print(" activated with intensity ");
      Serial.print(motor.intensity);
      Serial.print(" for ");
      Serial.print(motor.duration);
      Serial.println(" ms");
    }
  }
  // Otherwise activate just the specified motor
  else {
    if (motorIndex < 0 || motorIndex >= 16) return;

    NodeInfo& motor = motors[motorIndex];
    int channel = motorIndex;  // PWM channel matches array index

    // Set motor active
    motor.motorActive = true;

    // Apply intensity via PWM
    ledcWrite(channel, motor.intensity);

    // Record activation time to deactivate later
    motorActivationTimes[motorIndex] = millis();

    Serial.print("Motor ");
    Serial.print(motor.id);
    Serial.print(" activated with intensity ");
    Serial.print(motor.intensity);
    Serial.print(" for ");
    Serial.print(motor.duration);
    Serial.println(" ms");
  }
}

// Function to check and update motor states (non-blocking)
void updateMotorStates() {
  unsigned long currentTime = millis();

  for (int i = 0; i < 16; i++) {
    NodeInfo& motor = motors[i];

    // Check if this motor is active and needs to be turned off
    if (motor.motorActive && (currentTime - motorActivationTimes[i] >= motor.duration)) {
      // Turn off the motor
      ledcWrite(i, 0);
      motor.motorActive = false;

      Serial.print("Motor ");
      Serial.print(motor.id);
      Serial.println(" deactivated");
    }
  }
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
  if (doc.containsKey("id") && doc.containsKey("intensity") && doc.containsKey("duration")) {
    int id = doc["id"];
    if (id >= 1 && id <= 16) {
      int arrayIndex = id - 1;  // Convert 1-based ID to 0-based index
      motors[arrayIndex].intensity = doc["intensity"];
      motors[arrayIndex].duration = doc["duration"];

      // Check if we need to activate all motors
      bool activateAllMotors = false;
      if (doc.containsKey("isActiveAll")) {
        activateAllMotors = doc["isActiveAll"];
      }

      // Activate motor(s)
      activateMotor(arrayIndex, activateAllMotors);
      return true;
    }
  }
  // Check if it's a multi-motor command
  else if (doc.containsKey("motors")) {
    JsonArray motorArray = doc["motors"];

    // Check if we need to activate all motors
    bool activateAllMotors = false;
    if (doc.containsKey("isActiveAll")) {
      activateAllMotors = doc["isActiveAll"];
    }

    for (JsonObject motorObj : motorArray) {
      if (motorObj.containsKey("id") && motorObj.containsKey("intensity") && motorObj.containsKey("duration")) {
        int id = motorObj["id"];
        if (id >= 1 && id <= 16) {
          int arrayIndex = id - 1;
          motors[arrayIndex].intensity = motorObj["intensity"];
          motors[arrayIndex].duration = motorObj["duration"];

          // If not activating all, activate individual motors immediately
          if (!activateAllMotors) {
            activateMotor(arrayIndex, false);
          }
        }
      }
    }

    // If activating all, do it after updating all parameters
    if (activateAllMotors) {
      activateMotor(0, true);
    }

    return true;
  }

  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  setupVibrationMotors();

  BLEDevice::init("ESP32_Vibration_Glove");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());

  // Add write callback to handle incoming commands
  class CharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
      String valueStr = pCharacteristic->getValue(); //get value as json string from client
      if (valueStr.length() > 0) {
        String jsonString = String(valueStr.c_str());
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
  // Non-blocking motor management
  updateMotorStates();

  // Always give the system some time to process other tasks
  delay(10);
}