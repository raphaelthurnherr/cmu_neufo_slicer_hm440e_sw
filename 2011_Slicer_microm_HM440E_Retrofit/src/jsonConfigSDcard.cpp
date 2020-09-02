/**
 * @file jsonConfigSDcard.c
 * @author Raphael Thurnherr (raphael.thurnherr@unige.ch)
 * @brief These functions can open a file, deserialize the JSON data and put the values to a defined stucture in header file
 * @version 0.2
 * @date 2020-08-06
 * 
 * @remark 06.08.2020 - Adding config for NTC resistor and Temperature alarm degree user Setting, change "tempAlarm" by "alarmState"
 * 
 * @copyright Copyright (c) 2020
 *  
 */

// User application header file
#include "jsonConfigSDcard.h"

// SDcard utility libraries
#include <SPI.h>
#include "SdFat.h"

// JSON serialize/deserialize library
#include <ArduinoJson.h>

//  SD Card CS pin declaration for Arduino MKR zero 
#define SD_CS_PIN SDCARD_SS_PIN

// Code error declaration
#define NO_ERROR 0

// Functions declarations
int loadFileFromSD(char * fileName, char * destinationBuffer);
int SaveFileToSD(char * fileName, char * sourceBuffer);


/**
 * @brief Get the User Settings From Config file on the SD card
 * 
 * @param fileName fileName on the SD card
 * @param userSetting pointer to the users settings structure
 * @param configNb User setting number to read
 * @return int error code
 */

int getUserSettingsFromConfig(char * fileName, SETTINGS * userSetting, int configNb){
// Allocate the JSON document
// Inside the brackets, 2048 is the capacity of the memory pool in bytes.
// Don't forget to change this value to match your JSON document.
// Use arduinojson.org/v6/assistant to compute the capacity.
StaticJsonDocument<FILE_BUFFER_SIZE> JSONdoc;

 // Buffer for data string from file
char buffer[FILE_BUFFER_SIZE];

// Load file from SD card and put them to the buffer
if(loadFileFromSD(fileName, buffer) == NO_ERROR){

  // Deserialize the JSON document to the JSONdoc object
    DeserializationError JSONerror = deserializeJson(JSONdoc, buffer);

    // Test if parsing succeeds, return -1 (error) if failed otherwise, return 0
    if (JSONerror) {

      #ifdef SERIAL_DEBUG
      Serial.print(F("User config deserializeJson() failed: "));
      Serial.println(JSONerror.c_str());
      #endif
      return -1;
    }else
    {
      #ifdef SERIAL_DEBUG
      Serial.write("\n\nUser config JSON deserialization SUCCESS !\n\n");
      #endif

      // get the user config name from string
      strcpy(userSetting->name, JSONdoc["UsersSettings"][configNb]["Name"]);
      
      // get the tinkness mode from string
      if(!strcmp(JSONdoc["UsersSettings"][configNb]["DefaultThicknessMode"], "normal")){
        userSetting->mode = 0;
      }else userSetting->mode = 1;
      
      // Get others datas from integer values
      userSetting->thicknessNormalMode = JSONdoc["UsersSettings"][configNb]["thicknessNormal_um"];
      userSetting->thicknessTrimmingMode = JSONdoc["UsersSettings"][configNb]["thicknessTrimm_um"];
      userSetting->thresholdToRewind = JSONdoc["UsersSettings"][configNb]["thresholdToRewind"];
      userSetting->thresholdToCut = JSONdoc["UsersSettings"][configNb]["thresholdToCut"];

      // get the temperatur alarm state from string
      if(!strcmp(JSONdoc["UsersSettings"][configNb]["TempAlarmState"], "on")){
        userSetting->alarmState = 1;
      }else userSetting->alarmState = 0;

      userSetting->tempAlarmDegree = JSONdoc["UsersSettings"][configNb]["TemperatureAlarm"];

      #ifdef SERIAL_DEBUG
      Serial.println("User config\n------------");
      Serial.println(userSetting->name);
      Serial.println(userSetting->mode);
      Serial.println(userSetting->thicknessNormalMode);
      Serial.println(userSetting->thicknessTrimmingMode);
      Serial.println(userSetting->thresholdToRewind);
      Serial.println(userSetting->thresholdToCut);
      //Serial.println(userSetting->alarm);
      Serial.println(userSetting->alarmState);
      Serial.println(userSetting->tempAlarmDegree);
      #endif
      return 0;
    }
  }else return -1;
}

