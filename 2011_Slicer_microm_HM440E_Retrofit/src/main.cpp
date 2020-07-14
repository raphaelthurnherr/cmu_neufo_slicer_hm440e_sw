#include <Arduino.h>


#include "cmu_ws_2004_01_V1_board.h"
#include <Wire.h> 
#include "LiquidCrystal_I2C.h"


// Define the default motor speed and steps for run from BNC trigger
#define DEFAULT_MOTOR_SPEED 100
#define DEFAULT_MOTOR_STEPS 200




//  Frame slots for commands
#define BTN_GRBTGL 6
#define ADC_POT A0

#define KNOB_3 2
#define KNOB_6 7
#define MCP23017_INTA 1
//users
struct USERS_SETTINGS {
    String name;
    unsigned char mode;
    unsigned int thicknessNormalMode;
    unsigned int thicknessTrimmingMode;
    unsigned int thresholdSuperieur;
    unsigned int thresholdInferieur; 
};
struct HOME
{
  unsigned char counterValue;
  unsigned int trimValue;
  unsigned int feedValue;
};
struct MENU
{
  USERS_SETTINGS userStetting;
  HOME home ;
};

/*=======prototype function=====================*/
void SensRotation();
void RemoveZero(unsigned int value, unsigned char colonne, unsigned char ligne);
void ThresholdDetection(unsigned int value);
unsigned int AverageAdc (unsigned int valAdc);
void SaveThreshold();
void HomeScreen();
void ShowPot(unsigned char columns, unsigned char raw);

// Boards declaration
board_2004_01_V01 motor_2004_board;

// Display declaration
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display 


//variable declaration 
char btnGrbtgl;
String myString;
unsigned char counter;
unsigned char lastState;
unsigned char knob3;
unsigned char knob6;
unsigned int  valAdc;
unsigned int  moyenne; 
unsigned int thresholdHigh =900;
unsigned int thresholdLow =600;



USERS_SETTINGS userDefault ={"Default",0,60,100,150,800};
HOME home = {0,0,0};
MENU menu={userDefault,home};
USERS_SETTINGS currentUser = userDefault;
// Arduino setup
void setup()
{
  // Main board_2004 initialization
  motor_2004_board.begin();
   Serial.begin(9800);
  //pin Out config
  pinMode(BTN_GRBTGL,INPUT);

  // pinMode(ADC_POT, INPUT);
  attachInterrupt(digitalPinToInterrupt(KNOB_3),SensRotation, FALLING);
  attachInterrupt(digitalPinToInterrupt(KNOB_3),SensRotation, FALLING);
  
  pinMode(KNOB_6, INPUT);
  pinMode(8,OUTPUT);
  
  
  // initialize the lcd 
  lcd.init();   
  lcd.noDisplay();                   
  lcd.display();
  lcd.clear();
  // Print a message to the LCD.
  //lcd.setCursor(1,0);
  HomeScreen();
  
}


// Main ARDUINO LOOP

void loop() 
{

 btnGrbtgl = digitalRead (BTN_GRBTGL);
 digitalWrite(8,HIGH);
 delay(500);
 digitalWrite(8,LOW);
 delay(500);
 // compare the buttonState to its previous state
 if(!btnGrbtgl && btnGrbtgl != lastState)
 {  
  //counter++;
  // motor_2004_board.stepperRotation(MOTOR_A, 10*counter, 2);
   SaveThreshold();
 }
 lastState = btnGrbtgl;
 myString =  String (counter);
  valAdc = analogRead(ADC_POT);
 ThresholdDetection(valAdc);
 
 

         
  delay(10);
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
  static unsigned int tabValAdc[5];
  unsigned int average;
  unsigned char i;
  averageTempo=0;
  tabValAdc[cntLoop]=valAdc;
  cntLoop++;
  if(cntLoop>=5)
  {    
    cntLoop=0;
  }

  for( i=0;i<5;i++)
  {

    averageTempo += tabValAdc[i];

  }
  average = averageTempo/5;
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
/*------------------------------------------------------------------------------
   ThresholdDetection()
  ------------------------------------------------------------------------------
   Descriptif: Monte ou descend le scpécimen en fonction de la position de la 
   lame.
   Entrée    : valeur du potentionmètre 
   Sortie    : --
------------------------------------------------------------------------------*/
/**
 * @brief Raises or lowers the platform depending on the position of the blade.
 * @param value 
 */
void  ThresholdDetection(unsigned int value)
{
  static unsigned char memo;
  if(value>=thresholdHigh )
  { 
    if(!memo)
    {
       memo=1;
      motor_2004_board.stepperRotation(MOTOR_A, 50, DEFAULT_MOTOR_STEPS);
    }
  }
  else if(value<=thresholdLow)
  {
    memo=0;
  }

}
/**
 * @brief determines the direction of rotation of the encoder
 * 
 */
void SensRotation()
{
  valAdc = analogRead(ADC_POT);
  knob3 = digitalRead (KNOB_3);
  knob6 = digitalRead (KNOB_6);
  
  if(knob3 == knob6)
  {
    counter++;
  }
  else
  {
    counter--;
  }
}
/**
 * @brief Records when the media goes up and down 
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
  btnGrbtgl = digitalRead (BTN_GRBTGL);
  do 
  {
    btnGrbtgl = digitalRead (BTN_GRBTGL);
  } while (!btnGrbtgl);

  do 
  {
    btnGrbtgl = digitalRead (BTN_GRBTGL);
    valAdc = analogRead(ADC_POT);
    thresholdHigh = valAdc;
    ShowPot(16,1);
  }while (btnGrbtgl);
 
  do 
  {
    btnGrbtgl = digitalRead (BTN_GRBTGL);
  } while (!btnGrbtgl);
  do 
  {
    btnGrbtgl = digitalRead (BTN_GRBTGL);
    valAdc = analogRead(ADC_POT);
    thresholdLow = valAdc;
    ShowPot(16,2);
  }while (btnGrbtgl);
  lcd.clear();
  lcd.home();
  HomeScreen();
}
/**
 * @brief home screen
 * 
 * 
 */
void HomeScreen()
{
 
  lcd.setCursor(1,1);
  lcd.print("Feed=");
  lcd.print(home.counterValue);
  lcd.setCursor(1,3);
  lcd.print("Counter=");
  lcd.print(home.counterValue);
  lcd.setCursor(13,1);
  lcd.print("TRIM=");
  lcd.print(home.trimValue);
  lcd.setCursor(0,0);
  lcd.print(userDefault.name);
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
  lcd.print(myString);
  RemoveZero(valAdc,columns,raw);
}  
