#include <Arduino.h>


#include "cmu_ws_2004_01_V1_board.h"
#include <Wire.h> 
#include "LiquidCrystal_I2C.h"


// Define the default motor speed and steps for run from BNC trigger
#define DEFAULT_MOTOR_SPEED 100
#define DEFAULT_MOTOR_STEPS 200


// Main board 1921 - IC7 digital output bit definition for driver enable
#define GATE_A_ENABLE_BIT 6

//  Frame slots for commands
#define SYNC_SOF  0
#define MCMD  1
#define SPEED 2
#define STEPSCOUNT 3
#define SYNC_EOF  5
#define BTN_GRBTGL 6
#define ADC_POT A0
#define THRESHOLD_HIGH 900
#define THRESHOLD_LOW 600
#define KNOB_3 2
#define KNOB_6 7


void SensRotation();
void RemoveZero(unsigned int value, unsigned char colonne, unsigned char ligne);
void ThresholdDetection(unsigned int value);
unsigned int AverageAdc (unsigned int valAdc);

// Boards declaration
board_2004_01_V01 motor_2004_board;

// Display declaration
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//variable declaration 
char btnGrbtgl;
unsigned char counter;
unsigned char lastState;
unsigned char knob3;
unsigned char knob6;
unsigned int  valAdc;
unsigned int  moyenne; 
String myString;

// Arduino setup
void setup()
{
  // Main board_2004 initialization
  motor_2004_board.begin();
  
  //pin Out config
  pinMode(BTN_GRBTGL,INPUT);
  pinMode(ADC_POT, INPUT);
  attachInterrupt(digitalPinToInterrupt(KNOB_3),SensRotation, FALLING);
  pinMode(KNOB_6, INPUT);
  
  
  // initialize the lcd 
  lcd.init();                      
  lcd.home();
  lcd.clear();
  // Print a message to the LCD.
  lcd.setCursor(1,0);
  lcd.print("Hello, world!");
  lcd.setCursor(1,1);
  lcd.print("stage 2020");
   lcd.setCursor(1,2);
  lcd.print("Garcia");
  lcd.setCursor(1,3);
  lcd.print("neufo");

  delay(1000);
  
  lcd.clear();
  lcd.home();
  delay(50);
  lcd.print ("counter  = ");
  lcd.setCursor (0,1);
  lcd.print ("val pot = ");
  
}


// Main ARDUINO LOOP

void loop() 
{

 btnGrbtgl = digitalRead (BTN_GRBTGL);

 
 // compare the buttonState to its previous state
 if(!btnGrbtgl && btnGrbtgl != lastState)
 {  
  counter++;
 // motor_2004_board.stepperRotation(MOTOR_A, 10*counter, 2);
 }
 lastState = btnGrbtgl;
 myString =  String (counter);

 lcd.setCursor (11,0);
 lcd.print (counter);
  RemoveZero(counter,11,0);
 
 valAdc = analogRead(ADC_POT);
 valAdc = AverageAdc(valAdc);
 myString = String (valAdc);
 ThresholdDetection(valAdc);
 
 lcd.setCursor (11,1);
 lcd.print(myString);
 RemoveZero(valAdc,11,1);

         
  delay(100);
}
/*------------------------------------------------------------------------------
   AverageAdc()
  ------------------------------------------------------------------------------
   Descriptif: calcul une moyenne sur 5 des valeur du convertisseur analogique.
   Entrée    : valeur ADC
   Sortie    : --
------------------------------------------------------------------------------*/
unsigned int AverageAdc (unsigned int valAdc)
{
  unsigned long averageTempo;
  static unsigned int cntLoop;
  static unsigned int tabValAdc[5];
  unsigned int average;
  if(cntLoop>=5)
  {
    cntLoop=0;
  }
  tabValAdc[cntLoop]=valAdc;
  cntLoop++;
  for(int i=0;i<5;i++)
  {
    averageTempo += tabValAdc[i];
  }
  average = averageTempo/5;
  return average;
}
/*------------------------------------------------------------------------------
   RemoveZero()
  ------------------------------------------------------------------------------
   Descriptif: supprime les zéros d'un nombre sur LCD lors que ces dernier
   sont inutile.
   Entrée    : valeur du nombre. 
               numéro de colonne.
               numéro de ligne.
   Sortie    : --
------------------------------------------------------------------------------*/

/**
 * @brief  supprime les zéros d'un nombre sur LCD lors que ces dernier
 * 
 * @param valeur du nombre
 * @param colonne 
 * @param ligne 
 */

void RemoveZero(unsigned int value, unsigned char colonne, unsigned char ligne)
{
   if(value<999)
   {
     lcd.setCursor (colonne+3,ligne);
     lcd.print(" ");
   }
   if(value<99)
   {
     lcd.setCursor (colonne+2,ligne);
     lcd.print(" ");
   }
    if(value<10)
   {
     lcd.setCursor (colonne+1,ligne);
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
void  ThresholdDetection(unsigned int value)
{
  static unsigned char memo;
  if(value>=THRESHOLD_HIGH )
  { 
    if(!memo)
    {
       memo=1;
      motor_2004_board.stepperRotation(MOTOR_A, 50, DEFAULT_MOTOR_STEPS);
    }
  }
  else if(value<=THRESHOLD_LOW)
  {
    memo=0;
  }

}
/*------------------------------------------------------------------------------
   sensRotation ()
  ------------------------------------------------------------------------------
   Descriptif: Interruption externe, détermine le sens de rotation de l'encodeur
   Entrée    : --
   Sortie    : --
------------------------------------------------------------------------------*/
void SensRotation()
{
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
/*------------------------------------------------------------------------------
   AverageAdc()
  ------------------------------------------------------------------------------
   Descriptif: calcul une moyenne sur 5 des valeur du convertisseur analogique.
   Entrée    : valeur ADC
   Sortie    : --
------------------------------------------------------------------------------*/
void FixedTextLcd ()
{
  
  
}