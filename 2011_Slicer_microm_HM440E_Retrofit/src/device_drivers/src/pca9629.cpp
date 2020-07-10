/**
 * \file pca9629.h
 * \brief pca9629 Stepper motor drivers 
 * \author Raphael Thurnherr
 * \version 0.1
 * \date 09.06.2020
 *
 * Setup and drive stepper motors via PCA9629 IC
 * 
 */

#ifndef I2CSIMU

// REGISTER DEFINITION FOR PCA9629 IC
#include <Arduino.h>
#include "pca9629.h"
#include "arduino-i2c.h"


/**
 * \brief Initial configuration for Stepper motor controller
 * \param handler to PCA9629 configuration structure
 * \return code error
 */

int pca9629_init(device_pca9629 *pca9629config){
    unsigned char err=0;
    
    // CONFIGURATION DU CIRCUIT DRIVER MOTEUR PAS A PAS
    // bit 6 et 7 non utilisés dans les registres
    
    unsigned char devAddress = pca9629config->deviceAddress;
    
    unsigned char OP_CFG_PHS_DATA = 0x10;

    if(pca9629config->bipolar_mode)
        OP_CFG_PHS_DATA |= 0x40;
        
    err+= i2c_write(0, devAddress, 0x00, 0x20);    // MODE - Configuration du registre MODE (pin INT désactivée, Allcall Adr. désactivé)
    err+= i2c_write(0, devAddress, 0x01, 0xFF);    // WDTOI
    err+= i2c_write(0, devAddress, 0x02, 0x00);    // WDCNTL
    err+= i2c_write(0, devAddress, 0x03, 0x0F);    // IO_CFG
    err+= i2c_write(0, devAddress, 0x04, 0x10);    // INTMODE
    err+= i2c_write(0, devAddress, 0x05, 0x1F);    // MSK
    err+= i2c_write(0, devAddress, 0x06, 0x00);    // INTSTAT
    //err+= i2c_write(0, PCA9629, 0x07, 0x);    // IP
    err+= i2c_write(0, devAddress, 0x08, 0x00);    // INT_MTR_ACT
    err+= i2c_write(0, devAddress, 0x09, 0x00);    // EXTRASTEPS0
    err+= i2c_write(0, devAddress, 0x0A, 0x00);    // EXTRASTEPS1
    err+= i2c_write(0, devAddress, 0x0B, OP_CFG_PHS_DATA);    // OP_CFG_PHS
    //err+= i2c_write(0, devAddress, 0x0C, 0x00);    // OP_STAT_TO
    err+= i2c_write(0, devAddress, 0x0C, 0x0A);    // OP_STAT_TO
    err+= i2c_write(0, devAddress, 0x0D, 0x00);    // RUCNTL
    err+= i2c_write(0, devAddress, 0x0E, 0x00);    // RDCNTL
    err+= i2c_write(0, devAddress, 0x0F, 0x01);    // PMA - 0x01 Action unique, 0x00 action continue
    err+= i2c_write(0, devAddress, 0x10, 0x05);    // LOOPDLY_CW - Pour un delais de 20ms d'inversion de sens
    err+= i2c_write(0, devAddress, 0x11, 0x05);    // LOOPDLY_CCW Pour un delais de 20ms d'inversion de sens
    err+= i2c_write(0, devAddress, 0x12, 0xFF);    // CWSCOUNTL - Nombre de pas CW
    err+= i2c_write(0, devAddress, 0x13, 0xFF);    // CWSCOUNTH
    err+= i2c_write(0, devAddress, 0x14, 0xFF);    // CCWSCOUNTL - Nombre de pas CCW
    err+= i2c_write(0, devAddress, 0x15, 0xFF);    // CCWSCOUNTH
    err+= i2c_write(0, devAddress, 0x16, 0x4D);    // CWPWL - Vitesse / Largeur d'impulsion pour CW (1mS)
    err+= i2c_write(0, devAddress, 0x17, 0x01);    // CWPWH
    err+= i2c_write(0, devAddress, 0x18, 0x4D);    // CCWPWL - Vitesse / Largeur d'impulsion pour CCW (1mS)
    err+= i2c_write(0, devAddress, 0x19, 0x01);    // CCWPWH
    err+= i2c_write(0, devAddress, 0x1A, 0x00);    // MCNTL - Registre contrôle moteur
    err+= i2c_write(0, devAddress, 0x1B, 0xE2);    // SUBA1
    err+= i2c_write(0, devAddress, 0x1C, 0xE4);    // SUBA2
    err+= i2c_write(0, devAddress, 0x1D, 0xE8);    // SUBA3
    err+= i2c_write(0, devAddress, 0x1E, 0xE0);    // ALLCALLA
    //err+= i2c_write(0, PCA9629, 0x1F, 0x00);    // STEPCOUNT0
    //err+= i2c_write(0, PCA9629, 0x20, 0x00);    // STEPCOUNT1
    //err+= i2c_write(0, PCA9629, 0x21, 0x00);    // STEPCOUNT2
    //err+= i2c_write(0, PCA9629, 0x22, 0x00);    // STEPCOUNT3
   
    if(err){
     //   printf("Kehops I2C Step motor driver device initialization with %d error\n", err);
    }
    
    return err;        
}

