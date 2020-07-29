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
#include "cmu_ws_2004_01_V1_board.h"
#include <Wire.h> 
#include "LiquidCrystal_I2C.h"
#include <jsonConfigSDcard.h>
#include <SdFat.h>



// Define the default motor speed and steps for run from BNC trigger
#define DEFAULT_MOTOR_SPEED 100
#define DEFAULT_MOTOR_STEPS 200

//config 
#define NORMAL_MODE 0
#define TRIMING_MODE 1
#define ALARM_OFF 0
#define ALARM_ON 1
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
#define PUSH 1
#define NO_PUSH 0

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
#define EXIT -1

// Boards declaration
board_2004_01_V01 motor_2004_board; 
    
// Display declaration
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01 , 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE); // set the LCD address to 0x27 for a 16 chars and 2 line display

// Create new variable for user config storage
SETTINGS userConfig[MAX_USER_SETTINGS];
SLICERCONFIG machineConfig;

struct HOME
{
  unsigned char counterValue;
  unsigned int trimValue;
  unsigned int feedValue;
};

struct MENU
{
  HOME home;
};

/*=======prototype function=====================*/

void MenuAlarm();
void MenuThresholdToCut();
void ScreenThreshold();
void MenuThreshold();
void MenuThresholdToCut();
void ScreenThreshold();
void MenuThreshold();
void MenuThickness( );
void MenuMode();
void ArrowIndex();
void MenuSelection();
void MenuSelecScreenOne();
void MenuSelecScreenTwo();
void knobRotationDetection();
void knobSwitchDetection();
void RemoveZero(unsigned int value, unsigned char colonne, unsigned char ligne);
void ThresholdDetection(unsigned int value);
unsigned int AverageAdc (unsigned int valAdc);
void SaveThreshold();
void HomeScreen();
void ShowPot(unsigned char columns, unsigned char raw);
void PortInit();






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
int arrowIndexRow=0;
int arrowOldPosition=3;
bool flagUpperMenu;
bool flagLowerMenu;
byte retarrow[8] = {	0x10,0x10,0x14,0x16,0x1f,0x06,0x04};

HOME home = {0,0,0};
MENU menu;
 

// Arduino setup

void setup() {
  // Open serial communications and wait for port to open:
 /*
 


  }else Serial.write("ERROR during setting loading !");
  Serial.end();*/
  //pin Out config
  //pinMode(BTN_GRBTGL,INPUT);
  //pinMode(ADC_POT, INPUT);
  // pinMode(ADC_POT, INPUT);
  PortInit();
  attachInterrupt(digitalPinToInterrupt(KNOB_CHANNEL_A),knobRotationDetection, FALLING);
  attachInterrupt(digitalPinToInterrupt(KNOB_SWITCH_A),knobSwitchDetection, FALLING);
  //motor_2004_board.begin();
   
  lcd.begin(20,4);
  lcd.noDisplay();          
  lcd.display();
  lcd.clear();

   HomeScreen();
  //currentUser = userConfig[2];
  //motor_2004_board.begin();
   lcd.createChar(0, retarrow);
   
   
   delay(1000);
}

