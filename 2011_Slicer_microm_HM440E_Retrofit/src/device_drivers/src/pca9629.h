/**
 * \file pca9629.h
 * \brief pca9629 Stepper motor drivers 
 * \author Raphael Thurnherr
 * \version 0.1
 * \date 09.09.2020
 *
 * Setup and drive stepper motors via PCA9629 IC
 * 
 */


#ifndef PCA9629_H
#define PCA9629_H

#define MOTOR_CW 1
#define MOTOR_CCW -1
#define MOTOR_STOP 0

#define MOTOR_MODE_UNIPOLAR 0
#define MOTOR_MODE_BIPOLAR 1
#define MOTOR_MODE_BIPOLAR_HALF_STEP 2

#define STEPPER_MIN_PULSEWIDTH_MS     3
#define STEPPER_MAX_PULSEWIDTH_MS     10
// PCA_9629A_CLK_PRESCALER_REGVALUE -> (3 most significant bit) configured for run from STEPPER_MIN_PULSEWIDTH_MS to STEPPER_MAX_PULSEWIDTH_MS (PCA9629A datasheet sheet 27)
#define PCA_9629A_CLK_PRESCALER_REGVALUE  1     

/**
 * \struct device_pca9629 [pca9629.h] Configuration structure definition
 */

typedef struct pca9629{
    char deviceName[25];                        // Device Name of IC
    unsigned char deviceAddress;                // Bus device address
    float pulsesWidth_ms;               // Specify the pulse width for motor driving
    unsigned char bipolar_mode;
} device_pca9629;


/**
 * \brief Initial configuration for Stepper motor controller
 * \return code error
 */

extern int pca9629_init(device_pca9629 *pca9629config);



extern int PCA9629_StepperMotorControl(device_pca9629 *pca9629config, int data);
extern int PCA9629_StepperStepperMode(device_pca9629 *pca9629config, int mode);


extern int PCA9629_StepperMotorSetStep(device_pca9629 *pca9629config, int stepCount);         //Configuration du registre "PAS" du driver moteur
extern int PCA9629_StepperDriveMode(device_pca9629 *pca9629config, unsigned char data);       // Mode action continue ou unique
extern int PCA9629_StepperMotorPulseWidth(device_pca9629 *pca9629config, int data);           // Définition de la largeur d'impulstion
extern int PCA9629_ReadMotorState(device_pca9629 *pca9629config);                             // Lecture du registre de contrôle du moteur

extern int PCA9629_GPIOConfig(device_pca9629 *pca9629config, unsigned char data);             // Configuration du registre GPIO

/**
 * \brief pca9629_motorControl, Set the high level control of motor
 * \return code error
 */

extern int actuator_setStepperDriveMode(device_pca9629 *pca9629config, unsigned char stepMode);
extern int actuator_setStepperSpeed(device_pca9629 *pca9629config, int speed);
extern int actuator_setStepperStepAction(device_pca9629 *pca9629config, int direction, int stepCount);
extern int actuator_getStepperState(device_pca9629 *pca9629config);

#endif /* PCA9629_H */