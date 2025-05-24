#include <Wire.h>
#include "MotorRPM.h"
#include "WindSpeed.h"

// I2C address for this Nano (as slave)
const uint8_t I2C_ADDRESS = 0x08;

// Data structure to send to ESP32
struct SensorData {
  float motorRPM;
  float windSpeed;
};

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing RPM Sensors...");
  
  // Initialize I2C as slave
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(requestEvent);
  
  // Initialize sensors
  setupMotorRPM();
  setupWindSpeed();
}

void loop() {
  // Update sensor readings
  updateMotorRPM();
  updateWindSpeed();
  
  // Small delay to prevent flooding
  delay(100);
}

// Function called when ESP32 requests data
void requestEvent() {
  SensorData data;
  data.motorRPM = getMotorRPM();
  data.windSpeed = getWindSpeed();
  
  Wire.write((byte*)&data, sizeof(data));
}