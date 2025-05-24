#ifndef WIND_SPEED_H
#define WIND_SPEED_H

#include <Arduino.h>

// Pin definitions
#define WIND_SPEED_PIN 3  // Interrupt pin

// Variables
volatile unsigned long windPulseCount = 0;
unsigned long lastWindTime = 0;
float windSpeed = 0.0;
const int pulsesPerRevolutionWind = 1;  // Change this based on your sensor
const float windSpeedFactor = 1.0;     // Calibration factor for your anemometer

// Interrupt service routine
void countWindPulses() {
  windPulseCount++;
}

void setupWindSpeed() {
  pinMode(WIND_SPEED_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WIND_SPEED_PIN), countWindPulses, FALLING);
}

void updateWindSpeed() {
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();
  
  // Update wind speed every second
  if (currentTime - lastUpdateTime >= 1000) {
    detachInterrupt(digitalPinToInterrupt(WIND_SPEED_PIN));
    
    unsigned long timeElapsed = currentTime - lastWindTime;
    if (timeElapsed > 0) {
      float rpm = (windPulseCount * 60000.0) / (timeElapsed * pulsesPerRevolutionWind);
      windSpeed = rpm * windSpeedFactor;  // Convert RPM to wind speed (adjust factor as needed)
    } else {
      windSpeed = 0.0;
    }
    
    windPulseCount = 0;
    lastWindTime = currentTime;
    lastUpdateTime = currentTime;
    
    attachInterrupt(digitalPinToInterrupt(WIND_SPEED_PIN), countWindPulses, FALLING);
  }
}

float getWindSpeed() {
  // return windSpeed;
  return 20;
}

#endif