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
long refreshledsperiod = 200;
long refreshledsold = 0;
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
float glon = 0;
float glonmax = 0;
float glat = 0;
float glatmax = 0;
int rpm = 0;
int rpmvalue = 0;
int multiplier = 500;
volatile int rpmcount = 0;
int rpmmax = 0;
int rpmredline = 6000;
int alarmreason = 0;
boolean leftkeydown = false;
boolean rightkeydown = false;
int readleftkey = LOW;
int readrightkey = LOW;
long readleftkeyold = 0;
long readrightkeyold = 0;
long readkeyperiod = 200;
unsigned long strobeledsold = 0;
int strobeledsperiod = 100;
boolean strobeleds = false;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(36, 34, 32, 30, 28, 26);

void setup()
{
  Serial.begin(9600);
  attachInterrupt(0, tachpulse, CHANGE);

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
      Serial.println("Left button press!" );
      screen++;
      if (screen > 6)
      {
        screen = 0;
      }
      Serial.print("Screen=");
      Serial.println(screen);
      clearscreen();
    }
  }  
  if (currentmillis - readrightkeyold > readkeyperiod)
  {
    readrightkey = digitalRead(3);
    if (readrightkey == LOW)
    {
      readrightkeyold = millis();
      Serial.println("Right button press!" );
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
  
  //refresh leds
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
  float voltage = reading * 5.0;
  voltage /= 1024;
  float airtempvalue = (voltage - 0.5) * 100 ;
  if (airtempvalue < airtempmin)
  {
    airtempmin = airtempvalue;
  }  
  if (airtempvalue > airtempmax)
  {
    airtempmax = airtempvalue;
  }  
  return airtempvalue;
}  

int readrpm()
{
  rpmvalue = (rpmcount*150);  // *60 secs  /2 state changes  *5 samples per sec
  rpmcount = 0;
  if (rpm > rpmmax)
  {
    rpmmax = rpm;
  }    
  return rpmvalue;
}

void logtosdcard()
{
  return;
}

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

void displayscreenrpm()
{
  lcd.setCursor(4, 0);
  lcd.print("RPM:");
  lcd.setCursor(8, 0);
  lcd.print("    ");
  lcd.setCursor(8, 0);
  lcd.print(rpm);
  lcd.setCursor(4, 1);
  lcd.print("MAX:");
  lcd.setCursor(8, 1);
  lcd.print("    ");
  lcd.setCursor(8, 1);
  lcd.print(rpmmax);
  return;
}

void displayscreenalarm()
{
  return;
}

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

void tachpulse()
{
  rpmcount++;
  digitalWrite(52, HIGH);
  delay(250);
  digitalWrite(52, LOW);
  return;
}

