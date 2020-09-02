/**
 * @file main.cpp
 * @author Brayan Garcia  (brayan.grcmc@eduge.ch)
 * @brief Open a config file and parse the JSON data to a known structure
 *         Required Arduino libraries:
 *         - SDFAT (1.1.4) for SD Card IO
 *         - ArduinoJson (6.15.2) for JSON String parser
 * 
 * @version 0.1
 * @date 2020-07-20
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <Arduino.h>
#include <delay.h>
#include "cmu_ws_2004_01_V1_board.h"
#include <Wire.h> 
#include "LiquidCrystal_I2C.h"
#include "jsonConfigSDcard.h"
#include <SdFat.h>
#include "mcp230xx.h"

// Define the default motor speed and steps for run from BNC trigger
#define DEFAULT_MOTOR_SPEED 80
#define DEFAULT_MOTOR_STEPS 200
#define DEFAULT_MOTOR_SPEED_REVERSE -80
//config 
#define NORMAL_MODE 0
#define TRIMING_MODE 1
#define ALARM_OFF 0
#define ALARM_ON 1
#define NB_OF_USER 1
//DEFAULT
#define DEFAULT_THICKNESS_NORMAL_MODE 100
#define DEFAULT_THICKNESS_TRIM_MODE 150
#define DEFAULT_THRESHOLD_HIGH 800
#define DEFAULT_THRESHOLD_LOW 150

//Pinout 
#define ADC_POT A2
#define ADC_NTC A3
#define Buzzer A6
#define MCP23017_INTB 0
#define MCP23017_INTA 1
#define MCP23017_RESET 2
#define KNOB_CHANNEL_B 3
#define KNOB_CHANNEL_A 4
#define KNOB_SWITCH_A 5
#define PCA9629A_INT 6
#define DEBUG1 7
#define DEBUG2 8

// number of maximum users settings allocated
#define MAX_USER_SETTINGS 6

//Rotary knob 
#define CW -1
#define CCW 1
#define NO_ROTATION 0
#define LONG_PUSH -1
#define NO_PUSH 0
#define PUSH 1
#define LONG_PUSH_TIME 500
#define BOUNCE_ELIM_TIME 10

//POT
#define AVERAGE_SIZE 5

//Alarm
#define Alarm_OFF 0
#define Alarm_ON 1

//mode 
#define MODE_NORMAL 0
#define MODE_TRIMMING -1

//Menu index
#define MENU_MODE 0
#define MENU_THICKNESS 1
#define MENU_THRESHOLD 2
#define MENU_ALARM 3
#define EXIT 1

//MCP23017
#define FALLING_EDGE 1
#define RISING_EDGE 0


// LCD 
#define MAX_ROW_INDEX_LCD 3
#define FORCE 1

//SLICER
#define THICKNESS_MIN 1
#define THICKNESS_MAX 500
#define MODE_AUTO 1
#define MODE_MAN 0
//mcp23017
#define BTN_GRBTGL 0
#define BTN_RETREN 1
#define BTN_ROLL 2
#define BTN_AUTMAN 3
#define BTN_RES 4
#define SW_CALIBRATION 5
#define JOY_TRIM 8
#define JOY_GRBUP 9
#define JOY_STP 10
#define JOY_GRBDWN 11
#define LED_AUTO 12
#define LED_RETEN 13
#define LED_RETRA 14
#define LED_MAN 15
//alim
#define VCC 3.3
// Boards declaration
board_2004_01_V01 motor_2004_board; 
    
// Display declaration
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01 , 4, 5, 6, 7, 9, 10, 11, 12, POSITIVE); // set the LCD address to 0x27 for a 16 chars and 2 line display

// Create new variable for user config storage
SETTINGS userConfig[MAX_USER_SETTINGS];
SLICERCONFIG machineConfig;
device_mcp230xx mcp23017config= {"",0x24,0x0FFF,0X0000,0x0000,0x0FFF}; 

struct HOME
{
  unsigned int counterValue;
  unsigned int trimValue;
  unsigned int feedValue;

};

struct MENU
{
  HOME home;
  //IndexMenu indexArrow;
};
typedef struct t_NTCsensor{
  struct s_ntc_setting{
      int RThbeta=3435;  
      int RTh0=10000;
      int Th0=25;
      int RRef=9970;
  }settings;
  struct s_ntc_data{
      float RThValue=-1;  
      float Temp=-1;
  }measure;
}NTCsensor;
/*=======prototype function=====================*/

void MenuAlarmState();
void MenuAlarmSetting();
void MenuAlarm();
void MenuThresholdToCut();
void MenuThreshold();
void MenuThresholdToRewind();
void MenuThreshold();
void MenuThickness( );
void MenuMode();
void MenuBacklash();
void MenuSelectConfig();
void MenuUserConfig();
void MenuSlicerConfig();
void MenuThicknessNormal();
void MenuThicknessTrimming();
void MenuMotorSpeed();
void MenuSelectUser();
void ViewMenuUserConfig();
void ScreenThreshold();
void UserConfigScreenOne();
void UserConfigScreenTwo();
void backlashCwConfig();
void backlashCcwConfig();
void ArrowIndex(bool force);
void Home();
void knobRotationDetection();
void knobSwitchDetection();
void RemoveZero( int value, unsigned char colonne, unsigned char ligne);
void ThresholdDetection(SLICERCONFIG *machineConfig, SETTINGS *userSetting, unsigned int valPot);
unsigned int AverageAdc (unsigned int valAdc);
void SaveThreshold();
void ShowPot(unsigned char columns, unsigned char raw);
void PortInit();
void lcdClear();
void HomeScreen();
void TestSD();
void MotorHomingSpeed();
void MotorMovingSpeed();
void ModeAuto();
void ModeManu();
float calcNTCTemp(int UR10K, NTCsensor * NTC);
void GestionMesureTemp(int refresh);



//variable declaration 

String myString;
bool lastState;
int knobRotation;
int gknobPsuh;
unsigned int  gvalAdc;
unsigned int gvalAdcNtc;
int currentUser;
int arrowIndexRow=1;
int arrowOldPosition=MAX_ROW_INDEX_LCD;
bool gflagUpperMenu;
bool gflagLowerMenu;
int pas=10;
int oldRotation;
int gerr;
int gbtnBackPressed;
int gbtnAutoManPressed;
int gbtnjoygrbupPressed;
int gbtnjoygrdwnPressed;
int gSwCalibPressed;
int gbtnjoyStpPressed;
int gbtnjoyTrimPressed;
int gbtnResetPressed;
bool genRetractation=true;
int modeAutoMan=MODE_AUTO;
int gtemperatur;
int state;
byte retarrow[8] = {	0x10,0x10,0x14,0x16,0x1f,0x06,0x04};

HOME home = {0,0,0};
MENU menu;
NTCsensor ntcSensor;

// Arduino setup

void setup() {
  // init. port Arduino
  PortInit();
   
  //Interupt setting 
  attachInterrupt(digitalPinToInterrupt(KNOB_CHANNEL_A),knobRotationDetection, FALLING);
  attachInterrupt(digitalPinToInterrupt(KNOB_SWITCH_A),knobSwitchDetection, FALLING);
  //init. LCD
  lcd.begin(20,4);
  lcd.noDisplay();          
  lcd.display();
  //Cursor creation  
  lcd.createChar(0, retarrow);
  //init PCA9629A and Driver L298 
  motor_2004_board.begin();
  //Get the General Slicer Config object
  getGeneralSlicerConfig("config.cfg", &machineConfig);
  //Reset MCP23017
  digitalWrite(2,LOW);
  digitalWrite(2,HIGH);
  //MCP23017 config 
  gerr+=mcp23017_init(&mcp23017config);
  gerr+=mcp23017_setPort(&mcp23017config,0xFF);
  //display fixed text
  lcd.setCursor(0,0);
  lcd.print("    please wait    ");
  lcd.setCursor(0,1);
  lcd.print("         or        ");
  lcd.setCursor(0,2);
  lcd.print(" press knob button ");
  //waits for the user to press the knob button until the limit sensor is active 
  do
  {
    gSwCalibPressed = mcp230xx_getChannel(&mcp23017config,SW_CALIBRATION);
    motor_2004_board.stepperRotation(MOTOR_A,-(machineConfig.HomingSpeed),50);
  }while(gSwCalibPressed && gknobPsuh == NO_PUSH );
   gknobPsuh = NO_PUSH;
  lcdClear();
  //select User Menu 
  MenuSelectUser();
  //fixed text display home screen 
  HomeScreen();
 
 
}

