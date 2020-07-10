#include <Arduino.h>
#include <Wire.h>
#include "CONFIG_2004_01_V1.h"
#include "cmu_ws_2004_01_V1_board.h"


board_2004_01_V01::board_2004_01_V01(void){
    Wire.begin();                       // Initiate the Wire library for I2C

    CHANNEL_A_MOTOR.deviceAddress = IC1_PCA9629A_ADR;
    CHANNEL_A_MOTOR.pulsesWidth_ms = IC1_PULSE_WIDTH_MS;
    CHANNEL_A_MOTOR.bipolar_mode = IC1_MOTOR_MODE_BIPOLAR;
}

void board_2004_01_V01::begin(void){
    pca9629_init(&CHANNEL_A_MOTOR);
	
	// Config P2 & P3 for L298P Enable 
	PCA9629_GPIOConfig(&CHANNEL_A_MOTOR, 0xC3);
}

void  board_2004_01_V01::stepperRotation(char motor, char speed, int steps){
    int direction;
    device_pca9629 *selectedMotor;

    if(speed > 0)
        direction = 1;
    else 
        if(speed < 0){
            direction = -1;
            speed*=-1;
        }

    switch (motor){
        case MOTOR_A: selectedMotor = &CHANNEL_A_MOTOR; break;
        default: selectedMotor = &CHANNEL_A_MOTOR; break;
    }

    actuator_setStepperSpeed(selectedMotor, speed);
    actuator_setStepperStepAction(selectedMotor, direction, steps);
}


int  board_2004_01_V01::getStepperState(unsigned char motorNumber){
    device_pca9629 *selectedMotor;

    switch (motorNumber){
    case 0: selectedMotor = &CHANNEL_A_MOTOR; break;
    default: selectedMotor = &CHANNEL_A_MOTOR; break;
    }

    int state = (actuator_getStepperState(selectedMotor) & 0x80);
    return state;
}

int  board_2004_01_V01::setStepperDriveMode(char motorNumber, unsigned char driveMode){

    device_pca9629 *selectedMotor;

    if(driveMode == 0)
        driveMode = 1;      // Two phase drive (full step)
    else driveMode = 2;     // Half step drive

    switch (motorNumber){
    case 0: selectedMotor = &CHANNEL_A_MOTOR; break;
    default: selectedMotor = &CHANNEL_A_MOTOR; break;
    }

    actuator_setStepperDriveMode(selectedMotor, driveMode);
    return (0);
}