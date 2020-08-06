/**
 * @file main.cpp
 * @author Raphael Thurnherr (raphael.thurnherr@unige.ch)
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
#define BTN_GRBTGL 6
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

// LCD 
#define MAX_ROW_INDEX_LCD 3
#define FORCE 1
// Boards declaration
board_2004_01_V01 motor_2004_board; 
    
// Display declaration
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01 , 4, 5, 6, 7, 9, 10, 11, 12, POSITIVE); // set the LCD address to 0x27 for a 16 chars and 2 line display

// Create new variable for user config storage
SETTINGS userConfig[MAX_USER_SETTINGS];
SLICERCONFIG machineConfig;

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

/*=======prototype function=====================*/

void MenuAlarmState();
void MenuAlarmSetting();
void MenuAlarm();
void MenuThresholdToCut();
void ScreenThreshold();
void MenuThreshold();
void MenuThresholdToRewind();
void ScreenThreshold();
void MenuThreshold();
void MenuThickness( );
void MenuMode();
void MenuSelectConfig();
void MenuUserConfig();
void ArrowIndex(bool force);
void ViewMenuUserConfig();
void UserConfigScreenOne();
void UserConfigScreenTwo();
void knobRotationDetection();
void knobSwitchDetection();
void RemoveZero( int value, unsigned char colonne, unsigned char ligne);
void ThresholdDetection(unsigned int value);
unsigned int AverageAdc (unsigned int valAdc);
void SaveThreshold();
void Home();
void ShowPot(unsigned char columns, unsigned char raw);
void PortInit();
void lcdClear();
void MenuSelectUser();
void HomeScreen();





//variable declaration 

String myString;
unsigned char counter;
bool lastState;
int knobRotation;
int knobPsuh;
unsigned int  valAdc;
unsigned int  moyenne; 
unsigned int thresholdHigh =900;
unsigned int thresholdLow =600;
bool swEncodeur;
int currentUser;
int arrowIndexRow=1;
int arrowOldPosition=MAX_ROW_INDEX_LCD;
bool flagUpperMenu;
bool flagLowerMenu;
int i;
int pas=10;
int speed;
byte retarrow[8] = {	0x10,0x10,0x14,0x16,0x1f,0x06,0x04};

HOME home = {0,0,0};
MENU menu;


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
  delay(100);
  //select User Menu 
  MenuSelectUser();
  //fixed text display home screen 
  HomeScreen();
}

void loop() {
  //allows you to enter the configuration menus 
  if (knobPsuh == LONG_PUSH)
  {
    knobPsuh = NO_PUSH;
    //select User Menu  
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

    if(knobRotation==CW)
    {
      speed = 90;
      
       
       motor_2004_board.stepperRotation(MOTOR_A, speed, 200);
      
       home.counterValue +=1; 
      
    }
    else if(knobRotation==CCW)
    {
      speed = -90;
     
      
      
      motor_2004_board.stepperRotation(MOTOR_A, speed, 200);
      
      home.counterValue -=1;
    
      
    }
    if(knobRotation!=NO_ROTATION)
    {
      knobRotation = NO_ROTATION;
    }
  }
 
  
 
 
}


/**
 * @brief Raises or lowers the platform depending on the position of the blade.
 * @param value 
 */
void  ThresholdDetection(unsigned int value)
{
  static unsigned char memo;
  if(value>= userConfig[currentUser].thresholdToCut && !memo)
  { 
       memo=1;
      //motor_2004_board.stepperRotation(MOTOR_A, 50, DEFAULT_MOTOR_STEPS)
  }
  else if(value<= userConfig[currentUser].thresholdToRewind)
  {
    memo=0;
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
        arrowIndexRow =1;
        ArrowIndex(FORCE);
      }

      ArrowIndex(0);
      
      if(knobPsuh == PUSH)
      {
        firstLoop=false;
        knobPsuh = NO_PUSH;
        switch (arrowIndexRow)
        {
          case 1:
          MenuSelectUser();
          break;
          case 2: 
          MenuUserConfig();
          break;
          case 3:
            break;
          default :
          break;
        }
      }
    }while(knobPsuh != LONG_PUSH);
    knobPsuh = NO_PUSH;
    firstLoop=false;
}
void MenuUserConfig()
{
  static int menuNumber;
  ViewMenuUserConfig();
   do
    {
      ArrowIndex(0);
      
      if(flagLowerMenu || flagUpperMenu)
      {
        menuNumber = !menuNumber;
        ViewMenuUserConfig();
        
      }
      Serial.print(arrowIndexRow);
      if(knobPsuh == PUSH)
      {
        knobPsuh = NO_PUSH;
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
        ViewMenuUserConfig();
        knobPsuh = NO_PUSH;
      }
   
    }while(knobPsuh != LONG_PUSH);
    knobPsuh = NO_PUSH;
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

 
 if(memoFeedValue != home.feedValue)
 {
   lcd.setCursor(7,1);
   lcd.print(home.feedValue);
   RemoveZero(home.feedValue,7,1);
 }
 if(memoTrimValue != home.trimValue)
 {
   lcd.setCursor(17,1);
   lcd.print(home.feedValue);
   RemoveZero(home.trimValue,17,1);
 }
 if(memoCntValue != home.counterValue)
 {
   lcd.setCursor(10,3);
   lcd.print(home.counterValue);
   RemoveZero(home.counterValue,10,3);
 }
 memoCntValue = home.counterValue;
 memoFeedValue = home.feedValue;
 memoTrimValue = home.trimValue;

}
/**
 * @brief fixed text display of the home screen
 * 
 */
