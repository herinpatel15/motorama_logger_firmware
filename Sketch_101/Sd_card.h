#include <SD.h>
#include <SPIFFS.h>
//#include <Rtc.h>
#define SD_CS_PIN 5
String currentFileName;
unsigned long lastLogTime = 0;
float totalEnergy = 0;


void setupSD() {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card Mount Failed");
    return;
  }
}

String getFileName(DateTime date) {
  char fileName[13];
  sprintf(fileName, "%02d%02d%04d.csv", date.day(), date.month(), date.year());
  return String(fileName);
}

void createOrOpenFile() {
  DateTime now = rtc.now();
  currentFileName = getFileName(now);
  
  if (!SD.exists("/" + currentFileName)) {
    File dataFile = SD.open("/" + currentFileName, FILE_WRITE);
    if (dataFile) {
      dataFile.println("Time,Voltage,Current,Power,PowerFactor,Energy,Temperature,Humidity,operationtime"); // ,windspeed,rpm
      dataFile.close();
      Serial.println("Created new file: " + currentFileName);
    } else {
      Serial.println("Error creating file: " + currentFileName);
    }
  } else {
    Serial.println("File already exists: " + currentFileName);
  }
}

void logData(DateTime now,int voltage,float current,float power, float pF,float energy/*,float windspeed,int rpm*/,float temp, float humid,float operatioanltime) {
char timestamp[20];
  sprintf(timestamp, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  
  String dataString = String(timestamp) + "," +
                      String(voltage) + "," +
                      String(current, 3) + "," +
                      String(power, 2) + "," +
                      String(pF, 2) + "," +
                      String(energy, 3) + "," +
                      // String(windspeed)+","+
                      // String(rpm)+","+
                      String(temp, 1) + "," +
                      String(humid, 1);+"'"+
                      String(operatioanltime);
  
  File dataFile = SD.open("/" + currentFileName, FILE_APPEND);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  } else {
    Serial.println("Error opening file for logging");
  }
  
  // Check if we need to create a new file for the next day
  if (now.hour() == 23 && now.minute() == 59 && now.second() == 59) {
    DateTime tomorrow = now + TimeSpan(1, 0, 0, 0);
    currentFileName = getFileName(tomorrow);
    createOrOpenFile();
  }
}