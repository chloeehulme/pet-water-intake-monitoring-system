/**
   Detects temperature, humidity and force, and logs it with timestamps to a .csv file in SDCard.
   This program is designed for a smart pet water bowl to gain insights into your pets drinking habits, 
   the bowl will also notify you when it needs refilling via a red blinking light.
   Uses  HCSR505 PIR Passive Infra Red Motion Detector, 
   Adafruit NeoPixel Stick,
   DHT 22 Humidity and Temperature Sensor,
   RP-S40-ST Thin Film Pressure Sensor.
   Modified code from Niroshinie Fernando: https://github.com/FeifeiDeakin/MotionDataLogging
   Author: Chloe Hulme
*/

//Libraries //
#include "SD.h"
#include <Wire.h>
#include <DHT.h>
#include "RTClib.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

// SD Card and file setup //
#define SYNC_INTERVAL 600000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()
#define ECHO_TO_SERIAL   1 // echo data to serial port. 
RTC_PCF8523 RTC; // define the Real Time Clock object
const int chipSelect = 10; // pin for SD card
File logfile; // define the logging file object

// Sensor setup //
#define MOTION_PIN 5 // set motion sensor pin to digital 5
#define TEMP_HUMID_PIN 7 // change to whatever needed
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(TEMP_HUMID_PIN, DHTTYPE); // initialize DHT sensor object 
int FSR_Pin = A0; // set sensor pin to analog pin 0
const int FSR_empty_bowl = 940; // FSR value indicating ~200ml left in jug 
float humValue;  // stores humidity value
float tempValue; // stores temperature value
int FSRValue; // stores FSR value

// LED setup //
#define NEO_PIN 6 // set neo pixel stick pin
#define NUM_PIXELS 8 // number of pixels on stick
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800); // create neo pixel object
uint32_t white = strip.Color(127, 127, 127); // create colour white
uint32_t red = strip.Color(255, 0, 0); // create colour red 

// Timer variables //
unsigned long startMillis; // timer first instance (start of period)
unsigned long currentMillis; // time keeping variable (keeps track of time elapsed)
const unsigned long ten_mins = 600000; // 10 minutes, max period of time elapsed

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  // initialise input pins //
  pinMode(MOTION_PIN, INPUT);
  pinMode(TEMP_HUMID_PIN, INPUT);

  // initialise neo pixel stick //
  strip.begin();
  strip.setBrightness(50);
  strip.show(); // Initialize all pixels to 'off' 

  // initialize the SD card //
  initSDcard();

  // create a new file //
  createFile(); 

  // connect to RTC //
  initRTC(); 

  // create headers for CSV file //
  logfile.println("millis,stamp,datetime,temp_(C),humidity%,fsr_reading");
}

void loop() {

  // gets current time elapsed, log data to file if 10 minutes has elapsed //
  currentMillis = millis();
  if (currentMillis - startMillis >= ten_mins )
  {
     DateTime now;
  
     // log miliseconds since starting //
     uint32_t m = millis();
     logfile.print(m);     // milliseconds since start
     logfile.print(", ");
  
     // fetch the time //
     now = RTC.now();
     // log time //
     logfile.print(now.unixtime()); // seconds since 2000
     logfile.print(", ");
     logfile.print(now.year(), DEC);
     logfile.print("/");
     logfile.print(now.month(), DEC);
     logfile.print("/");
     logfile.print(now.day(), DEC);
     logfile.print(" ");
     logfile.print(now.hour(), DEC);
     logfile.print(":");
     logfile.print(now.minute(), DEC);
     logfile.print(":");
     logfile.print(now.second(), DEC);
  
     // read in current sensor values and store in variables //
     FSRValue = analogRead(FSR_Pin);
     humValue = dht.readHumidity();
     tempValue = dht.readTemperature();
  
     // log data to file // 
     logfile.print(", ");
     logfile.print(tempValue);
     logfile.print(", ");
     logfile.print(humValue);
     logfile.print(", ");
     logfile.println(FSRValue);
  
    // flush data from buffer //
    if ((millis() - syncTime) < SYNC_INTERVAL) return;
    syncTime = millis();
    logfile.flush();
  
     // re-start timer //
     startMillis = currentMillis;
     delay(2000);
  }
  
  // get FSR value and check if jug needs refilling //
  FSRValue = analogRead(FSR_Pin);
  if (FSRValue <= FSR_empty_bowl)
  {
     RefillJug();
  }
  else if (digitalRead(MOTION_PIN) == HIGH)
  {
     WhiteLight();
  }
  
}


// initialise the SD card //
void initSDcard()
{
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

}

// create a CSV file to write to //
void createFile()
{
  //file name must be in 8.3 format (name length at most 8 characters, follwed by a '.' and then a three character extension.
  char filename[] = "MLOG00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[4] = i / 10 + '0';
    filename[5] = i % 10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE);
      break;  // leave the loop!
    }
  }
}

// initialise the real time clock //
void initRTC()
{
  Wire.begin();
  if (!RTC.begin()) {
    logfile.println("RTC failed");
  }
}

// blink red light when bowl is empty //
void RefillJug()
{
   strip.fill(red);
   strip.show();
   delay(500);
   strip.clear();
   strip.show();
   delay(500);
}

// display white light for 10 seconds //
void WhiteLight()
{
   strip.fill(white);
   strip.show();
   delay(10000);
   strip.clear();
   strip.show();
}
