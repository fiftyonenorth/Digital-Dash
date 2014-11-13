//////////////////////
// Digital Dash     // 
// David T          //
//////////////////////

// include the library code:
#include <LiquidCrystal.h>

const int ledCount = 14;    // the number of digital pin LEDs in the bar graph
int ledPins[] = { 
  41, 43, 45, 47, 49, 51, 53, 40, 42, 44, 46, 48, 50, 52 };   // an array of digital pin numbers to which LEDs are attached
int airsensorpin = 3;  
int oldscreen = 0;
int screen = 0;
long sampleperiod = 250;
long sampleold = 0;
long logperiod = 1000;
long logold = 0;
long displayperiod = 500;
long displayold = 0;
unsigned long currentmillis;
int watertemp = 0;
int watertempmin = 0;
int watertempmax = 0;
int watertempmaxalarm = 95;
int oiltemp = 0;
int oiltempmin = 0;
int oiltempmax = 0;
int oiltempmaxalarm = 100;
float airtemp = 0;
float airtempmin = 0;
float airtempmax = 0;
float battvolt = 0;
float battvoltmin = 0;
float battvoltmax = 0;
float battvoltminalarm = 130;
float battvoltmaxalarm = 145; 
int g = 0;
int gmin = 0;
int gmax = 0;
int rpm = 0;
volatile int rpmcount = 0;
int rpmmax = 0;
int rpmredline = 6000;
int alarmreason = 0;
boolean leftkeydown = false;
boolean rightkeydown = false;
unsigned long strobeledsold = 0;
int strobeledsperiod = 100;
boolean strobeleds = false;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(36, 34, 32, 30, 28, 26);

void setup()
{
  Serial.begin(9600);
  attachInterrupt(0, tachpulse, RISING);  
  
// loop over the pin array and set them all to output:
  for (int thisLed = 0; thisLed < ledCount; thisLed++)
  {
    pinMode(ledPins[thisLed], OUTPUT);
  }
  lcd.begin(16, 2);
  lcdstartup();        // display welcome message
  ledstartup();        // sweep leds
}

void loop()
{
  
  // refresh parameters
  currentmillis = millis();
  if (currentmillis - sampleold > sampleperiod)
  {
    sampleold = millis();
    //  readwatertemp();
    //  readoiltemp();
    airtemp = readairtemp();
    battvolt = readbatteryvolt();
    //  readaccelerometer();
    readrpm();
  }
  
  // log parameters
  currentmillis = millis();
  if (currentmillis - logold > logperiod)
  {
    logtosdcard();
    logold = millis();
  }  

  // refresh screen
  currentmillis = millis();
  if (currentmillis - displayold > displayperiod)
    {
    displayold = millis();
    if (screen == 0)
    {
      Serial.println("OK2"); 
      displayscreenhome();
    }
    else if (screen == 1)
    {
      displayscreenwater();
    }
    else if (screen == 2)
    {
      displayscreenoil();
    }
    else if (screen == 3)
    {
      displayscreenair();
    }
    else if (screen == 4)
    {
      displayscreenbatt();
    }
    else if (screen == 5)
    {
      displayscreenaccel();
    }
    else if (screen == 6)
    {
      displayscreenrpm();
    }
    else if (screen == 7)
    {
      displayscreenalarm();
    }
  }
  
  //refresh leds
//  if (alarmreason != 0) {  // light all leds for alarm
//    displayleds (65535);
//  }  
//  else {
    
//    if (rpm > rpmredline) {  // strobe leds for redline rpm

//      if ((strobeleds == false) && (millis() > (strobeledsold + strobeledsperiod))) {
//        displayleds (0);
//        strobeleds == true;
//      }
//      else if ((strobeleds == true) && (millis() > (strobeledsold + strobeledsperiod))) {
//        displayleds (65535);
//        strobeleds == false;
//      }

//    else {
//      displayleds (rpm);
//    }
//    }
    
//  }    
}  // end of main loop

void lcdstartup()
{
  lcd.setCursor(0, 0);
  lcd.print("*  TIGER AVON  *");
  lcd.setCursor(0, 1);
  lcd.print("*  2.0 ZETEC   *");
  return;
}

void ledstartup()
{
  for (int thisLed = 0; thisLed < ledCount; thisLed++)
  {
    digitalWrite(ledPins[thisLed], HIGH);
    delay(30); 
  }
  for (int thisLed = 0; thisLed < ledCount; thisLed++)
  {
   digitalWrite(ledPins[thisLed], LOW);
   delay(30); 
  }
  return;
}

float readbatteryvolt()
{
  int reading = analogRead(A0);         // read the input on analoge pin 0
  float battvalue = reading * 4.8 ;     // convert to a voltage (0-5V) (my voltage regulator outputs 4.8V)
  battvalue = battvalue * 3.102;        // multiply up by ratio of voltage divider resistors (670 / 216 = 3.102)
  battvalue = battvalue / 1024;         // 1024 represents power supply, i.e. 4.8V
  battvalue = battvalue + 0.5;          // add back in 0.5V voltage drop due to diode on 12V input
  if (battvalue < battvoltmin)
  {
    battvoltmin = battvalue;
  }  
  if (battvalue > battvoltmax)
  {
    battvoltmax = battvalue;
  }  
  if ((battvalue > battvoltmaxalarm) || (battvalue < battvoltminalarm))
  {
    oldscreen = screen;
//    screen = 7;
    alarmreason = 4;
  }
  else
  {
    alarmreason = 0;
  }    
  return battvalue;
}

float readairtemp()
{
  int reading = analogRead(airsensorpin);  
  float voltage = reading * 5;
  voltage = voltage / 1024; 
  float airtempvalue = (voltage - 0.5) * 100  ;
  return airtempvalue;
}  

void readrpm()
{
  rpm = (((rpmcount*4)*60)/2);
  rpmcount = 0;  
  return;
}

void logtosdcard()
{
  return;
}

void displayscreenhome()
{
  char line0[17] = {"WT:      OT:    "};
  lcd.setCursor(0, 0);
  lcd.print(line0);
  char line1[17] = {"AT:      BV:    "};
  lcd.setCursor(0, 1);
  lcd.print(line1);
  if ((airtemp > -40) && (airtemp < 125))
  {
    lcd.setCursor(3, 1);
    lcd.print(airtemp,1);
  }
  if ((battvolt > 0) && (battvolt <20))
  {
    lcd.setCursor(12, 1);
    lcd.print(battvolt,1);
  }
  return;
}

void displayscreenwater()
{
  return;
}

void displayscreenoil()
{
  return;
}

void displayscreenair()
{
  return;
}

void displayscreenbatt()
{
  return;
}

void displayscreenaccel()
{
  return;
}

void displayscreenrpm()
{
  return;
}

void displayscreenalarm()
{
  return;
}

void displayleds(int revs)
{
  return;
}

void tachpulse()
{
  rpmcount++;
  return;
}

