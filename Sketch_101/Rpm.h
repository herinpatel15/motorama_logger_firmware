 #define ANEMOMETER 56
#define RPM_SENSOR_PIN 4 // Define the pin for RPM input
#define PROXIMITY 25

// Variables for RPM calculation
volatile unsigned long pulse_count_anemometer = 0;
volatile unsigned long pulseCount = 0; // Count pulses
unsigned long lastMillis = 0;
float currentRPM = 0;

// Calibration data points for wind speed
float rpm_values[] = {0, 8, 12, 16, 20, 22, 21, 24, 30, 32, 33, 39, 40, 42, 45, 48, 53, 58, 60, 61, 66, 70, 71, 75, 80};
float fpm_values[] = {0, 200, 260, 300, 340, 372, 394, 410, 440, 500, 535, 540, 512, 529, 535, 600, 660, 700, 770, 800, 850, 860, 870, 900, 960};
const int numPoints = sizeof(rpm_values) / sizeof(rpm_values[0]);

// Stabilization variables
float previousRPM = -1; 
float stableSpeed = 0.0;
float stableWindSpeed  =0;


volatile unsigned long pulse_count_proximity = 0;
unsigned long last_time = 0;
long rpm_anemometer = 0;
float inlet_area = 96.87;
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay_RPM = 15;  // Adjust as needed (milliseconds)
const unsigned long debponceDelay_anemometer = 6;


// Global variable to store RPM and Windspeed
void IRAM_ATTR handlePulse() {
   pulseCount++;
  Serial.print(".");
  // static unsigned long lastDebounceTime = 0;
  // unsigned long currentTime = millis();
  
  // if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
  //   pulseCount++;
  //   lastDebounceTime = currentTime;
  //   // Quick debug pulse - use sparingly in ISR
  //   Serial.print(".");
  // }
}


void getStableWindSpeed(void * parameter) {
  for(;;){
  unsigned long currentMillis = millis();
  // Calculate RPM every second
  if (currentMillis - lastMillis >= 1000) {
    noInterrupts(); // Disable interrupts temporarily
    unsigned long count = pulseCount; // Copy pulse count
    pulseCount = 0; // Reset pulse count
    interrupts(); // Re-enable interrupts
    currentRPM = (count); // Convert pulses/sec to RPM (assuming 1 pulse per rotation)
  // Stabilize RPM within ±5 range
  if (previousRPM != -1 && abs(currentRPM - previousRPM) <= 5) {
    currentRPM = previousRPM;
  } else {
    previousRPM = currentRPM;
  }
  // Linear interpolation to calculate wind speed
  float windSpeed = 0.0;
  for (int i = 0; i < numPoints - 1; i++) {
    if (currentRPM >= rpm_values[i] && currentRPM <= rpm_values[i + 1]) {
      windSpeed = fpm_values[i] + 
                  (currentRPM - rpm_values[i]) * 
                  (fpm_values[i + 1] - fpm_values[i]) / 
                  (rpm_values[i + 1] - rpm_values[i]);
      break;
    }
  }
  // Smooth output using exponential smoothing
  float alpha = 0.1; // Smoothing factor
  stableSpeed = alpha * windSpeed + (1 - alpha) * stableSpeed;
  stableWindSpeed= stableSpeed;
  Serial.println(stableSpeed);
  }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


void IRAM_ATTR countPulse_anemometer()
{
  unsigned long currentTime = millis();
  if ((currentTime - lastDebounceTime) > debponceDelay_anemometer) {
    pulse_count_anemometer++;
    lastDebounceTime = currentTime;
  }
  
}

void IRAM_ATTR countPulse_proximity()
{
  
    unsigned long currentTime = millis();
  if ((currentTime - lastDebounceTime) > debounceDelay_RPM) {
    pulse_count_proximity++;
    lastDebounceTime = currentTime;
  }

}

void calculateRPM(void * parameter) {
  for(;;) {
    unsigned long current_time = millis();
    unsigned long elapsed_time = current_time - last_time;
    
    if (elapsed_time >= 1000) {  // Calculate RPM every second
      rpm_anemometer = (pulse_count_anemometer * 60 * 1000) / elapsed_time;
      rpm = (pulse_count_proximity * 60 * 1000) / elapsed_time;
      
      
     
        windspeed = (rpm_anemometer/6)*1.2954;      // wind speed multiplier 0.006585 in m/s   //1.2954 fot ft/minute
        windspeed = windspeed/1.85;                 // update by abhisekh bhai
     
      // cfm = windspeed*inlet_area*0.0011;
      // Serial.print("wind speed(m/s) : ");
      // Serial.println(windspeed);
       
      
      pulse_count_anemometer = 0;
      pulse_count_proximity =0;
      last_time = current_time;
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to prevent busy waiting
  }
}
