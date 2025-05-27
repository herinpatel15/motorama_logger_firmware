#ifndef WIND_SPEED_H
#define WIND_SPEED_H

#include <Arduino.h>
#include <math.h>  // For pow() function

// Pin definitions
#define WIND_SPEED_PIN 3  // Interrupt pin

// Variables
volatile unsigned long windPulseCount = 0;
unsigned long lastWindTime = 0;
float windSpeed = 0.0;

// Polynomial regression calibration
const int dataSize = 8;
const int degree = 2;  // Quadratic polynomial

// Calibration data - pulse counts vs FPM
const float pulse_values[dataSize] = { 0, 130, 200, 237, 245, 300, 320, 360 };
const float fpm_values[dataSize] = { 0, 300, 450, 500, 650, 700, 800, 1000 };

/*
pulse FPM





*/



// Polynomial coefficients (will be calculated in setup)
float coefficients[degree] = { 0 };

// Matrix operations for polynomial regression
void transposeMatrix(float input[][degree], float output[][dataSize], int rows, int cols) {
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      output[j][i] = input[i][j];
    }
  }
}

void multiplyMatrices(float A[][dataSize], float B[][degree], float C[][degree], int rowsA, int colsA, int colsB) {
  for (int i = 0; i < rowsA; i++) {
    for (int j = 0; j < colsB; j++) {
      C[i][j] = 0;
      for (int k = 0; k < colsA; k++) {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
}

void multiplyMatrixVector(float A[][dataSize], float B[], float C[], int rows, int cols) {
  for (int i = 0; i < rows; i++) {
    C[i] = 0;
    for (int j = 0; j < cols; j++) {
      C[i] += A[i][j] * B[j];
    }
  }
}

void gaussianElimination(float A[][degree], float B[], float X[], int n) {
  float temp[degree][degree + 1];

  // Create augmented matrix
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      temp[i][j] = A[i][j];
    }
    temp[i][n] = B[i];
  }

  // Gaussian elimination
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      float ratio = temp[j][i] / temp[i][i];
      for (int k = 0; k <= n; k++) {
        temp[j][k] -= ratio * temp[i][k];
      }
    }
  }

  // Back substitution
  X[n - 1] = temp[n - 1][n] / temp[n - 1][n - 1];
  for (int i = n - 2; i >= 0; i--) {
    X[i] = temp[i][n];
    for (int j = i + 1; j < n; j++) {
      X[i] -= temp[i][j] * X[j];
    }
    X[i] /= temp[i][i];
  }
}

void calculatePolynomialCoefficients() {
  float X[dataSize][degree];   // Input matrix
  float Y[dataSize];           // Output vector
  float XT[degree][dataSize];  // Transpose of X
  float XTX[degree][degree];   // X^T * X
  float XTY[degree];           // X^T * Y

  // Create X matrix with polynomial terms (starting from x^1 to force through origin)
  for (int i = 0; i < dataSize; i++) {
    for (int j = 0; j < degree; j++) {
      X[i][j] = pow(pulse_values[i], j + 1);  // Start from x^1 instead of x^0
    }
    Y[i] = fpm_values[i];
  }

  // Calculate X transpose
  transposeMatrix(X, XT, dataSize, degree);

  // Calculate X^T * X
  multiplyMatrices(XT, X, XTX, degree, dataSize, degree);

  // Calculate X^T * Y
  multiplyMatrixVector(XT, Y, XTY, degree, dataSize);

  // Solve system of equations
  gaussianElimination(XTX, XTY, coefficients, degree);

  // Print coefficients for debugging
  Serial.println("Wind speed polynomial coefficients:");
  for (int i = 0; i < degree; i++) {
    Serial.print("a");
    Serial.print(i + 1);
    Serial.print(" = ");
    Serial.println(coefficients[i], 6);
  }
}

float calculateFPM(float pulse) {
  // Explicit check for zero - must return 0
  if (pulse <= 0) {
    return 0.0f;
  }

  float result = 0;
  float term = pulse;
  for (int i = 0; i < degree; i++) {
    result += coefficients[i] * term;
    term *= pulse;
  }

  // Ensure non-negative result
  return (result > 0) ? result : 0.0f;
}

// Interrupt service routine
void countWindPulses() {
  windPulseCount++;
}

void setupWindSpeed() {
  pinMode(WIND_SPEED_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WIND_SPEED_PIN), countWindPulses, FALLING);

  // Calculate polynomial coefficients at startup
  calculatePolynomialCoefficients();
}

void updateWindSpeed() {
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();

  // Update wind speed every second
  if (currentTime - lastUpdateTime >= 1000) {
    detachInterrupt(digitalPinToInterrupt(WIND_SPEED_PIN));

    unsigned long timeElapsed = currentTime - lastWindTime;
    if (timeElapsed > 0) {
      // Calculate wind speed using polynomial regression
      windSpeed = calculateFPM(windPulseCount);
    } else {
      windSpeed = 0.0;
    }
    // Serial.print(" WindPulse Count = ");
    // Serial.print(windPulseCount);
    Serial.print(" WindSpeed (FPM)= ");
    Serial.println(windSpeed);

    windPulseCount = 0;
    lastWindTime = currentTime;
    lastUpdateTime = currentTime;

    attachInterrupt(digitalPinToInterrupt(WIND_SPEED_PIN), countWindPulses, FALLING);
  }
}

float getWindSpeed() {
  // return windSpeed;
  return 50;
}

#endif