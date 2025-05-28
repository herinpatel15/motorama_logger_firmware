#ifndef I2C_COMMUNICATION_H
#define I2C_COMMUNICATION_H

#include <HardwareSerial.h>

// Create a HardwareSerial instance (Serial1)
HardwareSerial SerialSensor(1);  // UART1

// ESP32 UART pins
#define UART_RX_PIN 27
#define UART_TX_PIN 26

// Data structure to match the Nano's struct
struct SensorData {
  float motorRPM;
  float windSpeed;
};

// Global variables to store received data
float receivedRPM = 0;
float receivedWindSpeed = 0;
unsigned long lastUARTReadTime = 0;
const unsigned long UART_READ_INTERVAL = 1000; // Read every 1 second

void setupUART() {
  SerialSensor.begin(9600, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.println("UART communication initialized on RX=27, TX=26");
}

void readSensorData() {
  if (millis() - lastUARTReadTime >= UART_READ_INTERVAL) {
    lastUARTReadTime = millis();

    // Check if enough bytes are available
    if (SerialSensor.available() >= sizeof(SensorData)) {
      SensorData data;
      SerialSensor.readBytes((uint8_t*)&data, sizeof(data));

      receivedRPM = data.motorRPM;
      receivedWindSpeed = data.windSpeed;

      // Serial.printf("Received data - RPM: %.2f, Wind Speed: %.2f\n", receivedRPM, receivedWindSpeed);
    } else {
      Serial.println("UART communication warning - incomplete data");
    }
  }
}

float getReceivedRPM() {
  return receivedRPM;
}

float getReceivedWindSpeed() {
  return receivedWindSpeed;
}

#endif