/**
 * @brief Get the General Slicer Config object
 * 
 * @param fileName fileName on the SD card
 * @param machineConfig pointer to the machine config structure
 * @return int 
 */

int getGeneralSlicerConfig(char* fileName, SLICERCONFIG* machineConfig){
// Allocate the JSON document
//
// Inside the brackets, 2048 is the capacity of the memory pool in bytes.
// Don't forget to change this value to match your JSON document.
// Use arduinojson.org/v6/assistant to compute the capacity.
StaticJsonDocument<FILE_BUFFER_SIZE> JSONdoc;

// Buffer for data string from file
char buffer[FILE_BUFFER_SIZE];

// Load file from SD card and put them ti the buffer
if(loadFileFromSD(fileName, buffer) == NO_ERROR){
  // Deserialize the JSON document to the JSONdoc variable
    DeserializationError JSONerror = deserializeJson(JSONdoc, buffer);

// Test if parsing succeeds, return -1 (error) if failed otherwise, return 0
    if (JSONerror) {

      #ifdef SERIAL_DEBUG
      Serial.print(F("Machine config deserializeJson() failed: "));
      Serial.println(JSONerror.c_str());
      #endif
      return -1;
    }else
    {
      #ifdef SERIAL_DEBUG
      Serial.write("\n\nMachine config JSON deserialization SUCCESS !\n\n");
      #endif

      // Get other data from integer
      machineConfig->BacklashCCW = JSONdoc["General"]["BacklashCCW_correction"];
      machineConfig->BacklashCW = JSONdoc["General"]["BacklashCW_correction"];
      machineConfig->HomingSpeed = JSONdoc["General"]["HomingSpeed"];
      machineConfig->MovingSpeed = JSONdoc["General"]["MovingSpeed"];

      // get the alarm state from string
      if(!strcmp(JSONdoc["General"]["ScreenBacklight"], "on")){
        machineConfig->ScreenBacklight  = 1;
      }else machineConfig->ScreenBacklight = 0;

      #ifdef SERIAL_DEBUG
      Serial.println("Machine config\n------------");
      Serial.println(machineConfig->BacklashCCW);
      Serial.println(machineConfig->BacklashCW);
      Serial.println(machineConfig->HomingSpeed);
      Serial.println(machineConfig->MovingSpeed);
      Serial.println(machineConfig->ScreenBacklight);

      #endif
      return 0;
    }
  }else return -1;
}

/**
 * @brief loadFileFromSD, Read the file specified from fileName and put them to the destination buffer
 * 
 * @param fileName pointer to the file name to open
 * @param destinationBuffer pointer to the destination buffer for get file data
 * @return int code error
 */
int loadFileFromSD(char * fileName, char * destinationBuffer){
 // Create variable type FILE for file readind
File myFile;
SdFat SD;

 #ifdef SERIAL_DEBUG 
    Serial.print("Initializing SD card...");
 #endif

  if (!SD.begin(SD_CS_PIN)) {
       #ifdef SERIAL_DEBUG 
        Serial.println("initialization failed!");
        #endif
    return -1;
  }else{
      #ifdef SERIAL_DEBUG 
          Serial.println("initialization done.");
      #endif

    // Open the file for reading:
    myFile = SD.open(fileName, FILE_READ);

    if (myFile) {
      #ifdef SERIAL_DEBUG 
          Serial.println(fileName);
      #endif

      // read from the file until there's nothing else in it:
      int i;
      for(i=0; myFile.available(); i++) {
        destinationBuffer[i] = myFile.read();
      }
      // Add terminaison character
      destinationBuffer[i] = 0;

      // close the file:
      myFile.close();

      // Return NO ERROR
      return 0;
    } else {
          #ifdef SERIAL_DEBUG 
          // if the file didn't open, print an error:
          Serial.println("error opening .cfg");
          #endif
          
          // Return ERROR
          return -1;
    }
  }
}


/**
 * @brief Save as file on SD card the general setting and the users settings
 * 
 * @param fileName to save
 * @param machineConfig pointer to machineConfig structure
 * @param userConfig pointer to users settings structure
 * @param nbOfUserConfig number of users settings to save
 * @return int error code
 */
