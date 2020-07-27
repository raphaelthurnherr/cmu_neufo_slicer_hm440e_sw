#ifndef cmu_ws_2004_01_V1_board_h
#define cmu_ws_2004_01_V1_board_h

#include <Arduino.h>
//Low level device hardware library
#include "device_drivers/src/pca9629.h"

#define MOTOR_A 0

class board_2004_01_V01{
    public:
        board_2004_01_V01(void);
        void begin(void);
        int getStepperState(unsigned char motorNumber);
        void stepperRotation(char motor, char speed, int steps);
        int setStepperDriveMode(char motorNumber, unsigned char driveMode);

    protected:
    device_pca9629 CHANNEL_A_MOTOR;
};

#endif