void loop() {
  //allows you to enter the configuration menus
  if (gknobPsuh == LONG_PUSH)
  {
    gknobPsuh = NO_PUSH;
    //select config Menu  
    MenuSelectConfig();
    //fixed text display home screen 
    HomeScreen();
    //Saves the configuration to the MicroSD card
    saveUserAndGeneralSettings("config.cfg", &machineConfig, userConfig, MAX_USER_SETTINGS);    
  }
  else
  {
    //changes the values in the home screen
    Home();
    // read automatic/manual button 
    gbtnAutoManPressed= mcp230xx_getChannel(&mcp23017config,BTN_AUTMAN);
    //choose mode 
    if(!gbtnAutoManPressed && lastState != gbtnAutoManPressed)
    modeAutoMan = !modeAutoMan;
    lastState=gbtnAutoManPressed;
    if(modeAutoMan == MODE_AUTO)
    {
      ModeAuto();
    }
    else 
    {
      ModeManu();
    }
    //read continous up button
    gbtnjoygrbupPressed = mcp230xx_getChannel(&mcp23017config,JOY_GRBUP);
    if(gbtnjoygrbupPressed )
    {
      //continous up 
      do
      {
        //wait motor end move  
        if(motor_2004_board.getStepperState(MOTOR_A)==0)
        motor_2004_board.stepperRotation(MOTOR_A,machineConfig.HomingSpeed,50);
        gbtnjoygrbupPressed = mcp230xx_getChannel(&mcp23017config,JOY_GRBUP);
       state = motor_2004_board.getStepperState(MOTOR_A);
      
      }while (gbtnjoygrbupPressed );
    }
    //read continous down button 
    gbtnjoygrdwnPressed = mcp230xx_getChannel(&mcp23017config,JOY_GRBDWN);
    //read switch calibration 
    gSwCalibPressed = mcp230xx_getChannel(&mcp23017config,SW_CALIBRATION);

    if(gbtnjoygrdwnPressed && gSwCalibPressed)
    {
      //continous down 
      do
      {
        gbtnjoygrdwnPressed = mcp230xx_getChannel(&mcp23017config,JOY_GRBDWN);
        gSwCalibPressed = mcp230xx_getChannel(&mcp23017config,SW_CALIBRATION);
        //wait motor move
        if(motor_2004_board.getStepperState(MOTOR_A)==0)
          motor_2004_board.stepperRotation(MOTOR_A,-(machineConfig.HomingSpeed),50);
      }while(gbtnjoygrdwnPressed && gSwCalibPressed);
    }
    //read step button 
    gbtnjoyStpPressed = mcp230xx_getChannel(&mcp23017config,JOY_STP);
    //change mode 
    if(gbtnjoyStpPressed)
      userConfig[currentUser].mode = MODE_NORMAL;
    //read trim btton   
    gbtnjoyTrimPressed = mcp230xx_getChannel(&mcp23017config,JOY_TRIM);
    //change mode 
    if(gbtnjoyTrimPressed)
      userConfig[currentUser].mode = MODE_TRIMMING; 
    //temperature measurement management 
    GestionMesureTemp(1000);
    // reset counter value 
    gbtnResetPressed = mcp230xx_getChannel(&mcp23017config,BTN_RES);
    if(!gbtnResetPressed)
    {
      home.counterValue = 0;
    }
    //move motor whit knob 
    if(knobRotation == CW)
      motor_2004_board.stepperRotation(MOTOR_A,machineConfig.MovingSpeed,userConfig[currentUser].thicknessNormalMode);
    if(knobRotation == CCW && gSwCalibPressed)
    {
      if(motor_2004_board.getStepperState(MOTOR_A)==0)
        motor_2004_board.stepperRotation(MOTOR_A,-(machineConfig.MovingSpeed),userConfig[currentUser].thicknessNormalMode);
    }
    if(knobRotation != NO_ROTATION)
      knobRotation = NO_ROTATION;  
  }
}
/**
 * @brief  temperature measurement 
 * @param refresh interval between measurements [ms]
 */
void GestionMesureTemp(int refresh)
{ 
  unsigned int timer;
  static unsigned int oldTimer=0;
  static unsigned int oldTimerBuzzer=0;
  static unsigned int counterBip=0;
  static unsigned char flagTresholdTemp=0;
  timer=millis();
  //measures the temperature 
  if((timer-oldTimer)>=refresh)
  {
    oldTimer=timer;
    gvalAdcNtc = analogRead(ADC_NTC);
    gvalAdcNtc = ((3300* gvalAdcNtc)/1023);
    ntcSensor.measure.Temp = calcNTCTemp(gvalAdcNtc, &ntcSensor);
    ntcSensor.measure.Temp *=100;
    ntcSensor.measure.Temp = (int)ntcSensor.measure.Temp /10;
    ntcSensor.measure.Temp /=10;
  }
  //temperature threshold reached 
  if(flagTresholdTemp)
  {
    //activates the buzzer 
    if(userConfig[currentUser].tempAlarmDegree<0)
    {
      if(ntcSensor.measure.Temp>=userConfig[currentUser].tempAlarmDegree)
      {
        //switches on the buzzer one second three times every second  
        if(counterBip<3)
        {
          if((timer-oldTimerBuzzer)>=1000)
          { 
            oldTimerBuzzer=timer;
            digitalWrite(Buzzer,HIGH);
          }
          else 
          digitalWrite(Buzzer,LOW);
        }
        counterBip++;
        if(counterBip>=3)
          counterBip=4;       
      }
      else
      {
        counterBip=0;
        flagTresholdTemp=0;
      }   
    }
    if(userConfig[currentUser].tempAlarmDegree>0)
    {
      //activates the buzzer 
      if(ntcSensor.measure.Temp<=userConfig[currentUser].tempAlarmDegree)
      {
        //switches on the buzzer one second three times every second  
        if(counterBip<3)
        {
          if((timer-oldTimerBuzzer)>=1000)
          { 
            oldTimerBuzzer=timer;
            digitalWrite(Buzzer,HIGH);
          }
          else 
          digitalWrite(Buzzer,LOW);
        }
        counterBip++;
        if(counterBip>=3)
          counterBip=4;
      }
      else
      {
        counterBip=0;
        flagTresholdTemp=0;
      }     
    }
  }
  else 
  {
    if(userConfig[currentUser].tempAlarmDegree<0)
    {
      //temperature threshold reached 
      if(ntcSensor.measure.Temp<=userConfig[currentUser].tempAlarmDegree)
        flagTresholdTemp=1;
    }
    if(userConfig[currentUser].tempAlarmDegree>0)
    {
      //temperature threshold reached 
      if(ntcSensor.measure.Temp>=userConfig[currentUser].tempAlarmDegree)
        flagTresholdTemp=1; 
    }
  }   
}
/**
 * @brief Mode manual 
 * move up the specimen when the button is pressed
 */
