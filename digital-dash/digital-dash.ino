////////////////////////////////////////////////////////////////////////////////
// Project: Digital Dash                                                      //
// Author:  David T                                                           //
// Date:    19 Nov 2014                                                       //
// Details: Car digital dash based upon Arduino Mega 2560 R3                  //
//                                                                            //
// Pin   Assignment                                                           //
// 02    Tacho pulse input - Interrupt 0                                      //
// 03    Right push btton                                                     //
// 04    Left push button                                                     //
// 26    LCD RS                                                               //
// 28    LCD Enable                                                           //
// 30    LCD D4                                                               //
// 32    LCD D5                                                               //
// 34    LCD D6                                                               //
// 36    LCD D7                                                               //
// 40    Tacho LED8                                                           //
// 41    Tacho LED1                                                           //
// 42    Tacho LED9                                                           //
// 43    Tacho LED2                                                           //
// 44    Tacho LED10                                                          //
// 45    Tacho LED3                                                           //
// 46    Tacho LED11                                                          //
// 47    Tacho LED4                                                           //
// 48    Tacho LED12                                                          //
// 49    Tacho LED5                                                           //
// 50    Tacho LED13                                                          //
// 51    Tacho LED6                                                           //
// 52    Tacho LED14                                                          //
// 53    Tacho LED7                                                           //
// A0    Battery voltage                                                      //
// A1    Water temperature                                                    //
// A2    Oil temperature                                                      //
// A3    Air temperature                                                      //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Libraries to include                                                       //
////////////////////////////////////////////////////////////////////////////////
#include <LiquidCrystal.h>
// initialize the lcd library with the numbers of the interface pins
LiquidCrystal lcd(36, 34, 32, 30, 28, 26);

////////////////////////////////////////////////////////////////////////////////
// LED control parameters                                                     //
////////////////////////////////////////////////////////////////////////////////
const int ledCount = 14;      // the number of digital pin LEDs in the bar graph
int ledPins[] = {             // array of digital pin numbers with LEDs attached
  41, 43, 45, 47, 49, 51, 53, 40, 42, 44, 46, 48, 50, 52 };   
long refreshledsperiod = 333; //read RPM and update LED 3 times/sec
long refreshledsold = 0;
long strobeledsold = 0;
int strobeledsperiod = 100;
boolean strobeleds = false;

////////////////////////////////////////////////////////////////////////////////
// Display control parameters                                                 //
////////////////////////////////////////////////////////////////////////////////
int oldscreen = 0;           // screen to return to after clearing alarm
int screen = 0;              // start on home screen
long displayperiod = 500;    // refresh screen 2 times/sec
long displayold = 0;         // to store last time screen display event occured

////////////////////////////////////////////////////////////////////////////////
// Data sampling control parameters                                                 //
////////////////////////////////////////////////////////////////////////////////
long sampleperiod = 500;     // read parameters 2 times/sec
long sampleold = 0;          // to store last time sample event occured

////////////////////////////////////////////////////////////////////////////////
// Data logging control parameters                                            //
////////////////////////////////////////////////////////////////////////////////
long logperiod = 1000;       // write data to SD card 1 times/second
long logold = 0;             // to store last time data log event occured

////////////////////////////////////////////////////////////////////////////////
// Miscellaneous loop control parameters                                      //
////////////////////////////////////////////////////////////////////////////////
unsigned long currentmillis; // used to measure previous event vs current time
boolean firstloop = true;    // some min. values need to be set higher than zero

////////////////////////////////////////////////////////////////////////////////
// Water temperature parameters                                               //
////////////////////////////////////////////////////////////////////////////////
int watertemp = 0;           // current water temperature reading
int watertempmin = 0;        // minimum water temperature reading
int watertempmax = 0;        // maximum water temperature reading
int watertempmaxalarm = 95;  // water temperature alarm trigger value

////////////////////////////////////////////////////////////////////////////////
// Oil temperature parameters                                                 //
////////////////////////////////////////////////////////////////////////////////
int oiltemp = 0;             // current oil temperature reading
int oiltempmin = 0;          // minimum oil temperature reading
int oiltempmax = 0;          // maximum oil temperature reading
int oiltempmaxalarm = 100;   // oil temperature alarm trigger value

