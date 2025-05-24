#define AHT10_ADDRESS 0x38
#define AHT10_INIT_CMD 0xE1
#define AHT10_MEASURE_CMD 0xAC
// float temperature;
// float humidity;



 // Initialize AHT10
 void AH10_init(){
  Wire.beginTransmission(AHT10_ADDRESS);
  Wire.write(AHT10_INIT_CMD);
  Wire.write(0x08);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(100);
 }

bool readAHT10(float *temp, float *humid) {
  uint8_t data[6];
  
  // Send measurement command
  Wire.beginTransmission(AHT10_ADDRESS);
  Wire.write(AHT10_MEASURE_CMD);
  Wire.write(0x33);
  Wire.write(0x00);
  Wire.endTransmission();
  
  delay(80); // Wait for measurement to complete
  
  // Read data
  Wire.requestFrom(AHT10_ADDRESS, 6);
  if (Wire.available() != 6) {
    return false;
  }
  
  for (int i = 0; i < 6; i++) {
    data[i] = Wire.read();
  }
  
  // Check status bit
  if (data[0] & 0x80) {
    return false;
  }
  
  // Calculate humidity and temperature
  *humid = ((float)((data[1] << 12) | (data[2] << 4) | (data[3] >> 4))) / 1048576.0 * 100.0;
  *temp = ((float)(((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5])) / 1048576.0 * 200.0 - 50.0;
  
  return true;
}