void ModeManu()
{
  unsigned int thickness= userConfig[currentUser].thicknessNormalMode;
  static int odlState = 1;
  int btnPressed;
  int speed = machineConfig.MovingSpeed;
  //activates leds
  mcp230xx_setChannel(&mcp23017config,LED_AUTO,1);
  mcp230xx_setChannel(&mcp23017config,LED_MAN,0);
  //defined the cutting thickness 
  btnPressed=mcp230xx_getChannel(&mcp23017config,BTN_GRBTGL);
  if(userConfig[currentUser].mode == NORMAL_MODE)
    thickness = userConfig[currentUser].thicknessNormalMode;
  else 
    thickness = userConfig[currentUser].thicknessTrimmingMode;
  //move up specimen   
  if(!btnPressed && odlState!=btnPressed)
  {
    if(motor_2004_board.getStepperState(MOTOR_A)==0)
      motor_2004_board.stepperRotation(MOTOR_A, speed, thickness);
    home.counterValue++;
  }
  odlState=btnPressed;
}
/**
 * @brief Mode automatic
 * the slicer goes down after cutting the specimen 
 */
void ModeAuto()
{
  int btnPressed;
  static int odlState = 1;
  //activates the automatic led 
  mcp230xx_setChannel(&mcp23017config,LED_AUTO,0);
  mcp230xx_setChannel(&mcp23017config,LED_MAN,1);
  //active or not led retraction function  
  if(genRetractation)
    mcp230xx_setChannel(&mcp23017config,LED_RETEN,0);
  //activate or not the retraction function   
  btnPressed= mcp230xx_getChannel(&mcp23017config,BTN_RETREN);
  if(!btnPressed && odlState!=btnPressed)
  {
    genRetractation = !genRetractation;
  } 
  odlState=btnPressed;
  //determines the blade position
  gvalAdc = analogRead(ADC_POT);
  //detects thresholds 
  ThresholdDetection(&machineConfig, &userConfig[currentUser], gvalAdc);
}
/**
 * @brief Raises or lowers the platform depending on the position of the blade
 * @param machineConfig 
 * @param userSetting 
 * @param valPot 
 */
void ThresholdDetection(SLICERCONFIG *machineConfig, SETTINGS *userSetting, unsigned int valPot)
{
  static unsigned char step=4;
  static int speed = machineConfig->MovingSpeed;
  static unsigned int thresholdToCut = userSetting->thresholdToCut;
  static unsigned int thresholdToRewind = userSetting->thresholdToRewind;
  unsigned int backlash = machineConfig->BacklashCW;
  unsigned int thickness;

  if(userSetting->mode == NORMAL_MODE)
    thickness = userSetting->thicknessNormalMode;
  else 
    thickness = userSetting->thicknessTrimmingMode;
  if(thresholdToCut != userSetting->thresholdToCut)
  {
    thresholdToCut = userSetting->thresholdToCut;
    step=4;
  }
  if(thresholdToRewind != userSetting->thresholdToRewind)
  {
    thresholdToRewind = userSetting->thresholdToRewind;
    step=4;
  }    
  switch (step)
  {
    /*
    case 0:// première passe
      if(valPot <= thresholdToCut)
        step=1;
      break;*/
    case 1://Attendre d'arriver au trheshold to rewind 
     
      if(valPot >= thresholdToRewind)
      {       
          if(!genRetractation)
            step=3;
          else 
            step=2;
        home.counterValue++;
      }
      
      break;
    case 2://Descendre plateau
       gSwCalibPressed = mcp230xx_getChannel(&mcp23017config,SW_CALIBRATION);
       if(gSwCalibPressed)
       {
         
          if(speed>0)
          {
            backlash = machineConfig->BacklashCCW;
            speed = (-1)*speed;
          }
          else 
          {
            backlash=0;
          }
          motor_2004_board.stepperRotation(MOTOR_A, speed, thickness+backlash);         
       }
        step=3;
      break;
    case 3://Allumer led retra 
         if(motor_2004_board.getStepperState(MOTOR_A)==0)//motor stopped
         {
            //mcp230xx_setChannel(&mcp23017config,LED_RETRA,0);  
            step=4;
         }
      break;
    case 4://Attendre d'atteindre le threshold to cut  
       mcp230xx_setChannel(&mcp23017config,LED_RETRA,0);  
      if(valPot <= thresholdToCut)
      {     
          step=5;
      }
      break;
    case 5://éteindre led 
       mcp230xx_setChannel(&mcp23017config,LED_RETRA,1);
       step=6;
      break;
    case 6:// Monter plateau 
       
          if(speed<0)
          {
            speed = (-1)*speed;
            backlash = machineConfig->BacklashCW;
          }
          else 
          {
            backlash=0;
          }
          if(!genRetractation)
          {
            speed=machineConfig->MovingSpeed;
            home.counterValue++;
          }
          else
          {        
            thickness+=thickness;
          }
          motor_2004_board.stepperRotation(MOTOR_A, speed, thickness+backlash);
          step=7;
      break;
      case 7:
       if(motor_2004_board.getStepperState(MOTOR_A)==0)//motor stopped
        {  
          step=1;
        }
      break;
    default:
      
      break;
  }

}
/**
 * @brief Setup selection menu
 * allows us to choose the configuration we want to modify between slicer, current user and change user 
 */
void MenuSelectConfig()
{
 static bool firstLoop=false;
  do
    {
      if(!firstLoop)
      {
        firstLoop = true;
        lcdClear();
        lcd.setCursor(1,0);
        lcd.print("---Select setting--");
        lcd.setCursor(1,1);
        lcd.print("User select");
        lcd.setCursor(1,2);
        lcd.print("Current user set.");
        lcd.setCursor(1,3);
        lcd.print("Slicer setting");
        arrowIndexRow =2;
        ArrowIndex(FORCE);
      }

      ArrowIndex(0);
      
      if(gknobPsuh == PUSH)
      {
        firstLoop=false;
        gknobPsuh = NO_PUSH;
        switch (arrowIndexRow)
        {
          case 1:
          MenuSelectUser();
          break;
          case 2: 
          MenuUserConfig();
          break;
          case 3:
          MenuSlicerConfig();
          break;
          default :
          break;
        }
      }
      gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
    }while(gknobPsuh != LONG_PUSH && gbtnBackPressed == 1);
    do
      gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
    while (!gbtnBackPressed);
    gknobPsuh = NO_PUSH;
    firstLoop=false;
}
/**
 * @brief Slicer setting configuration menu 
 * 
 */