////////////////////////////////////////////////////////////////////////////////
// Air temperature parameters                                                 //
////////////////////////////////////////////////////////////////////////////////
float airtemp = 0;           // current air temperature reading
float airtempmin = 0;        // minimum air temperature reading
float airtempmax = 0;        // maximum air temperature reading
float airtempminalarm = 3;   // ice warning alarm trigger value

////////////////////////////////////////////////////////////////////////////////
// Battery voltage parameters                                                 //
////////////////////////////////////////////////////////////////////////////////
float battvolt = 0;               // current battery voltage reading
float battvoltmin = 0;            // minimum battery voltage reading
float battvoltmax = 0;            // maximum battery voltage reading
float battvoltminalarm = 130;     // minimum battery voltage alarm trigger value
float battvoltmaxalarm = 145;     // maximum battery voltage alarm trigger value

////////////////////////////////////////////////////////////////////////////////
// Accelerometer parameters                                                   //
////////////////////////////////////////////////////////////////////////////////
float glon = 0;
float glonmax = 0;
float glat = 0;
float glatmax = 0;

////////////////////////////////////////////////////////////////////////////////
// RPM parameters                                                             //
////////////////////////////////////////////////////////////////////////////////
int rpm = 0;
int rpmvalue = 0;
int multiplier = 500;
volatile int rpmcount = 0;
int rpmmax = 0;
int rpmredline = 6000;        //  RPM trigger value to strobe LEDs

////////////////////////////////////////////////////////////////////////////////
// Alarm parameters                                                           //
////////////////////////////////////////////////////////////////////////////////
int alarmreason = 0;

////////////////////////////////////////////////////////////////////////////////
// Key press control parameters                                               //
////////////////////////////////////////////////////////////////////////////////
boolean leftkeydown = false;
boolean rightkeydown = false;
int readleftkey = LOW;
int readrightkey = LOW;
long readleftkeyold = 0;
long readrightkeyold = 0;
long readkeyperiod = 200;