int saveUserAndGeneralSettings(char * fileName, SLICERCONFIG *machineConfig, SETTINGS * userConfig, unsigned char nbOfUserConfig){

// Allocate the JSON document
//
// Inside the brackets, 2048 is the capacity of the memory pool in bytes.
// Don't forget to change this value to match your JSON document.
// Use arduinojson.org/v6/assistant to compute the capacity.
StaticJsonDocument<FILE_BUFFER_SIZE> JSONdoc;

// Buffer for data string for JSON serializazion
char buffer[FILE_BUFFER_SIZE];

// SERIALIZATION OF MACHINE SETTINGS
JsonObject General = JSONdoc.createNestedObject("General");
// Add machine setting integer data
General["BacklashCW_correction"] = machineConfig->BacklashCW;
General["BacklashCCW_correction"] = machineConfig->BacklashCCW;
General["HomingSpeed"] = machineConfig->HomingSpeed;
General["MovingSpeed"] = machineConfig->MovingSpeed;

// Add machine setting string data
if(machineConfig->ScreenBacklight == 0)
  General["ScreenBacklight"] = "off";
  else General["ScreenBacklight"] = "on";

// NTC Settings
General["NTC_Coeff"] = machineConfig->NTCsensor.RThbeta;
General["NTC_RRef"] = machineConfig->NTCsensor.RRef;

// SERIALIZATION OF USER SETTINGS
JsonArray UsersSettings = JSONdoc.createNestedArray("UsersSettings");

int i;
JsonObject JSON_UsersSettings[6];

for(i=0;i<nbOfUserConfig;i++){
  SETTINGS *pUser = userConfig+i;
 
  JSON_UsersSettings[i] = UsersSettings.createNestedObject();
  JSON_UsersSettings[i]["Name"] = pUser->name;
  
  if(pUser->mode == 0)
    JSON_UsersSettings[i]["DefaultThicknessMode"] = "normal";
  else 
    JSON_UsersSettings[i]["DefaultThicknessMode"] = "trim";
  
  JSON_UsersSettings[i]["thicknessNormal_um"] = pUser->thicknessNormalMode;
  JSON_UsersSettings[i]["thicknessTrimm_um"] = pUser->thicknessTrimmingMode;
  JSON_UsersSettings[i]["thresholdToRewind"] = pUser->thresholdToRewind;
  JSON_UsersSettings[i]["thresholdToCut"] = pUser->thresholdToCut;

  if(pUser->alarmState == 0)
      JSON_UsersSettings[i]["TempAlarmState"] = "off";
  else
      JSON_UsersSettings[i]["TempAlarmState"] = "on";

  JSON_UsersSettings[i]["TemperatureAlarm"] = pUser->tempAlarmDegree;

}
// Serialize to unformatted output (in line)
//serializeJson(JSONdoc, buffer);

// Serialize to formatted output
serializeJsonPretty(JSONdoc, buffer);

#ifdef SERIAL_DEBUG
Serial.write("Serialisation result\n----------------\n");
Serial.write(buffer);
#endif

if(SaveFileToSD(fileName, buffer) == 0)
  return 0;
else return -1;
}


/**
 * @brief Save the buffer to file on SD card
 * 
 * @param fileName pointer to apply
 * @param sourceBuffer pointer to write in file
 * @return int 
 */
int SaveFileToSD(char * fileName, char * sourceBuffer){
 // Create variable type FILE for file readind
File myFile;
SdFat SD;

  if (!SD.begin(SD_CS_PIN)) {
    #ifdef SERIAL_DEBUG
    Serial.println("initialization failed!");
    #endif
    return -1;
  }else{
    #ifdef SERIAL_DEBUG
    Serial.println("initialization done.");
    #endif

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    myFile = SD.open(fileName, FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile) {
      #ifdef SERIAL_DEBUG
      Serial.print("Writing to file...");
      #endif
      myFile.rewind();
      myFile.write(sourceBuffer);
      //myFile.println(buffer);
      
      // close the file:
      myFile.close();
      #ifdef SERIAL_DEBUG
      Serial.println("done.");
      #endif
      return 0;
    } else {
      // if the file didn't open, print an error:
      #ifdef SERIAL_DEBUG
      Serial.println("error opening file to save");
      #endif
      return -1;
    }    
  }
}