void MenuSlicerConfig()
{
  static bool firstLoop=false;
  do
  {
    if(!firstLoop)
    {
      firstLoop = true;
      lcdClear();
      lcd.setCursor(1,0);
      lcd.print("---Slicer setting--");
      lcd.setCursor(1,1);
      lcd.print("Blacklash correct.");
      lcd.setCursor(1,2);
      lcd.print("Motor Speed");
      lcd.setCursor(1,3);
      lcd.print("NTC");
      arrowIndexRow =1;
      ArrowIndex(FORCE);
    }

    ArrowIndex(0);
      
    if(gknobPsuh == PUSH)
    {
      firstLoop=false;
      gknobPsuh = NO_PUSH;
      switch (arrowIndexRow)
      {
        case 1:
        MenuBacklash();
        break;
        case 2: 
        MenuMotorSpeed();
        break;
        case 3:
        MenuSlicerConfig();
        break;
        default :
        break;
      }
    }
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while(gknobPsuh != LONG_PUSH && gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  gknobPsuh = NO_PUSH;
  firstLoop=false;
}
/**
 * @brief motor backlash correction selection menu 
 * 
 */
void MenuBacklash()
{
   static bool firstLoop=false;
   do
    {
      if(!firstLoop)
      {
        firstLoop = true;
        lcdClear();
        lcd.setCursor(1,0);
        lcd.print("---Backlash set.--");
        lcd.setCursor(1,1);
        lcd.print("Blacklash CW");
        lcd.setCursor(1,2);
        lcd.print("Blacklash CCW");
        arrowIndexRow =1;
        ArrowIndex(FORCE);
      }

      ArrowIndex(0);
      
      if(gknobPsuh == PUSH)
      {
        firstLoop=false;
        gknobPsuh = NO_PUSH;
        switch (arrowIndexRow)
        {
          case 1:
          backlashCwConfig();
          break;
          case 2: 
          backlashCcwConfig();
          break;
          case 3:
          break;
          default :
          break;
        }
      }
      gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
    }while(gknobPsuh != LONG_PUSH&&gbtnBackPressed == 1 );
    do
      gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
    while (!gbtnBackPressed);
    gknobPsuh = NO_PUSH;
    firstLoop=false; 
}
/**
 * @brief motor backlash correction in counter wise  
 * 
 */
void backlashCwConfig()
{
  int backlashCw=machineConfig.BacklashCW;
  lcdClear();
  lcd.setCursor(0,0);
  lcd.print("--Backlash CW set.-");
  lcd.setCursor(1,2);
  lcd.print("Backlash = ");
  lcd.setCursor(12,2);
  lcd.print(machineConfig.BacklashCW);
  //allows you to change mode 
  do{
    if(knobRotation != NO_ROTATION)
    {
      if(knobRotation == CW)
      backlashCw++;
      else if (knobRotation == CCW)
       backlashCw--;
      if(backlashCw<0)
        backlashCw=0;
      if(knobRotation != NO_ROTATION)
      {
        knobRotation=NO_ROTATION;
        lcd.setCursor(12,2);
        lcd.print(backlashCw);
        RemoveZero(backlashCw,12,2);
      }
    }
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh!=PUSH && gbtnBackPressed == 1 );
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  gknobPsuh = NO_PUSH;
  machineConfig.BacklashCW = backlashCw;
}
/**
 * @brief motor backlash correction in contrary counter wise 
 * 
 */
void backlashCcwConfig()
{
  int backlashCcw=machineConfig.BacklashCCW;
  lcdClear();
  lcd.setCursor(0,0);
  lcd.print("-Backlash CCW set.-");
  lcd.setCursor(1,2);
  lcd.print("Backlash = ");
  lcd.setCursor(12,2);
  lcd.print(backlashCcw);
  //allows you to change backlash ccw 
  do{
    if(knobRotation != NO_ROTATION)
    {
      if(knobRotation == CW)
      backlashCcw++;
      else if (knobRotation == CCW)
       backlashCcw--;
      if(backlashCcw<0)
        backlashCcw=0;
      if(knobRotation != NO_ROTATION)
      {
        knobRotation=NO_ROTATION;
        lcd.setCursor(12,2);
        lcd.print(backlashCcw);
        RemoveZero(backlashCcw,12,2);
      }
    }
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh!=PUSH && gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  gknobPsuh = NO_PUSH;
  machineConfig.BacklashCCW = backlashCcw;
}
/**
 * @brief motor parameter selection menu 
 * 
 */
void MenuMotorSpeed()
{
   static bool firstLoop=false;
   do
    {
      if(!firstLoop)
      {
        firstLoop = true;
        lcdClear();
        lcd.setCursor(1,0);
        lcd.print("---Motor Speed--");
        lcd.setCursor(1,1);
        lcd.print("Homing Speed");
        lcd.setCursor(1,2);
        lcd.print("Moving Speed");
        arrowIndexRow =1;
        ArrowIndex(FORCE);
      }

      ArrowIndex(0);
      
      if(gknobPsuh == PUSH)
      {
        firstLoop=false;
        gknobPsuh = NO_PUSH;
        switch (arrowIndexRow)
        {
          case 1:
          MotorHomingSpeed();
          break;
          case 2: 
          MotorMovingSpeed();
          break;
          case 3:
          break;
          default :
          break;
        }
      }
      gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
    }while(gknobPsuh != LONG_PUSH && gbtnBackPressed == 1);
    do
      gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
    while (!gbtnBackPressed);
    gknobPsuh = NO_PUSH;
    firstLoop=false; 
}
/**
 * @brief allows the user to change the motor speed for continuous movement 
 * 
 */
void MotorHomingSpeed()
{
  int motorHomingSpeed=machineConfig.HomingSpeed;
  lcdClear();
  lcd.setCursor(0,0);
  lcd.print("Motor homing speed");
  lcd.setCursor(1,2);
  lcd.print("Motor speed = ");
  lcd.setCursor(15,2);
  lcd.print(motorHomingSpeed);
  //allows you to change motor Homing Speed 
  do{
    if(knobRotation != NO_ROTATION)
    {
      if(knobRotation == CW)
      motorHomingSpeed++;
      else if (knobRotation == CCW)
       motorHomingSpeed--;
      if(motorHomingSpeed>100)
        motorHomingSpeed=100;
      else if (motorHomingSpeed<0)
        motorHomingSpeed=0;
      if(knobRotation != NO_ROTATION)
      {
        knobRotation=NO_ROTATION;
        lcd.setCursor(15,2);
        lcd.print(motorHomingSpeed);
        RemoveZero(motorHomingSpeed,15,2);
      }
    }
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh!=PUSH && gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  gknobPsuh = NO_PUSH;
  machineConfig.HomingSpeed = motorHomingSpeed;
}
/**
 * @brief allows the user to change the speed of the motor for movement when cutting  
 * 
 */
void MotorMovingSpeed()
{
  int motorMovingSpeed=machineConfig.MovingSpeed;
  lcdClear();
  lcd.setCursor(0,0);
  lcd.print("Motor homing speed");
  lcd.setCursor(1,2);
  lcd.print("Motor speed = ");
  lcd.setCursor(15,2);
  lcd.print(motorMovingSpeed);
  //allows you to change motor Moving Speed 
  do{
    if(knobRotation != NO_ROTATION)
    {
      if(knobRotation == CW)
      motorMovingSpeed++;
      else if (knobRotation == CCW)
       motorMovingSpeed--;
      if(motorMovingSpeed>100)
        motorMovingSpeed=100;
      else if (motorMovingSpeed<0)
        motorMovingSpeed=0;
      if(knobRotation != NO_ROTATION)
      {
        knobRotation=NO_ROTATION;
        lcd.setCursor(15,2);
        lcd.print(motorMovingSpeed);
        RemoveZero(motorMovingSpeed,15,2);
      }
    }
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh!=PUSH&& gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  gknobPsuh = NO_PUSH;
  machineConfig.MovingSpeed = motorMovingSpeed;
}
/**
 * @brief user parameter selection menu 
 * 
 */
void MenuUserConfig()
{
  static int menuNumber;
  //refresh display
  ViewMenuUserConfig();
   do
    {
      //display cursor
      ArrowIndex(0);
      //refresh diplay
      if(gflagLowerMenu || gflagUpperMenu)
      {
        menuNumber = !menuNumber;
        ViewMenuUserConfig();
        
      }
      // select menu 
      if(gknobPsuh == PUSH)
      {
        gknobPsuh = NO_PUSH;
        if(!menuNumber)
        {
          switch (arrowIndexRow)
          {
            case 1:
            MenuMode();
            break;
            case 2: 
            MenuThickness();
            break;
            case 3:
            MenuThreshold();
            break;
            default :
            UserConfigScreenOne();
            break;
          }
        }
        else 
        {
          switch (arrowIndexRow)
          {
            case 1:
            MenuThickness();
            break;
            case 2:
            MenuThreshold();
            break;
            case 3:
            MenuAlarm();
            break;
            default:
            UserConfigScreenTwo();
            break;
          }
        }
        //refresh display 
        ViewMenuUserConfig();
        gknobPsuh = NO_PUSH;
      }
     //read back button   
     gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
    }while(gknobPsuh != LONG_PUSH && gbtnBackPressed == 1);
    //wait user release  bakc button 
    do
     gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
    while (!gbtnBackPressed);
    gknobPsuh = NO_PUSH;
}
/**
 * @brief change home screen values 
 * 
 * 
 */
void Home( )
{

 static unsigned int memoCntValue;
 static unsigned int memoFeedValue;
 static unsigned int memoTrimValue;
 static float memoTemperature;
 static int memoMode;
 
 if(memoFeedValue != userConfig[currentUser].thicknessNormalMode)
 {
   lcd.setCursor(5,1);
   lcd.print(userConfig[currentUser].thicknessNormalMode);
   RemoveZero(userConfig[currentUser].thicknessNormalMode,5,1);
 }
 if(memoTrimValue != userConfig[currentUser].thicknessTrimmingMode)
 {
   lcd.setCursor(15,1);
   lcd.print(userConfig[currentUser].thicknessTrimmingMode);
   RemoveZero(userConfig[currentUser].thicknessTrimmingMode,15,1);
 }
 if(memoCntValue != home.counterValue)
 {
   lcd.setCursor(8,3);
   lcd.print(home.counterValue);
   RemoveZero(home.counterValue,8,3);
 }
 if(memoTemperature != ntcSensor.measure.Temp)
 {
   if(ntcSensor.measure.Temp<=-71)
   {
     lcd.setCursor(6,2);
     lcd.print("     ");
     lcd.setCursor(6,2);
     lcd.print("--");
   }
   else
   {   
    lcd.setCursor(6,2);
    lcd.print("     ");
    lcd.setCursor(6,2);
    lcd.print(ntcSensor.measure.Temp,1);
   }
   //RemoveZero(ntcSensor.measure.Temp,7,2);
 }
 if(memoMode != userConfig[currentUser].mode)
 {
   lcd.setCursor(5,0);
   if(userConfig[currentUser].mode == MODE_NORMAL)
    lcd.print("Normal");
   else
    lcd.print("Trim.  ");
 }
 memoCntValue = home.counterValue;
 memoFeedValue = userConfig[currentUser].thicknessNormalMode;
 memoTrimValue = userConfig[currentUser].thicknessTrimmingMode;
 memoTemperature = ntcSensor.measure.Temp;
 memoMode = userConfig[currentUser].mode; 
}
/**
 * @brief fixed text display of the home screen
 * 
 */
void HomeScreen()
{
  lcdClear();
  lcd.setCursor(0,0);
  lcd.print("Mode=");
  if(userConfig[currentUser].mode == MODE_NORMAL)
    lcd.print("Normal");
  else 
    lcd.print("Trim.");
  lcd.setCursor(0,1);
  lcd.print("Feed=");
  lcd.print(userConfig[currentUser].thicknessNormalMode);
  lcd.setCursor(10,1);
  lcd.print("TRIM=");
  lcd.print(userConfig[currentUser].thicknessTrimmingMode);
  lcd.setCursor(0,3);
  lcd.print("Counter=");
  lcd.print(home.counterValue);
  lcd.setCursor(12,0);
  lcd.print(userConfig[currentUser].name);
  lcd.setCursor(0,2);
  lcd.print("Temp =      C");
  lcd.setCursor(6,2);
  lcd.print(ntcSensor.measure.Temp,1);
  //lcd.write(0xa1);
}
/**
 * @brief allows to change the fixed text between screen one and screen two of MenuUserConfig
 * 
 */
void ViewMenuUserConfig()
{
 // static bool firstLoop=false;
  static bool flagScreenTwo=false;
   
  if(gflagUpperMenu ||  gflagLowerMenu)
  {
    //firstLoop=false;
    flagScreenTwo= !flagScreenTwo;
  }
    
    //firstLoop=true;
    if(!flagScreenTwo)
    {
      UserConfigScreenOne();
    }
    else 
    {
      UserConfigScreenTwo();
    }
    ArrowIndex(FORCE);        
  
}
/**
 * @brief fixed text display menu user config screen one of MenuUserConfig
 * 
 */
void UserConfigScreenOne()
{
    lcdClear();
    lcd.home();
    lcd.setCursor(0,0);
    lcd.print("----user setting---");
    lcd.setCursor(1,1);
    lcd.print("Mode = ");
    if(userConfig[currentUser].mode==MODE_NORMAL)
    {
      lcd.print("NORMAL");
    }
    else
    {
       lcd.print("TRIMMING");
    }
    
    lcd.setCursor(1,2);
    lcd.print("Thickness");
    lcd.setCursor(1,3);
    lcd.print("Thresholds");
}
/**
 * @brief fixed text display menu user config screen two of MenuUserConfig
 * 
 */
void UserConfigScreenTwo()
{
    lcdClear();
    lcd.setCursor(0,0);
    lcd.print("----user setting---");
    lcd.setCursor(1,1);
    lcd.print("Thickness");
    lcd.setCursor(1,2);
    lcd.print("Thresholds");
    lcd.setCursor(1,3);
    lcd.print("Alarm = ");
    if(userConfig[currentUser].alarmState)
    {
      lcd.print("ON");
    }
    else 
    {
      lcd.print("OFF");
    }
  
}
/**
 * @brief fixed text display menu mode 
 * 
 * 
 */
void MenuMode()
{
  static bool toggle = true;
  lcdClear();
  lcd.setCursor(0,0);
  lcd.print("--------Mode------");
  lcd.setCursor(1,2);
  lcd.print("Mode : ");
  lcd.setCursor(8,2);
  if(userConfig[currentUser].mode == NORMAL_MODE)
  {
    lcd.print("Normal");
  }
  else
  {
    lcd.print("Triming");
  }
  //allows you to change mode 
  do{
    if(knobRotation != NO_ROTATION)
    {
      knobRotation = NO_ROTATION;
      toggle = !toggle;
      //changes without taking into account the direction 
      //of rotation of the encoder. 
      if(toggle)
      {
        myString = "Normal  ";
        userConfig[currentUser].mode = MODE_NORMAL;
      }
      else 
      {     
        myString = "Triming";
        userConfig[currentUser].mode = MODE_TRIMMING;
      }
      lcd.setCursor(8,2);
      lcd.print(myString);
    }
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh!=PUSH && gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);

}
/**
 * @brief Menu Select User, show basic information about 
 *        current user
 * 
 */
void MenuSelectUser()
{
  int screenNum=0;
  int timer;
  int oldTimer=0;
  int i; 
  lcd.setCursor(0,0);
  lcd.print("-----Select User----");
  lcd.setCursor(0,1);
  lcd.print("User : ");
  //loads the values of the different users
  for(i=0;i<MAX_USER_SETTINGS;i++)
  {
    getUserSettingsFromConfig("config.cfg", &userConfig[i], i);
  }
  lcd.print(userConfig[currentUser].name);
  do
  {
    timer= millis();
    //allows you to change users
    if(knobRotation == CW)
    {
      currentUser++;
    }
    else if(knobRotation == CCW)
    {
      currentUser--;
    }
    //security for not being off index
    if(currentUser<0)
    {
      currentUser = MAX_USER_SETTINGS-1;
    }
    else if(currentUser>MAX_USER_SETTINGS-1)
    {
      currentUser=0;
    }
    if(knobRotation!=NO_ROTATION)
    {
      
      getUserSettingsFromConfig("config.cfg", &userConfig[currentUser], currentUser);
      lcd.setCursor(7,1);
      lcd.print("             ");
      lcd.setCursor(7,1);
      lcd.print(userConfig[currentUser].name);
      knobRotation = NO_ROTATION;
      screenNum=0;
    }
    //allows the user's configuration to be displayed by scrolling on the screen.
    //Every second, the information shifts upwards. 
    if((timer-oldTimer)>=1000)
    {
      //deletes the lines 2 and 3 of the LCD 
      lcd.setCursor(0,2);
      lcd.print("                    ");
      lcd.setCursor(0,3);
      lcd.print("                    ");
      oldTimer=timer;
      switch (screenNum)
      {
        case 0:
          lcd.setCursor(0,2);
          lcd.print("Mode : ");
          if(userConfig[currentUser].mode==MODE_NORMAL)
          {
            lcd.print("Normal");
            
          }
          else 
          {
            lcd.print("Trimming");
            
          }
          lcd.setCursor(0,3);
          lcd.print("Thick. Nor. =     um");
          lcd.setCursor(14,3);
          lcd.print(userConfig[currentUser].thicknessNormalMode);
        break;
        case 1:
          lcd.setCursor(0,2);
          lcd.print("Thick. Nor. =     um");
          lcd.setCursor(14,2);
          lcd.print(userConfig[currentUser].thicknessNormalMode);
          lcd.setCursor(0,3);
          lcd.print("Thick. Tri. =     um");
          lcd.setCursor(14,3);
          lcd.print(userConfig[currentUser].thicknessTrimmingMode);
        break;
        case 2:
          lcd.setCursor(0,2);
          lcd.print("Thick. Tri. =     um");
          lcd.setCursor(14,2);
          lcd.print(userConfig[currentUser].thicknessTrimmingMode);
          lcd.setCursor(0,3);
          lcd.print("Thres. cut. =     ");
          lcd.setCursor(14,3);
          lcd.print(userConfig[currentUser].thresholdToCut);
        break;
        case 3:
          lcd.setCursor(0,2);
          lcd.print("Thres. cut. =     ");
          lcd.setCursor(14,2);
          lcd.print(userConfig[currentUser].thresholdToCut);
          lcd.setCursor(0,3);
          lcd.print("Thres. rew. =     ");
          lcd.setCursor(14,3);
          lcd.print(userConfig[currentUser].thresholdToRewind);
        break;
        case 4:
          lcd.setCursor(0,2);
          lcd.print("Thres. rew. =     ");
          lcd.setCursor(14,2);
          lcd.print(userConfig[currentUser].thresholdToRewind);
          lcd.setCursor(0,3);
          lcd.print("Alam : ");
          if(userConfig[currentUser].alarmState == ALARM_ON)
          {
            lcd.print("On");
          }
          else 
          {
            lcd.print("Off");
          }
        break;
        case 5:
          lcd.setCursor(0,2);
          lcd.print("Alam : ");
          if(userConfig[currentUser].alarmState == ALARM_ON)
          {
            lcd.print("On");
          }
          else 
          {
            lcd.print("Off");
          }
          lcd.setCursor(0,3);
          lcd.print("Mode : ");
          if(userConfig[currentUser].mode==MODE_NORMAL)
          {
            lcd.print("Normal");
          }
          else 
          {
            lcd.print("Trimming");
          }
        break;
        default:
          break;
      }
      screenNum++;
      if(screenNum>5)
      {
        screenNum=0; 
      }
    }
   gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);

  } while ( gbtnBackPressed && gknobPsuh != PUSH);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  gknobPsuh=NO_PUSH;
  lcdClear(); 
}
/**
 * @brief fixed text display menu Thickness 
 * 
 */
void MenuThickness( )
{
  lcdClear();
  arrowIndexRow=1;
  ArrowIndex(FORCE);
  lcd.setCursor(0,0);
  lcd.print("-----Thickness-----");

  lcd.setCursor(1,1);
  lcd.print("Normal  = ");
  lcd.print(userConfig[currentUser].thicknessNormalMode);
  lcd.print(" um");

  lcd.setCursor(1,2);
  lcd.print("Triming = ");
  lcd.print(userConfig[currentUser].thicknessTrimmingMode);
  lcd.print(" um");
  //allows you to choose which parameter to change 
  do
  {
      ArrowIndex(0);
     if(gknobPsuh == PUSH)
     {
       gknobPsuh=NO_PUSH;
       switch (arrowIndexRow)
       {
          case 1 :
          MenuThicknessNormal();
          break;
          case 2 : 
          MenuThicknessTrimming();
          break;
          default:
          break;
       }
      //fixed text display
      lcdClear();
      ArrowIndex(FORCE);
      lcd.setCursor(0,0);
      lcd.print("-----Thickness-----");

      lcd.setCursor(1,1);
      lcd.print("Normal  = ");
      lcd.print(userConfig[currentUser].thicknessNormalMode);
      lcd.print(" um");

      lcd.setCursor(1,2);
      lcd.print("Triming = ");
      lcd.print(userConfig[currentUser].thicknessTrimmingMode);
      lcd.print(" um");
     }
     gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh != LONG_PUSH&&gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
}
/**
 * @brief Normal mode thickness configuration 
 * 
 */
void MenuThicknessNormal()
{
  unsigned int thickness = userConfig[currentUser].thicknessNormalMode;
  lcdClear();
  arrowIndexRow=2;
  ArrowIndex(FORCE);
  lcd.setCursor(0,0);
  lcd.print("-Thickness normal--");
  lcd.setCursor(1,2);
  lcd.print("Thickness = ");
  lcd.print(thickness);
  lcd.setCursor(17,2);
  lcd.print("um");
  gknobPsuh=NO_PUSH;
  //stores the user's thicnkess normal mode value
  do 
  {
    if(knobRotation==CW)
      thickness++;
    else if(knobRotation==CCW)
      thickness--;
    if(thickness<THICKNESS_MIN)
      thickness=THICKNESS_MIN;
    if(thickness>THICKNESS_MAX)
      thickness=THICKNESS_MAX;   
    if(knobRotation!=NO_ROTATION)
    {
      knobRotation = NO_ROTATION;
      lcd.setCursor(13,2);
      lcd.print(thickness);
      RemoveZero(thickness,13,2);
      lcd.setCursor(17,2);
      lcd.print("um");
    }
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh!=PUSH && gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  gknobPsuh = NO_PUSH;
  userConfig[currentUser].thicknessNormalMode=thickness;
}
/**
 * @brief trimming mode thickness configuration 
 * 
 */
void MenuThicknessTrimming()
{
  unsigned int thickness = userConfig[currentUser].thicknessTrimmingMode;
  // display fiexd text 
  lcdClear();
  arrowIndexRow=2;
  ArrowIndex(FORCE);
  lcd.setCursor(0,0);
  lcd.print("-Thickness trimming");
  lcd.setCursor(1,2);
  lcd.print("Thickness = ");
  lcd.print(thickness);
  lcd.setCursor(17,2);
  lcd.print("um");
  gknobPsuh=NO_PUSH;
  //stores the user's thickness trimming mode value 
  do 
  {
    if(knobRotation==CW)
      thickness++;
    else if(knobRotation==CCW)
      thickness--;
    if(thickness<THICKNESS_MIN)
      thickness=THICKNESS_MIN;
    if(thickness>THICKNESS_MAX)
      thickness=THICKNESS_MAX;   
    if(knobRotation!=NO_ROTATION)
    {
      knobRotation = NO_ROTATION;
      lcd.setCursor(13,2);
      lcd.print(thickness);
      RemoveZero(thickness,13,2);
      lcd.setCursor(17,2);
      lcd.print("um");
    }
    //Read back button
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh!=PUSH && gbtnBackPressed == 1);
  do
  //wait user waits for the user to release the button 
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  gknobPsuh = NO_PUSH;
  userConfig[currentUser].thicknessTrimmingMode=thickness;
}
/**
 * @brief Selection menu for thresholds 
 * 
 */
void MenuThreshold()
{
  //fixed text display 
  lcdClear();
  arrowIndexRow=1;
  ArrowIndex(FORCE);
  lcd.setCursor(0,0);
  lcd.print("-----Threshold-----");

  lcd.setCursor(1,1);
  lcd.print("Threshold to cut");

  lcd.setCursor(1,2);
  lcd.print("Threshold to rewind");
  do
  {
      ArrowIndex(0);
     if(gknobPsuh == PUSH)
     {
       gknobPsuh=NO_PUSH;
       switch (arrowIndexRow)
       {
          case 1 :
          ScreenThreshold();
          MenuThresholdToCut();
          break;
          case 2 : 
          ScreenThreshold();
          MenuThresholdToRewind();
          break;
          default:
          break;
       }
      //fixed text display
      lcdClear();
      ArrowIndex(FORCE);
      lcd.setCursor(0,0);
      lcd.print("-----Threshold------");

      lcd.setCursor(1,1);
      lcd.print("Threshold to cut");

      lcd.setCursor(1,2);
      lcd.print("Threshold to revwind ");
     }
     gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh != LONG_PUSH && gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  
  
}
/**
 * @brief tells the user how to select thresholds 
 * 
 */
void ScreenThreshold()
{
  //tells the user how to select thresholds 
  lcdClear();
  lcd.setCursor(0,1);
  lcd.print("place the blade and");
  lcd.setCursor(0,2);
  lcd.print("Press validation");
  lcd.setCursor(0,3);
  lcd.print("button");
  do
  {
    delay(10);    
  }while(gknobPsuh != PUSH);
  gknobPsuh=NO_PUSH;
}
/**
 * @brief Threshold to cut setting menu 
 * 
 */
void MenuThresholdToCut()
{
  //fixed text display
  lcdClear();
  lcd.setCursor(0,1);
  lcd.write((byte)0);
  lcd.setCursor(0,0);
  lcd.print("--Threshold to cut--");
  lcd.setCursor(1,1);
  lcd.print("Position = ");
  gknobPsuh=NO_PUSH;
  //stores the user's threshold to cut value
  do 
  {
    //Reading the analog value of the position potentiometer
    gvalAdc = analogRead(ADC_POT);
    //displays the value of the potentiometer and after averaging 
    ShowPot(12,1);
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh!=PUSH && gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  //saves the current user value
  userConfig[currentUser].thresholdToCut = gvalAdc;
  gknobPsuh = NO_PUSH;
}
/**
 * @brief Threshold to Rewind setting menu 
 * 
 */
void MenuThresholdToRewind()
{
  //fixed text display
  lcdClear();
  lcd.setCursor(0,1);
  lcd.write((byte)0);
  lcd.setCursor(0,0);
  lcd.print(" Threshold to rewind");
  lcd.setCursor(1,1);
  lcd.print("Position = ");
  gknobPsuh=NO_PUSH;
  //stores the user's threshold to cut value
  do 
  {
    //Reading the analog value of the position potentiometer
    gvalAdc = analogRead(ADC_POT);
    //displays the value of the potentiometer and after averaging 
    ShowPot(12,1);
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh!=PUSH && gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  //saves the current user value
   userConfig[currentUser].thresholdToRewind = gvalAdc;
  gknobPsuh=NO_PUSH;
}
/**
 * @brief Alarm setup menu
 * 
 */
void MenuAlarm()
{
  lcdClear();
  arrowIndexRow=1;
  ArrowIndex(FORCE);
  lcd.setCursor(0,0);
  lcd.print("-------Alarm--------");

  lcd.setCursor(1,1);
  lcd.print("Alarm State : ");
  if(userConfig[currentUser].alarmState)
  {
    lcd.print("ON");
  }
  else 
  {
    lcd.print("OFF");
  }
  lcd.setCursor(1,2);
  lcd.print("Alarm Setting");
  do
  {
      ArrowIndex(0);
     if(gknobPsuh == PUSH)
     {
       gknobPsuh=NO_PUSH;
       switch (arrowIndexRow)
       {
          case 1 :
          MenuAlarmState();
          break;
          case 2 : 
          MenuAlarmSetting();
          break;
          default:
          break;
       }
      //fixed text display
      lcdClear();
      arrowIndexRow=1;
      ArrowIndex(FORCE);
      lcd.setCursor(0,0);
      lcd.print("-------Alarm--------");

      lcd.setCursor(1,1);
      lcd.print("Alarm State : ");
      if(userConfig[currentUser].alarmState)
      {
        lcd.print("ON");
      }
      else 
      {
        lcd.print("OFF");
      }
      lcd.setCursor(1,2);
      lcd.print("Alarm Setting");
     }
     gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh != LONG_PUSH && gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
}
/**
 * @brief configures the alarm status 
 * 
 */
void MenuAlarmState()
{
  static bool toggle = true;
  gknobPsuh=NO_PUSH;
  //displays fixed text 
  lcdClear();
  lcd.setCursor(0,2);
  lcd.write((byte)0);
  lcd.setCursor(0,0);
  lcd.print("-------Alarm--------");
  lcd.setCursor(1,2);
  lcd.print("Alarm =");
  lcd.setCursor(9,2);
  if(userConfig[currentUser].alarmState)
  {
    lcd.print("ON");
  }
  else 
  {
    lcd.print("OFF");
  }
  
  do{
  //activates or deactivates the temperature alarmState 
    if(knobRotation != NO_ROTATION)
    {
      knobRotation = NO_ROTATION;
      toggle = !toggle;
      //changes without taking into account the direction 
      //of rotation of the encoder. 
      if(toggle)
      {
        myString = "ON ";
        userConfig[currentUser].alarmState = ALARM_ON;
      }
      else 
      {     
        myString = "OFF";
        userConfig[currentUser].alarmState = Alarm_OFF;
      }
      lcd.setCursor(9,2);
      lcd.print(myString);
    }
    //Read back button 
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh != PUSH&&gbtnBackPressed == 1);
  //read back button 
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  gknobPsuh =NO_PUSH;
}
/**
 * @brief Alarm temperatur setting
 * 
 */
void MenuAlarmSetting()
{
  int tempAlarmDegree= userConfig[currentUser].tempAlarmDegree;
  lcdClear();
  lcd.setCursor(0,2);
  lcd.write((byte)0);
  lcd.setCursor(1,0);
  lcd.print("---Temp. setting--");
  lcd.setCursor(1,2);
  lcd.print("Temp. = ");
  lcd.print(userConfig[currentUser].tempAlarmDegree);
  lcd.setCursor(9,2);
  do
  {
    if(knobRotation == CW)
      tempAlarmDegree++;
    else if (knobRotation == CCW)
      tempAlarmDegree--;
    if(knobRotation != NO_ROTATION)
    {
      knobRotation=NO_ROTATION;
      lcd.setCursor(9,2);
      lcd.print(tempAlarmDegree);
      RemoveZero(tempAlarmDegree,9,2);
    }
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  }while (gknobPsuh != PUSH && gbtnBackPressed == 1);
  do
    gbtnBackPressed =  mcp230xx_getChannel(&mcp23017config,BTN_ROLL);
  while (!gbtnBackPressed);
  gknobPsuh = NO_PUSH;
  userConfig[currentUser].tempAlarmDegree = tempAlarmDegree;
}
/**
 * @brief calculates an average over 5 values of the analog converter.
 * @param valAdc ADC value
 * @return unsigned int Avaraged ADC value
 */
unsigned int AverageAdc (unsigned int valAdc)
{
  unsigned int averageTempo;
  static unsigned int cntLoop=0;
  static unsigned int tabValAdc[AVERAGE_SIZE];
  unsigned int average;
  unsigned char i;
  
  averageTempo=0;
  //stock the last 5 values 
  tabValAdc[cntLoop++]=valAdc;
  //Reset cntLoop
  if(cntLoop>=AVERAGE_SIZE)
  {    
    cntLoop=0;
  }
  //avarages
  for( i=0;i<AVERAGE_SIZE;i++)
  {
    averageTempo += tabValAdc[i];
  }
  average = averageTempo/AVERAGE_SIZE;
  return average;
}
/**
 * @brief  removes the remaining zeros from the display 
 * 
 * @param value 
 * @param column 
 * @param raw 
 */
void RemoveZero( int value, unsigned char column, unsigned char raw)
{
   if(value<0)
   {
     value *= -1;
     column += 1;
   }
      
   if(value<=999)
   {
     lcd.setCursor (column+3,raw);
     lcd.print(" ");
   }
   if(value<=99)
   {
     lcd.setCursor (column+2,raw);
     lcd.print(" ");
   }
    if(value<=9)
   {
     lcd.setCursor (column+1,raw);
     lcd.print(" ");
   }
}
/**
 * @brief determines the direction of rotation 
 *        of the rotary encoder 
 */
void knobRotationDetection()
{
  bool knobChannelA;
  bool knobChannelB;
  
  //input reading 
  knobChannelA = digitalRead (KNOB_CHANNEL_A);
  knobChannelB = digitalRead (KNOB_CHANNEL_B); 
  //Clock wise 
  if(knobChannelA == knobChannelB)
  {   knobRotation = CW;}
  //contrary clock wise 
  else
  { knobRotation = CCW;}
}
/**
 * @brief Detects whether the button has been 
 *        pressed for a short or long time
 */
void knobSwitchDetection()
{

  static bool edgeMemo=0;
  unsigned long timer;
  static unsigned long timerMemo;
  //reverses the interruption edge
  edgeMemo =!edgeMemo;

  timer = millis();
  //button press 
  if(edgeMemo)
  {
    attachInterrupt(digitalPinToInterrupt(KNOB_SWITCH_A),knobSwitchDetection, RISING);
    //saves the time "t" at which the button was pressed 
     timerMemo=timer;  
  }
  //Button release
  else 
  {
     attachInterrupt(digitalPinToInterrupt(KNOB_SWITCH_A),knobSwitchDetection, FALLING);
      //test if the button has been pressed long enough
      if((timer-timerMemo) > LONG_PUSH_TIME)
      {
          gknobPsuh = LONG_PUSH;
      }
      //bounce eliminator
      else if((timer-timerMemo) > BOUNCE_ELIM_TIME)
      {
         gknobPsuh = PUSH;
      }
      
  }  
}
/**
 * @brief draws and moves the cursor on the LCD
 * 
 * @param force forces the cursor display 
 */
void ArrowIndex(bool force)
{
  gflagUpperMenu=false;
  gflagLowerMenu=false;
  //test if the encoder has rotated 
  if(knobRotation!=NO_ROTATION )
  {  
    //the encoder has turned clockwise 
    if(knobRotation==CW)
    { arrowIndexRow ++; }
    //the encoder has turned contrary clockwise 
    else if(knobRotation==CCW)
    { arrowIndexRow --; }
    //Test if the cursor index is higher than the number 
    //of lines on the screen to create a rotation effect.
    if(arrowIndexRow<1)
    { arrowIndexRow= MAX_ROW_INDEX_LCD; }
    else if (arrowIndexRow>MAX_ROW_INDEX_LCD)
    { arrowIndexRow=1; }
    //indicates that we have rotated up or down the screen
    if(arrowOldPosition==MAX_ROW_INDEX_LCD && arrowIndexRow==1 )
    {
      gflagUpperMenu=false;
      gflagLowerMenu=true;
    }
    else if (arrowOldPosition==1 && arrowIndexRow==MAX_ROW_INDEX_LCD)
    {
      gflagUpperMenu=true;
      gflagLowerMenu=false;
    }
    //displays the cursor at the new position 
    lcd.setCursor(0,arrowIndexRow);
    lcd.write((byte)0);
    //deletes the old cursor 
    lcd.setCursor(0,arrowOldPosition);
    lcd.print(" ");
    knobRotation=NO_ROTATION;
    arrowOldPosition=arrowIndexRow;
  }
  else if(force)
  {
    //forces the display of the cursor 
    lcd.setCursor(0,arrowIndexRow);
    lcd.write((byte)0);
    arrowOldPosition=arrowIndexRow;
  }
 
}
/**
 * @brief replaces the lcd.clear() function of the library 
 *        because it causes display problems.
 * 
 */
void lcdClear()
{
  lcd.setCursor(0,0);
  lcd.print("                    ");
  lcd.setCursor(0,1);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,3);
  lcd.print("                    ");
}
/**
 * @brief  read the value of the potentiometer and display it 
 * 
 * @param columns 
 * @param raw 
 */
void ShowPot(unsigned char columns, unsigned char raw)
{  
  gvalAdc = analogRead(ADC_POT);
  //calculates the average of the adc values
  gvalAdc = AverageAdc(gvalAdc);
  //convert to string 
  myString = String (gvalAdc);
  lcd.setCursor (columns,raw);
  //Serial.print(columns);
  //lcd.print(myString);
  lcd.print(myString);
  RemoveZero(gvalAdc,columns,raw);
}
/**
 * @brief Calculation of NTC température
 * 
 * @param UR10K Ref. Resistor input voltage
 * @param NTC NTC circuit settings (RTh beta, RTh@0degree, RRef value)
 * @return float Temperature in degree C
 */
float calcNTCTemp(int UR10K, NTCsensor * NTC){

  //Variables
  float RT, ln, TX, T0, Rvoltage;

  T0 = NTC->settings.Th0 + 273.15; //Temperature T0 from datasheet, conversion from Celsius to kelvin
  Rvoltage = (float)UR10K/1000.0;
  RT = (VCC - Rvoltage) / (Rvoltage/NTC->settings.RRef);
  NTC->measure.RThValue=RT;
  ln = log(RT / NTC->settings.RTh0);
  TX = (1 / ((ln / NTC->settings.RThbeta) + (1 / T0))); //Temperature from thermistor
  TX = TX - 273.15; //Conversion to Celsius
  NTC->measure.Temp=TX;
  return (TX);

}
/**
 * @brief Port init Arduino MKRZERO
 * 
 */
void PortInit()
{
  pinMode(ADC_POT,INPUT);
  pinMode(ADC_NTC,INPUT);
  pinMode(Buzzer,OUTPUT);
  pinMode(MCP23017_INTB,INPUT);
  pinMode(MCP23017_INTA,INPUT);
  pinMode(MCP23017_RESET,OUTPUT);
  pinMode(KNOB_CHANNEL_B,INPUT);
  pinMode(KNOB_CHANNEL_A,INPUT);
  pinMode(KNOB_SWITCH_A,INPUT);
  pinMode(PCA9629A_INT,INPUT);
  pinMode(DEBUG1,OUTPUT);
  pinMode(DEBUG2,INPUT);
  pinMode(SDA,INPUT);
  pinMode(SCL,INPUT);
  
}  
#ifdef SERIAL_DEBUG
void TestSD()
{
  Serial.begin(9600);
  while (!Serial) {
     // wait for serial port to connect. Needed for native USB port only
  }

  int i;
  int error=0;

  // Get the data config for each user
  for(i=0;i<MAX_USER_SETTINGS;i++){
    error+= getUserSettingsFromConfig("config.cfg", &userConfig[i], i);
  }

  // Get the slicer general configuration
  if(error == 0){
    getGeneralSlicerConfig("config.cfg", &machineConfig);
    Serial.write("Users settings loaded !");

  userConfig[0].thresholdToCut=1000;

  // Try to save general machine config and users settings as file if no error (>=0)
  if(saveUserAndGeneralSettings("test.cfg", &machineConfig, userConfig, MAX_USER_SETTINGS) >= 0)
    Serial.write("Saving file done !");
  else Serial.write("Saving file ERROR !");
  }
}
#endif