/**
 * \brief pca9629_motorControl, Set the control register of motor
 * \return code error
 */

int PCA9629_StepperMotorControl(device_pca9629 *pca9629config, int data){
   	unsigned char err=0;

        unsigned char devAddress = pca9629config->deviceAddress;

        // Configuration du registre dans le sens horaire
        err += i2c_write(0, devAddress, 0x1a, data & 0x00FF);
        
        return(err);
}


/**
 * \brief int PCA9629_StepperMotorSetStep, Set the number of step to do in CW and CCW direction
 * \param handler to PCA9629 configuration structure
 * \return code error
 */

int PCA9629_StepperMotorSetStep(device_pca9629 *pca9629config, int stepCount){
   	unsigned char err=0;
       
        unsigned char devAddress = pca9629config->deviceAddress;

        // Configuration du registre de nombre de pas dans le sens horaire
        err += i2c_write(0, devAddress, 0x12, stepCount&0x00FF);           // Défini le nombre de pas dans le registre LOW
        err += i2c_write(0, devAddress, 0x13, (stepCount&0xFF00)>>8);    // Défini le nombre de pas dans le registre HIGH

        // Configuration du registre de nombre de pas dans le sens anti-horaire
        err += i2c_write(0, devAddress, 0x14, stepCount&0x00FF);           // Défini le nombre de pas dans le registre LOW
        err += i2c_write(0, devAddress, 0x15, (stepCount&0xFF00)>>8);    // Défini le nombre de pas dans le registre HIGH        

	return(err);
}

/**
 * \brief int PCA9629_ReadMotorState, Get the actual state of the motor
 * \param handler to PCA9629 configuration structure
 * \return code error
 */
int PCA9629_ReadMotorState(device_pca9629 *pca9629config){
   	unsigned char err=0;
    unsigned char regState;
    
    unsigned char devAddress = pca9629config->deviceAddress;

    // Lecture du registre de controle du driver moteur
    //err += i2c_readByte(0, devAddress, 0x1a, &regState);
    err += i2c_read(0, devAddress, 0x1a, &regState, 1);
    
    if(!err){    
        return regState;
	}else{
            //printf("PCA9629_ReadMotorState() -> Read error\n");
            return -1;
        }
}


/**
 * \brief int PCA9629_StepperMotorMode, Set the mode continuous or single action
 * \param handler to PCA9629 configuration structure
 * \return code error
 */

int PCA9629_StepperMotorMode(device_pca9629 *pca9629config, int data){
   	unsigned char err=0;
        
        unsigned char devAddress = pca9629config->deviceAddress;

        // Configuration du registre dans le sens horaire
        err += i2c_write(0, devAddress, 0x0f, data & 0x00FF);           // Défini le nombre de rotation dans le registre LOW    
        return(err);
}


/**
 * \brief int PCA9629_StepperMotorPulseWidth, Set the pulse width in CW and CCW direction
 * between 2mS (500Hz) and 22.5mS (44Hz)
 * \param handler to PCA9629 configuration structure
 * \return code error
 */

