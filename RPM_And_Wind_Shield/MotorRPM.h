#ifndef MOTOR_RPM_H
#define MOTOR_RPM_H

#include <Arduino.h>

// Pin definitions
#define MOTOR_RPM_PIN 2  // Interrupt pin

// Variables
volatile unsigned long motorPulseCount = 0;
unsigned long lastMotorTime = 0;
float motorRPM = 0.0;
const int pulsesPerRevolution = 1;  // Change this based on your sensor

// Interrupt service routine
void countMotorPulses() {
  motorPulseCount++;
}

void setupMotorRPM() {
  pinMode(MOTOR_RPM_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MOTOR_RPM_PIN), countMotorPulses, FALLING);
}

void updateMotorRPM() {
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();
  
  // Update RPM every second
  if (currentTime - lastUpdateTime >= 1000) {
    detachInterrupt(digitalPinToInterrupt(MOTOR_RPM_PIN));
    
    unsigned long timeElapsed = currentTime - lastMotorTime;
    if (timeElapsed > 0) {
      motorRPM = (motorPulseCount * 60000.0) / (timeElapsed * pulsesPerRevolution);
    } else {
      motorRPM = 0.0;
    }
    
    motorPulseCount = 0;
    lastMotorTime = currentTime;
    lastUpdateTime = currentTime;
    
    attachInterrupt(digitalPinToInterrupt(MOTOR_RPM_PIN), countMotorPulses, FALLING);
  }
}

float getMotorRPM() {
  // return motorRPM;
  return 15;
}

#endif