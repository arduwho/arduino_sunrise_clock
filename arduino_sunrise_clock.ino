// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>  //This version of the library for the display with "I2C Backpack"

RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

char daysOfTheWeek[7][12] = {"Sunday    ", "Monday    ", "Tuesday   ", "Wednesday ", "Thursday  ", "Friday    ", "Saturday  "};
byte buffer;                       //variable to hold data for length check
String buffers;                    //result of variable length check
byte lightPin = 11;                  //PWM output pin for sunrise light
byte ledPin = 10;                  //PWM output pin for display
byte ledVal = 1;                  //set lowest possible initial value for display brightness
byte dimPotPin = 14;                  //analog input from display dimmer pot
byte buttonModePin = 3;            //run mode change button
byte buttonPlusPin = 5;            //increment for setting
byte buttonMinusPin = 6;            //decrement for setting
byte buttonSnoozePin = 9;
byte currentGUIMode = 0;           //defined modes are run(0), clockset(1), alarmset(2), prealarmset(3), postalarmset(4)
byte currentAlarmMode = 1;         //defined modes are off(0), run(1), snooze(2)
byte snoozeDuration = 5;          //not an actual snooze but a temporary dimming.  Probably more useful if the light is uncomfortably bright as you crawl out of bed.
byte snoozeOffset = 50;           //dimming value for PWM when snooze pressed.
byte startBrightness = 1;         //initial PWM value when light turns on.
byte currentBrightness = 0;       //current value of PWM light output
long alarmCalcHour;               //
long alarmCalcMinute;               //
long alarmCalcSecond = 0;          //Created to prevent weird problems in time calculation when time rolls over a minute
long secondsToAlarm;               //
boolean initialStart;             //If true, sends user through all setup menus (assumes initial start or blank RTC)

//  Alarm clock settings which will eventually be stored/retrieved from nvram on the RTC
byte alarmHour = 6;                  //wake hour
byte alarmMinute = 40;                //wake minute
byte alarmSecond = 0;
byte alarmPreMinute = 25;             //number of minutes before alarm set time when light starts
byte alarmPostMinute = 20;            //number of minutes light will remain on after alarm time
byte maxBrightness = 200;         //maximum PWM value when light on.

// Alarm clock settings which will be retrieved from RTC nvram
//byte alarmHour;
//byte alarmMinute;
//byte alarmPreMinute;
//byte alarmPostMinute;
//byte maxBrightness;

void setup () {
  lcd.init();                      // initialize the lcd
  lcd.backlight();                 // turn on backlight.  Unnecessary due to AVR-controlled brighness but useful in case display brightness jumper is reinstalled 
  pinMode(ledPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
  pinMode(dimPotPin, INPUT);
  analogWrite(ledPin,ledVal);     //write display brightness to output pin
  
  while (!Serial);                // for Leonardo/Micro/Zero
  Serial.begin(57600);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  //rtc.adjust(DateTime(2017, 12, 8, 20, 33, 0));  //set this to current time before first run of RTC.  Comment out then upload again.

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(2017, 12, 9, 0, 0, 0));  //RTC is not running, so set this startup date to get nearer to 'now'
    initialStart = true;                           //RTC was not set, so trigger all setup menus in sequence    
  }

  //readSettings();                 //Read alarm settings from the RTC nvram

  //delay(1000); //Delay to allow RTC to start up.  Prevents garbage data on first display of clock.

  // This block of code reads the alarm settings out of the nvram of the RTC
}

void loop () {
  
  // CODE TO CALL SETUP FUNCTIONS BY SETTING initialStart == TRUE. TRIGGERED BY SOME BUTTON SEQUENCE (ALL 3 BUTTONS 5 SECONDS??)
  
  //checkSettings();                //Check if saved alarm settings have impossible values.  Doesn't guarantee they aren't nonsensical! 

  // AN IF STATEMENT TO CALL ALL OF THE SETUP MENUS IN ORDER IF AN INITIAL STARTUP IS DETECTED

  // SOME CODE TO READ MODE BUTTON AND DECIDE IF WE SHOULD CYCLE currentGUIMode OR RETURN TO currentGUIMode == 0 AFTER TIMEOUT
  
  // CODE TO CALL APPROPRIATE SETUP MENUS BASED ON VALUE OF currentGUIMode 

  timeToAlarm();
  setBacklight();
  setLamp();
       
  DateTime now = rtc.now();
  lcd.setCursor(0,0);
  lcd.print(daysOfTheWeek[now.dayOfTheWeek()]);
  lcd.print(" ");
  buffer = now.hour();    
  if (buffer < 10) { buffers = "0" + String(buffer); } else { buffers = String(buffer); }    
  lcd.print(buffers);
  lcd.print(':');
  buffer = now.minute();
  if (buffer < 10) { buffers = "0" + String(buffer); } else { buffers = String(buffer); }    
  lcd.print(buffers);
  lcd.print(now.minute(), DEC);
  lcd.setCursor(0,1);
  buffer = now.day();
  if (buffer < 10) { buffers = "0" + String(buffer); } else { buffers = String(buffer); }    
  lcd.print(buffers);
  lcd.print('/');
  buffer = now.month();
  if (buffer < 10) { buffers = "0" + String(buffer); } else { buffers = String(buffer); }    
  lcd.print(buffers);
  lcd.print('/');
  lcd.print(now.year());
  lcd.print("      ");
}