int PCA9629_StepperMotorPulseWidth(device_pca9629 *pca9629config, int data){
   	unsigned char err=0;

        unsigned char devAddress = pca9629config->deviceAddress;

        
        // Configuration du registre dans le sens horaire
        err+= i2c_write(0, devAddress, 0x16, data & 0x00FF);         // CWPWL - Vitesse / Largeur d'impulsion pour CW
        err+= i2c_write(0, devAddress, 0x17, (data & 0xFF00)>>8);    // CWPWH
        
        err+= i2c_write(0, devAddress, 0x18, data & 0x00FF);         // CCWPWL - Vitesse / Largeur d'impulsion pour CCW
        err+= i2c_write(0, devAddress, 0x19, (data & 0xFF00)>>8);    // CCWPWH
        return(err);
}

 int PCA9629_StepperDriveMode(device_pca9629 *pca9629config, unsigned char data){
   	unsigned char err=0;
    unsigned char devAddress = pca9629config->deviceAddress;

    data = ((data << 6) & (unsigned char)0xC0) | 0x10;

    err+= i2c_write(0, devAddress, 0x0B, data);         //  OP_CFG_PHS Change Phase control register and motor drive output
    return(err);
 }

  int PCA9629_GPIOConfig(device_pca9629 *pca9629config, unsigned char data){
   	unsigned char err=0;
    unsigned char devAddress = pca9629config->deviceAddress;

    //data = ((data << 6) & (unsigned char)0xC0) | 0x10;

    err+= i2c_write(0, devAddress, 0x03, data);         //  OP_CFG_PHS Change Phase control register and motor drive output
    return(err);
 }

/*
 * \fn char actuator_setStepperSpeed()
 * \brief Get the STEPPER hardware id of and setup the speed
 *
 * \param motorNumber, direction, stepCount
 * \return -
 */

int actuator_setStepperSpeed(device_pca9629 *pca9629config, int speed){
   long regData;
   float mappingResult;

        // V�rification ratio max et min comprise entre 0..100%
	if(speed > 100)
		speed = 100;
	if (speed<1)
		speed = 1;

    //MAPPING (0->100) to STEPPER_MAX_PULSEWIDTH_MS -> STEPPER_MIN_PULSEWIDTH_MS define in pca9629a.h
    mappingResult = (STEPPER_MIN_PULSEWIDTH_MS + ((STEPPER_MAX_PULSEWIDTH_MS-STEPPER_MIN_PULSEWIDTH_MS)/100.0)*(100-speed));
 

    // CALCUL FOR CONVERT xmS to register value  (3uS is PCA962+9a minimum pulse width)
    // ROUND(    (mS*1000)/(3uS*(2^PRESCALE VALUE))-1             )
     regData = (mappingResult * 1000.0)/(3*pow(2,PCA_9629A_CLK_PRESCALER_REGVALUE))-1;    
    
    PCA9629_StepperMotorPulseWidth(pca9629config, regData);
    return (1);
}


/*
 * \fn char actuator_getStepperState()
 * \brief Get the STEPPER state register
 *
 * \param motorNumber
 * \return -
 */

int actuator_getStepperState(device_pca9629 *pca9629config){
    int state=-1;    
  
    state = PCA9629_ReadMotorState(pca9629config);
    return state;
}


//
/**
 * \fn char actuator_setStepperStepAction()
 * \brief Get the STEPPER hardware id of and setup direction and step count to do
 *
 * \param motorNumber, direction, stepCount
 * \return -
 */

int actuator_setStepperStepAction(device_pca9629 *pca9629config, int direction, int stepCount){      
        unsigned char ctrlData = 0;
        unsigned char PMAmode = 0;
        switch(direction){
            case 1 :	ctrlData = 0x80; break;                         // CW
            case -1 :   ctrlData = 0x81; break;                     // CCW
            case 0 : 	ctrlData = 0x20; break;                         // Stop
            default :		     	break;
        }

        if(stepCount<=0)
            // Configuration du driver pour une rotation continue
           PMAmode = 0x00;
        else{
            // Configuration du driver pour une action unique
            PMAmode = 0x01;
            }    
        // Reset le registre de contronle
        // (Indispensable pour une nouvelle action après une action infinie)
        PCA9629_StepperMotorControl(pca9629config, 0x00);

        // Assignation du mode action continu ou unique
        PCA9629_StepperMotorMode(pca9629config, PMAmode);
        PCA9629_StepperMotorSetStep(pca9629config, stepCount);
        PCA9629_StepperMotorControl(pca9629config, ctrlData);            

    return (0);
} 

/**
 * @brief Set the driver mode for stepper motor (One phase, two phases or half-step)
 * 
 * @param pca9629config 
 * @param stepMode     0: one phase, 1, two phases, 3, two phases half-step, 
 * @return int nothing
 */
int actuator_setStepperDriveMode(device_pca9629 *pca9629config, unsigned char stepMode){
    int modeValue;

    switch(stepMode){
        case 0 : modeValue = 0x00; break;           // One phase
        case 1 : modeValue = 0x01; break;           // two phase
        case 2 : modeValue = 0x03; break;           // two phase half step
        default: modeValue = 0x01; break;
    }
    PCA9629_StepperDriveMode(pca9629config, modeValue);
    return (0);
}
#endif