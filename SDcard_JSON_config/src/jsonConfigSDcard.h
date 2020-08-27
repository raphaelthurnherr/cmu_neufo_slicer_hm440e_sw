/**
 * @file jsonConfigSDcard.h
 * @author Raphael Thurnherr (raphael.thurnherr@unige.ch)
 * @brief These functions can open a file, deserialize the JSON data and put the values to a defined stucture in header file
 * @version 0.2
 * @date 2020-08-06
 * 
 * @remark 06.08.2020 - Adding config for NTC resistor and Temperature alarm user Setting, change "tempAlarm" by "tempAlarmDegree"
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef jsonConfigSDcard_h
#define jsonConfigSDcard_h

#define FILE_BUFFER_SIZE 2048

//#define SERIAL_DEBUG

// Structure definition for application and data config
typedef struct USERS_SETTINGS {
    char name[16];
    unsigned char mode;
    unsigned int thicknessNormalMode;
    unsigned int thicknessTrimmingMode;
    unsigned int thresholdToRewind;
    unsigned int thresholdToCut;
    int tempAlarmDegree; 
    unsigned char alarmState; 
} SETTINGS;

// Structure definition for application and data config
typedef struct SLICER_CONFIG {
    unsigned int BacklashCW;
    unsigned int BacklashCCW;
    unsigned char HomingSpeed;
    unsigned char MovingSpeed;
    unsigned char ScreenBacklight;
    struct t_NTCsensor{
            int RThbeta=3435;  
            int RTh0=10000;
            int Th0=25;
            int RRef=10000;
    }NTCsensor;

} SLICERCONFIG;

extern int getUserSettingsFromConfig(char * fileName, SETTINGS * userSetting, int configNb);
extern int getGeneralSlicerConfig(char * fileName, SLICERCONFIG *machineConfig);
extern int saveUserAndGeneralSettings(char * fileName, SLICERCONFIG * machineConfig, SETTINGS * userConfig,  unsigned char nbOfUserConfig);
#endif