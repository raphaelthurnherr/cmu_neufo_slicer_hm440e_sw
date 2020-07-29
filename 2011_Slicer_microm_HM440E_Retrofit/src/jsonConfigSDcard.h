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
    unsigned char alarm; 
} SETTINGS;

// Structure definition for application and data config
typedef struct SLICER_CONFIG {
    unsigned int BacklashCW;
    unsigned int BacklashCCW;
    unsigned char HomingSpeed;
    unsigned char MovingSpeed;
    unsigned char ScreenBacklight;
} SLICERCONFIG;

extern int getUserSettingsFromConfig(char * fileName, SETTINGS * userSetting, int configNb);
extern int getGeneralSlicerConfig(char * fileName, SLICERCONFIG *machineConfig);
extern int saveUserAndGeneralSettings(char * fileName, SLICERCONFIG * machineConfig, SETTINGS * userConfig,  unsigned char nbOfUserConfig);
#endif