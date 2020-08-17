#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "NTPtimeESP.h"
#include "defines.h"

NTPtime NTPch("ch.pool.ntp.org");
unsigned long actual_brightness = 0;
bool FLAG_LED_WORKING = false;
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
unsigned long startTime = (START_HOUR * 60) + START_MIN;
unsigned long nightLightStartTime = (NIGHT_LIGHT_START_HOUR * 60) + NIGHT_LIGHT_START_MIN;
unsigned long endTime = (END_HOUR * 60) + END_MINUTE;
strDateTime dateTime;

void confirmBlink()
{
  #ifdef DEBUG
  Serial.println("Connected!");
  #endif
  analogWrite(LED_STRIP_0, 300);
  delay(500);
  analogWrite(LED_STRIP_0, 0);
  analogWrite(LED_STRIP_1, 30);
  delay(500);
  analogWrite(LED_STRIP_1, 0);
}

void waitIndicator()
{
  #ifdef DEBUG
  Serial.println("connecting...");
  #endif
  analogWrite(LED_STRIP_0, 30);
  delay(1500);
  analogWrite(LED_STRIP_0, 0);
}

void rampUp(unsigned long targetPwm){
  while(actual_brightness < targetPwm){
    actual_brightness += 1;
    analogWrite(LED_STRIP_0, actual_brightness);
    analogWrite(LED_STRIP_1, actual_brightness);
    if(actual_brightness > 25)
    {
      delay(1000);
    }
    else
    {
      delay(3000); 
    }
    #ifdef DEBUG
    Serial.println("brightness: " + (String)actual_brightness);
    #endif
  }
  FLAG_LED_WORKING = true;
}

void rampDown(unsigned long targetPwm){
  while(actual_brightness > targetPwm){
    actual_brightness -= 1;
    analogWrite(LED_STRIP_0, actual_brightness);
    analogWrite(LED_STRIP_1, actual_brightness);
    if(actual_brightness < targetPwm + 100)
    {
      delay(2000);
    }
    else
    {
      delay(500); 
    }
    #ifdef DEBUG
    Serial.println("brightness: " + (String)actual_brightness);
    #endif
  }
  if(targetPwm != 0)
  {
    FLAG_LED_WORKING = true;
  }
  else
  {
    FLAG_LED_WORKING = false;
  }
}

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif
  analogWrite(LED_STRIP_0, 0);
  analogWrite(LED_STRIP_1, 0);
  WiFi.mode(WIFI_STA);
  WiFi.begin (ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    waitIndicator();
    delay(500);
  }
  confirmBlink();
  NTPch.setSendInterval(TIME_CHECK_INTERVAL);
  NTPch.setRecvTimeout(TIME_CHECK_TIMEOUT);

#ifdef DEBUG
  Serial.println("starttime: " + (String)startTime);
  Serial.println("endtime: " + (String)endTime);
#endif
}

void loop()
{
  /*If TESTBUILD isn't defined, it's a normal release build*/
  #ifndef TESTBUILD 

  // first parameter: Time zone in floating point (for India); second parameter: 1 for European summer time; 2 for US daylight saving time; 0 for no DST adjustment; (contributed by viewwer, not tested by me)
  dateTime = NTPch.getNTPtime(1.0, 1);
  unsigned long currentTimeInMinutes = ((int)dateTime.hour * 60) + (int)dateTime.minute;
  if(dateTime.valid)
  {
  #ifdef DEBUG
    Serial.println("\ncurrent: " + (String)currentTimeInMinutes);
    Serial.println("\nflag:" + (String)FLAG_LED_WORKING);
  #endif
    /*If year is 2036, time is invalid, do nothing*/
    if(dateTime.year != 2036)
    {
      if(!FLAG_LED_WORKING)
      {
        if(currentTimeInMinutes > startTime && currentTimeInMinutes < nightLightStartTime)
        {
          rampUp(MAX_LED_PWM);
        }
        #ifdef NIGHT_LIGHT
        else if (currentTimeInMinutes > nightLightStartTime && currentTimeInMinutes < endTime)
        {
          rampDown(NIGHT_LIGHT_MAX_PWM);
        }
        #endif
        else if(currentTimeInMinutes > endTime)
        { 
          rampDown(0);
        }
      }
    }
  }
  /*Wait 1 minute before requesting NTP time or doing anything else*/
  delay(60000);

  #else/*For testing purposes, LEDs will start ramping up right away*/
  rampUp(MAX_LED_PWM);
  rampDown(0);
  #endif
}