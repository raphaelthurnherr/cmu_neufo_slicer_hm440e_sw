/**
 * \file mcp230xx.h
 * \brief MCP23008 and MCP23017 GPIO Extender drivers
 * \author Raphael Thurnherr
 * \version 0.1
 * \date 23.09.2019
 *
 * Library to setup and use the 8 channel MCP23008 GPIO extender I2C device
 * 
 */

#ifndef MCP230xx_H
#define MCP230xx_H

/**
 * \struct mcp23008 [mcp23008.h] Configuration structure definition
 */

typedef struct mcp230xx{
    char deviceName[25];                        // Device Name of IC
    unsigned char deviceAddress;                // Bus device address
    unsigned int gpioDirection;                // Specify the GPIO's port dirtection, 1 input, 0 output
    unsigned int invertedInput;               // >0, invert the output input level
    unsigned int pullupEnable;                 // The internal pullup 100k resistor configuration, 1 enable, 0 disable.
} device_mcp230xx;

/**
 * \brief MCP23008 driver initialization
 * \param pointer on the configuration structure
 * \return code error
 */
extern int mcp23008_init(device_mcp230xx *mcp23008config);        // MCP23008 driver initialization

/**
 * \brief MCP23017 driver initialization
 * \param pointer on the configuration structure
 * \return code error
 */
extern int mcp23017_init(device_mcp230xx *mcp23017config);        // MCP23017 driver initialization


/**
 * \brief MCP23008 read input state on specified input channel
 * \param pointer on the configuration structure
 * \return code error
 */
extern int mcp23008_getChannel(device_mcp230xx *mcp23008config, unsigned char channel);


/**
 * \brief MCP23008 set output state on specified input channel, the function read port state before rewrite
 * \param pointer on the configuration structure
 * \return code error
 */
extern int mcp23008_setChannel(device_mcp230xx *mcp23008config, unsigned char channel, unsigned char state);

/**
 * \brief MCP23008 set output values on all GPIO port bits
 * \param pointer on the configuration structure
 * \param value, value to apply on outputs
 * \return code error
 */
extern int mcp23008_setPort(device_mcp230xx *mcp23008config, unsigned char value);

/**
 * \brief MCP23008 get input value on GPIO port
 * \param pointer on the configuration structure
 * \return code error
 */
extern int mcp23008_getPort(device_mcp230xx *mcp23008config);



extern int actuator_getDigitalInput(device_mcp230xx *mcp230xxconfig, unsigned char channel);
extern char actuator_setDigitalOutput(device_mcp230xx *mcp230xxconfig, unsigned char channel, unsigned char state);

#endif /* MCP230xx_H */