void timeToAlarm() {
  DateTime now = rtc.now();
  DateTime dt0 (now.year(),now.month(),now.day(),now.hour(),now.minute(),now.second());
  Serial.print(dt0.unixtime());
  Serial.print(" ");
  DateTime dt1 (now.year(),now.month(),now.day(),alarmHour,alarmMinute,alarmSecond);
  Serial.print(dt1.unixtime());
  Serial.print(" ");
  secondsToAlarm = dt1.unixtime()-dt0.unixtime();
  if (secondsToAlarm<0) {
    secondsToAlarm += 86400;
  }
  Serial.print(secondsToAlarm);
  Serial.print(" ");  
}

void readSettings() {
  uint8_t readData[5] = {0};
  rtc.readnvram(readData, 5, 0);
  alarmHour = readData[0];
  alarmMinute = readData[1];
  alarmPreMinute = readData[2];
  alarmPostMinute = readData[3];
  maxBrightness = readData[4];
}
    
void checkSettings() {
  if (alarmHour<0 || alarmHour>23 || alarmMinute<0 || alarmMinute>59 || alarmPreMinute<0 || alarmPostMinute<0 || maxBrightness<1 || maxBrightness>255) {
    initialStart = true;
  }
}

// calculate sunrise light level based on current time and alarm time
// if time is between alarm time and preAlarmMinutes earlier then calculate brightness ramping up to maxBrightness

void setLamp() {
  if (secondsToAlarm<alarmPreMinute*60){
     currentBrightness = ((alarmPreMinute*60-secondsToAlarm)*maxBrightness)/(alarmPreMinute*60);
  }
  else if (secondsToAlarm>(86400-alarmPostMinute*60)) {
     currentBrightness = maxBrightness;
  }
  else {
     currentBrightness = 0;
  }
  analogWrite(lightPin,currentBrightness); //set new brightness to PWM output for sunrise light
  Serial.println(currentBrightness);                //show calculated value for display brightness
}

void setBacklight() {
  // IF STATEMENT TO MAKE LCD DISPLAY BRIGHTNESS FOLLOW SUNRISE LIGHT BRIGHTNESS DURING LIGHT-ON TIMES.  OTHERWISE LET IT FOLLOW LIGHT SENSOR.
  // MAP DISPLAY BRIGHTNESS FROM SUNRISE LIGHT BRIGHTNESS SO DISPLAY CAN REACH 255 PWM EVEN IF SUNRISE LIGHT IS LIMITED OUT TO A LOWER VALUE.
  if (currentBrightness>0) {
    ledVal = currentBrightness;
  }
  else {
    ledVal = ((ledVal * 6) + constrain(map(analogRead(dimPotPin),0,100,3,255),3,255))/7; //read dimmer pot,map to appropriate output value, apply filter to average values out and avoid flicker
  }
  analogWrite(ledPin,ledVal); //set new brightness to PWM output for display
  Serial.print(analogRead(dimPotPin));                //show measured input for display brightness
  Serial.print("     ");  
  Serial.print(ledVal);                //show calculated value for display brightness
  Serial.print("     ");                
}


    
    // SOME CODE HERE TO READ THE ALARM TIME PARAMETER STORED IN THE EXTRA MEMORY OF THE RTC/
    
    // SOME CODE HERE TO SET THE CLOCK IF initialStart == TRUE OR clockIsSet == FALSE OR currentGUIMode == 1

    // SOME CODE HERE TO SET THE ALARM IF initialStart == TRUE OR alarmIsSet == FALSE OR currentGUIMode == 2
    
    // SOME CODE HERE TO SET preAlarmMinutes IF initialStart == TRUE OR currentGUIMode == 3

    // SOME CODE HERE TO SET postAlarmMinutes IF initialStart == TRUE OR currentGUIMode == 4