void loop() {
  swEncodeur = digitalRead (KNOB_SWITCH_A);
  ArrowIndex();
 // lastState = swEncodeur;
 // compare the buttonState to its previous state


 //valAdc = analogRead(ADC_POT);
 //ThresholdDetection(valAdc);
 MenuSelection();

   
 //MenuMode(&currentUser); 
 //motor_2004_board.stepperRotation(MOTOR_A, 80, 10);
  delay(100);
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

void RemoveZero(unsigned int value, unsigned char column, unsigned char raw)
{
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
 * @brief determines the direction of knobRotation of the encoder
 * 
 */
void knobRotationDetection()
{
  bool knobChannelA;
  bool knobChannelB;
  
  knobChannelA = digitalRead (KNOB_CHANNEL_A);
  knobChannelB = digitalRead (KNOB_CHANNEL_B);
 
  if(knobChannelA == knobChannelB)
  {
    
    
     knobRotation = CW;
  }
  else
  {
   
    knobRotation = CCW;
  }
}

void knobSwitchDetection()
{
  
  knobPsuh = PUSH;
}



/**
 * @brief stores the user's threshold value
 * 
 */
void SaveThreshold()
{
  
  lcd.clear();
  lcd.setCursor (0,0);
  lcd.print("threshol config");
  lcd.setCursor (0,1);
  lcd.print("threshol high = ");
  lcd.setCursor (0,2);
  lcd.print("threshol low = ");
  knobPsuh=NO_PUSH;
  //stores the user's threshold to cut value
  do 
  {
    swEncodeur = digitalRead (KNOB_SWITCH_A);
    valAdc = analogRead(ADC_POT);
    userConfig[currentUser].thresholdToCut = valAdc;
    ShowPot(16,1);
  }while (knobPsuh==NO_PUSH);   

  knobPsuh=NO_PUSH;
  //stores the user's threshold to rewind cut value
  do 
  {
    swEncodeur = digitalRead (KNOB_SWITCH_A);
    valAdc = analogRead(ADC_POT);
    userConfig[currentUser].thresholdToRewind = valAdc;
    ShowPot(16,2);
  }while (knobPsuh==NO_PUSH);

  //back to home screen
  HomeScreen();
}
/**
 * @brief home screen
 * 
 * 
 */
void HomeScreen( )
{
  lcd.home();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Feed=");
  lcd.print(home.counterValue);
  lcd.setCursor(5,3);
  lcd.print("Counter=");
  lcd.print(home.counterValue);
  lcd.setCursor(13,1);
  lcd.print("TRIM=");
  lcd.print(home.trimValue);
  lcd.setCursor(5,0);
  lcd.print(userConfig[currentUser].name);
}
/**
 * @brief moves the cursor on display 
 * 
 * @return int 
 */
void ArrowIndex()
{
  flagUpperMenu=false;
    flagLowerMenu=false;
  if(knobRotation!=NO_ROTATION)
  {
    
    Serial.begin(9600);
    if(knobRotation==CW)
    {
      
      
      arrowIndexRow ++;
      
    }
    else if(knobRotation==CCW)
    {
      
     
      arrowIndexRow --;
    }
    if(arrowIndexRow<0)
    {
      arrowIndexRow=3;
    }
    else if (arrowIndexRow>3)
    {
      arrowIndexRow=0;
    }
    if(arrowOldPosition==3 && arrowIndexRow==0 )
    {
      flagUpperMenu=false;
      flagLowerMenu=true;
    }
    else if (arrowOldPosition==0 && arrowIndexRow==3)
    {
      flagUpperMenu=true;
      flagLowerMenu=false;
    }
    Serial.print(flagLowerMenu);
    Serial.print(flagUpperMenu);
    lcd.setCursor(0,arrowIndexRow);
    lcd.write((byte)0);
    lcd.setCursor(0,arrowOldPosition);
    lcd.print(" ");
    knobRotation=NO_ROTATION;
    arrowOldPosition=arrowIndexRow;
  }
 
}
void MenuSelection()
{
  static bool firstLoop=false;
  static bool flagScreenTwo=false;
   
   if(flagUpperMenu ||  flagLowerMenu)
  {
    firstLoop=false;
    flagScreenTwo= !flagScreenTwo;
  }
  if(!firstLoop)
  {   
    firstLoop=true;
    if(!flagScreenTwo)
    {
      MenuSelecScreenOne();
    }
    else 
    {
      MenuSelecScreenTwo();
    }
            
  }
 //MenuSelecScreenTwo();

   if(!swEncodeur && swEncodeur != lastState )
 {  
 }
 lastState = swEncodeur;
}
void MenuSelecScreenOne()
{
    lcd.clear();
    lcd.home();
    lcd.setCursor(1,0);
    lcd.print("----user setting---");
    lcd.setCursor(1,1);
    lcd.print("Mode = ");
    if(userConfig[currentUser].mode==MODE_NORMAL)
    {
      myString = "Normal";
    }
    else
    {
      myString = "Trimming";
    }
    lcd.print(myString);
    lcd.setCursor(1,2);
    lcd.print("Thickness");
    lcd.setCursor(1,3);
    lcd.print("Thresholds");
}
void MenuSelecScreenTwo()
{
    lcd.clear();
    lcd.home();
    lcd.setCursor(1,0);
    lcd.print("----user setting---");
    lcd.setCursor(1,1);
    lcd.print("Thickness");
    lcd.setCursor(1,2);
    lcd.print("Thresholds");
    lcd.setCursor(1,3);
    lcd.print("Alarm = ");
    if(userConfig[currentUser].alarm)
    {
      lcd.print("ON");
    }
    else 
    {
      lcd.print("OFF");
    }
  //  lcd.setCursor(9,3);
    //lcd.print(myString);
  
}
/**
 * @brief fixed text display menu mode 
 * 
 * 
 */
void MenuMode()
{
  
  lcd.setCursor(0,0);
  lcd.print("--------Mode--------");
  lcd.setCursor(1,1);
  lcd.print("Mode : ");
  if(0 == NORMAL_MODE)
  {
    lcd.print("Normal");
  }
  else
  {
    lcd.print("Triming");
  }
  lcd.setCursor(3,1);
  lcd.print("Exit");
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

void MenuThreshold()
{
  lcd.setCursor(0,0);
  lcd.print("-----Threshold-----");

  lcd.setCursor(1,1);
  lcd.print("Threshold to cut");

  lcd.setCursor(2,1);
  lcd.print("Threshold to revwind ");

  lcd.setCursor(3,1);
  lcd.print("Exit");
}
void ScreenThreshold()
{
  lcd.setCursor(0,1);
  lcd.print("place the blade and");
  lcd.setCursor(1,1);
  lcd.print("Press validation");
  lcd.setCursor(2,1);
  lcd.print("button");
}
void MenuThresholdToCut()
{
  lcd.setCursor(0,0);
  lcd.print("--Threshold to cut--");
  lcd.setCursor(1,1);
  lcd.print("Position = XXXX");
  lcd.setCursor(3,1);
  lcd.print("Exit");
  knobPsuh=NO_PUSH;
  //stores the user's threshold to cut value
  do 
  {
    swEncodeur = digitalRead (KNOB_SWITCH_A);
    valAdc = analogRead(ADC_POT);
    userConfig[currentUser].thresholdToCut = valAdc;
    ShowPot(16,1);
  }while (knobPsuh==NO_PUSH);

  //back to home screen
  HomeScreen();   
}
void MenuThresholdToRewind()
{
  lcd.setCursor(0,0);
  lcd.print("-Threshold to rewind-");
  lcd.setCursor(1,1);
  lcd.print("Position = XXXX");
  lcd.setCursor(3,1);
  lcd.print("Exit");
  knobPsuh=NO_PUSH;
  do 
  {
    swEncodeur = digitalRead (KNOB_SWITCH_A);
    valAdc = analogRead(ADC_POT);
    userConfig[currentUser].thresholdToRewind = valAdc;
    ShowPot(16,2);
  }while (knobPsuh==NO_PUSH);

  //back to home screen
  HomeScreen();

}
/**
 * @brief Alarm setup menu
 * 
 */
void MenuAlarm()
{
  static bool firstLoop = false;
  static bool toggle = true;
  //displays fixed text 
  if(!firstLoop)
  {
    firstLoop=true;
    lcd.clear();
    lcd.home();
    lcd.setCursor(0,0);
    lcd.print("-------Alarm--------");
    lcd.setCursor(1,2);
    lcd.print("Alarm =");
    lcd.setCursor(9,2);
    //lcd.print(userConfig[currentUser].alarm);
    
    if(userConfig[currentUser].alarm)
    {
      lcd.print("ON");
    }
    else 
    {
      lcd.print("OFF");
    }
     
  }
  //activates or deactivates the temperature alarm 
  if(knobRotation != NO_ROTATION)
  {
    knobRotation = NO_ROTATION;
    toggle = !toggle;
    //changes without taking into account the direction 
    //of rotation of the encoder. 
    if(toggle)
    {
      myString = "ON ";
      userConfig[currentUser].alarm = ALARM_ON;
    }
    else 
    {     
      myString = "OFF";
       userConfig[currentUser].alarm = Alarm_OFF;
    }
    
  }
  lcd.setCursor(9,2);
  lcd.print(myString); 
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