void HomeScreen()
{
  lcdClear();
  lcd.setCursor(0,1);
  lcd.print("Feed =");
  lcd.print(home.feedValue);
  lcd.setCursor(10,1);
  lcd.print("TRIM =");
  lcd.print(home.trimValue);
  lcd.setCursor(0,3);
  lcd.print("Counter = ");
  lcd.print(home.counterValue);
  lcd.setCursor(12,0);
  lcd.print(userConfig[currentUser].name);
}
/**
 * @brief allows to change the fixed text between screen one and screen two 
 * 
 */
void ViewMenuUserConfig()
{
 // static bool firstLoop=false;
  static bool flagScreenTwo=false;
   
  if(flagUpperMenu ||  flagLowerMenu)
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
 * @brief fixed text display menu user config screen one
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
 * @brief fixed text display menu user config screen two
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
  }while (knobPsuh!=PUSH);

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
   
  } while (knobPsuh != PUSH);
  knobPsuh=NO_PUSH;
  lcdClear(); 
}
/**
 * @brief fixed text display menu Thickness 
 * 
 */
void MenuThickness( )
{
  lcd.setCursor(0,0);
  lcd.print("-----Thickness-----");

  lcd.setCursor(1,1);
  lcd.print("Normal = ");
  lcd.print(userConfig[currentUser].thicknessNormalMode);

  lcd.setCursor(2,1);
  lcd.print("Triming = ");
  lcd.print(userConfig[currentUser].thicknessTrimmingMode);

  lcd.setCursor(3,1);
  lcd.print("Exit");
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
  lcd.print("Threshold to revwind ");
  do
  {
      ArrowIndex(0);
     if(knobPsuh == PUSH)
     {
       knobPsuh=NO_PUSH;
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
      lcd.setCursor(0,arrowIndexRow);
      lcd.write((byte)0);
      lcd.setCursor(0,0);
      lcd.print("-----Threshold------");

      lcd.setCursor(1,1);
      lcd.print("Threshold to cut");

      lcd.setCursor(1,2);
      lcd.print("Threshold to revwind ");
     }
  }while (knobPsuh != LONG_PUSH);
  
  
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
  }while(knobPsuh != PUSH);
  knobPsuh=NO_PUSH;
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
  knobPsuh=NO_PUSH;
  //stores the user's threshold to cut value
  do 
  {
    //Reading the analog value of the position potentiometer
    valAdc = analogRead(ADC_POT);
    //displays the value of the potentiometer and after averaging 
    ShowPot(12,1);
    //saves the current user value
    userConfig[currentUser].thresholdToCut = valAdc;
  }while (knobPsuh!=PUSH);
  knobPsuh = NO_PUSH;
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
  knobPsuh=NO_PUSH;
  //stores the user's threshold to cut value
  do 
  {
    //Reading the analog value of the position potentiometer
    valAdc = analogRead(ADC_POT);
    //displays the value of the potentiometer and after averaging 
    ShowPot(12,1);
    //saves the current user value
    userConfig[currentUser].thresholdToRewind = valAdc;
  }while (knobPsuh!=PUSH);
  knobPsuh=NO_PUSH;
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
  lcd.print("-----Alarm-----");

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
     if(knobPsuh == PUSH)
     {
       knobPsuh=NO_PUSH;
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
      lcd.print("-----Alarm-----");

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
  }while (knobPsuh != LONG_PUSH);
}
void MenuAlarmState()
{
  static bool toggle = true;
  knobPsuh=NO_PUSH;
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
  }while (knobPsuh != PUSH);
  knobPsuh =NO_PUSH;
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
  }while (knobPsuh != PUSH);
  knobPsuh = NO_PUSH;
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
      
   if(value<999)
   {
     lcd.setCursor (column+3,raw);
     lcd.print(" ");
   }
   if(value<99)
   {
     lcd.setCursor (column+2,raw);
     lcd.print(" ");
   }
    if(value<10)
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
          knobPsuh = LONG_PUSH;
      }
      //bounce eliminator
      else if((timer-timerMemo) > BOUNCE_ELIM_TIME)
      {
         knobPsuh = PUSH;
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
  flagUpperMenu=false;
  flagLowerMenu=false;
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
      flagUpperMenu=false;
      flagLowerMenu=true;
    }
    else if (arrowOldPosition==1 && arrowIndexRow==MAX_ROW_INDEX_LCD)
    {
      flagUpperMenu=true;
      flagLowerMenu=false;
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
  valAdc = analogRead(ADC_POT);
  //calculates the average of the adc values
  valAdc = AverageAdc(valAdc);
  //convert to string 
  myString = String (valAdc);
  lcd.setCursor (columns,raw);
  //Serial.print(columns);
  //lcd.print(myString);
  lcd.print(myString);
  RemoveZero(valAdc,columns,raw);
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
  pinMode(MCP23017_INTB,INPUT);
  pinMode(MCP23017_RESET,INPUT);
  pinMode(KNOB_CHANNEL_B,INPUT);
  pinMode(KNOB_CHANNEL_A,INPUT);
  pinMode(KNOB_SWITCH_A,INPUT);
  pinMode(PCA9629A_INT,INPUT);
  pinMode(DEBUG1,OUTPUT);
  pinMode(DEBUG2,OUTPUT);
  pinMode(SDA,INPUT);
  pinMode(SCL,INPUT);
}  
#ifndef SERIAL_DEBUG
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