////////////////////////////////////////////////////////////////////////////////
// Set Up                                                                     //
////////////////////////////////////////////////////////////////////////////////
void setup()
{
//  Serial.begin(9600);
  attachInterrupt(0, tachpulse, FALLING);

// set digital pins as inputs to read key presses
  pinMode(3,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
  
// loop over the pin array and set them all to output:
  for (int thisLed = 0; thisLed < ledCount; thisLed++)
  {
    pinMode(ledPins[thisLed], OUTPUT);
  }
  lcd.begin(16, 2);
  lcdstartup();        // display welcome message
  ledstartup();        // sweep leds
  clearscreen();       // blank screen
}

////////////////////////////////////////////////////////////////////////////////
// Main Loop                                                                  //
////////////////////////////////////////////////////////////////////////////////
void loop()
{
  // check for key presses
  currentmillis = millis();
  if (currentmillis - readleftkeyold > readkeyperiod)
  {
    readleftkey = digitalRead(4);
    if (readleftkey == LOW)
    {
      readleftkeyold = millis();
      screen++;
      if (screen > 6)
      {
        screen = 0;
      }
      clearscreen();
    }
  }  
  if (currentmillis - readrightkeyold > readkeyperiod)
  {
    readrightkey = digitalRead(3);
    if (readrightkey == LOW)
    {
      reset();
      readrightkeyold = millis();
    }
  }
    
  // refresh parameters, except rpm
  currentmillis = millis();
  if (currentmillis - sampleold > sampleperiod)
  {
    sampleold = millis();
    //  readwatertemp();
    //  readoiltemp();
    airtemp = readairtemp();
    battvolt = readbatteryvolt();
    //  readaccelerometer();
  }
    
  //refresh rpm
  currentmillis = millis();
  if (currentmillis - refreshledsold > refreshledsperiod)
  {
    refreshledsold = millis();
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
      rpm = readrpm();
      displayleds (rpm);
//    }
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

    firstloop = false;
//  }    
}
////////////////////////////////////////////////////////////////////////////////
// End of main Loop                                                           //
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Functions                                                                  //
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Display LCD startup message                                                //
////////////////////////////////////////////////////////////////////////////////
void lcdstartup()
{
  lcd.setCursor(0, 0);
  lcd.print("*  TIGER AVON  *");
  lcd.setCursor(0, 1);
  lcd.print("*  2.0 ZETEC   *");
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Sweep LEDs at startup                                                      //
////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
// Clear LCD screen                                                           //
////////////////////////////////////////////////////////////////////////////////
void clearscreen()
{
  char line0[17] = {"                "};
  lcd.setCursor(0, 0);
  lcd.print(line0);
  char line1[17] = {"                "};
  lcd.setCursor(0, 1);
  lcd.print(line1);
  return;
}  

////////////////////////////////////////////////////////////////////////////////
// Reset parameters                                                           //
////////////////////////////////////////////////////////////////////////////////
void reset()
{
  if (screen == 1)
  {
    //
  }
  else if (screen == 2)
  {
    //
  }
  else if (screen == 3)
  {
    airtempmin = airtemp;
    airtempmax = airtemp;
  }
  else if (screen == 4)
  {
    battvoltmin = battvolt;
    battvoltmax = battvolt;
  }
  else if (screen == 5)
  {
    //
  }
  else if (screen == 6)
  {
    rpmmax = 0;
  }
return;
}

////////////////////////////////////////////////////////////////////////////////
// Read battery voltage                                                       //
////////////////////////////////////////////////////////////////////////////////
float readbatteryvolt()
{
  int reading = analogRead(A0);         // read the input on analoge pin 0
  float battvalue = reading * 4.8 ;     // convert to a voltage (0-5V) (my voltage regulator outputs 4.8V)
  battvalue = battvalue * 3.102;        // multiply up by ratio of voltage divider resistors (670 / 216 = 3.102)
  battvalue = battvalue / 1024;         // 1024 represents power supply, i.e. 4.8V
  battvalue = battvalue + 0.5;          // add back in 0.5V voltage drop due to diode on 12V input
  if ((battvalue < battvoltmin) || (firstloop))
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

////////////////////////////////////////////////////////////////////////////////
// Read air temperature                                                       //
////////////////////////////////////////////////////////////////////////////////
float readairtemp()
{
  int reading = analogRead(A3);  
  float voltage = reading * 5.0;
  voltage /= 1024;
  float airtempvalue = (voltage - 0.5) * 100 ;
  if ((airtempvalue < airtempmin) || (firstloop))
  {
    airtempmin = airtempvalue;
  }  
  if (airtempvalue > airtempmax)
  {
    airtempmax = airtempvalue;
  }  
  return airtempvalue;
}  

////////////////////////////////////////////////////////////////////////////////
// Read RPM                                                                   //
////////////////////////////////////////////////////////////////////////////////
int readrpm()
{
  rpmvalue = (rpmcount*100);  //     /2 tacho pulses per engine revolution
                              //     *60 secs
                              //     *3.333 samples per sec
                              //     gives a round multiples of 100 for display
  rpmcount = 0;
  if (rpm > rpmmax)
  {
    rpmmax = rpm;
  }    
  return rpmvalue;
}

////////////////////////////////////////////////////////////////////////////////
// Log data to SD card                                                        //
////////////////////////////////////////////////////////////////////////////////
void logtosdcard()
{
  // yet to be implemented
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Display home screen (index 0)                                              //
////////////////////////////////////////////////////////////////////////////////
void displayscreenhome()
{
  lcd.setCursor(0, 0);
  lcd.print("WAT:");
  lcd.setCursor(8, 0);
  lcd.print("OIL:");
  lcd.setCursor(0, 1);
  lcd.print("AIR:");
  lcd.setCursor(8, 1);
  lcd.print("BAT:");
  lcd.setCursor(4, 1);
  lcd.print("    ");
  lcd.setCursor(4, 1);
  lcd.print(airtemp,0);
  lcd.setCursor(12, 1);
  lcd.print("    ");
  lcd.setCursor(12, 1);
  lcd.print(battvolt,1);
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Display water temperature screen (index 1)                                 //
////////////////////////////////////////////////////////////////////////////////
void displayscreenwater()
{
  lcd.setCursor(4, 0);
  lcd.print("WAT:");
  lcd.setCursor(8, 0);
  lcd.print(watertemp);
  lcd.setCursor(0, 1);
  lcd.print("MIN:");
  lcd.setCursor(4, 1);
  lcd.print(watertempmin);
  lcd.setCursor(9, 1);
  lcd.print("MAX:");
  lcd.setCursor(13, 1);
  lcd.print(watertempmax);
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Display oil temperature screen (index 2)                                   //
////////////////////////////////////////////////////////////////////////////////
void displayscreenoil()
{
  lcd.setCursor(4, 0);
  lcd.print("OIL:");
  lcd.setCursor(8, 0);
  lcd.print(oiltemp);
  lcd.setCursor(0, 1);
  lcd.print("MIN:");
  lcd.setCursor(4, 1);
  lcd.print(oiltempmin);
  lcd.setCursor(9, 1);
  lcd.print("MAX:");
  lcd.setCursor(13, 1);
  lcd.print(oiltempmax);
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Display air temperature screen (index 3)                                   //
////////////////////////////////////////////////////////////////////////////////
void displayscreenair()
{
  lcd.setCursor(4, 0);
  lcd.print("AIR:");
  lcd.setCursor(8, 0);
  lcd.print(airtemp, 0);
  lcd.setCursor(0, 1);
  lcd.print("MIN:");
  lcd.setCursor(4, 1);
  lcd.print(airtempmin, 0);
  lcd.setCursor(9, 1);
  lcd.print("MAX:");
  lcd.setCursor(13, 1);
  lcd.print(airtempmax, 0);
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Display battery voltage screen (index 4)                                   //
////////////////////////////////////////////////////////////////////////////////
void displayscreenbatt()
{
  lcd.setCursor(4, 0);
  lcd.print("BAT:");
  lcd.setCursor(8, 0);
  lcd.print(battvolt, 1);
  lcd.setCursor(0, 1);
  lcd.print("MN:");
  lcd.setCursor(3, 1);
  lcd.print(battvoltmin, 1);
  lcd.setCursor(9, 1);
  lcd.print("MX:");
  lcd.setCursor(12, 1);
  lcd.print(battvoltmax, 1);
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Display accelerometer screen (index 5)                                     //
////////////////////////////////////////////////////////////////////////////////
void displayscreenaccel()
{
  lcd.setCursor(0, 0);
  lcd.print("LON:");
  lcd.setCursor(4, 0);
  lcd.print(glon, 1);
  lcd.setCursor(9, 0);
  lcd.print("MX:");
  lcd.setCursor(12, 0);
  lcd.print(glonmax, 1);
  lcd.setCursor(0, 1);
  lcd.print("LAT:");
  lcd.setCursor(4, 1);
  lcd.print(glon, 1);
  lcd.setCursor(9, 1);
  lcd.print("MX:");
  lcd.setCursor(12, 1);
  lcd.print(glonmax, 1);
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Display RPM screen (index 6)                                               //
////////////////////////////////////////////////////////////////////////////////
void displayscreenrpm()
{
  lcd.setCursor(4, 0);
  lcd.print("RPM:");
  lcd.setCursor(8, 0);
  lcd.print("     ");
  lcd.setCursor(8, 0);
  lcd.print(rpm);
  lcd.setCursor(4, 1);
  lcd.print("MAX:");
  lcd.setCursor(8, 1);
  lcd.print("     ");
  lcd.setCursor(8, 1);
  lcd.print(rpmmax);
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Display alarm screen (index 7)                                             //
////////////////////////////////////////////////////////////////////////////////
void displayscreenalarm()
{
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Light LEDs to indicate RPM                                                 //
////////////////////////////////////////////////////////////////////////////////
void displayleds(int revs)
{
  for (int displed = 0; displed < ledCount; displed++)
  {
    if (revs > (displed*multiplier)) 
    {
      digitalWrite(ledPins[displed], HIGH);
    }
    else
    {
      digitalWrite(ledPins[displed], LOW);
    }
  }  
  return;
}

////////////////////////////////////////////////////////////////////////////////
// RPM Interrupt                                                              //
////////////////////////////////////////////////////////////////////////////////
void tachpulse()
{
  rpmcount++;     // increment rpm counter every time tacho pulse occurs
  return;
}

