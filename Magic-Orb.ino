/*
 * Magic Orbs - A set of IoT orbs that change color when they are pondered.
 * Connected using Firebase Realtime Database
 * 
 * Using the Firebase ESP8266 tutorial by 
 * Rui Santos https://RandomNerdTutorials.com/esp8266-nodemcu-firebase-realtime-database/
 * 
 * @author Taylor Giles
*/

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"

//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Network credentials
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASS"

// Firebase API Key (Project Settings -> Web API Key)
#define API_KEY "KEY"

// URL of the Realtime Database
#define DATABASE_URL "URL" 

//Number of millis between updates
#define UPDATE_DELAY 10000

//Number of millis between sends 
#define SEND_DELAY 5000

//Duration (in millis) to allow a light to stay on
#define LIGHT_DURATION 120000

//Firebase objs
FirebaseData fb_data;
FirebaseAuth fb_auth;
FirebaseConfig fb_config;

//Program values
unsigned long millisLastUpdated = 0;
unsigned long millisLastSent = 0;
unsigned long millisLastPondered = 0;
int count = 0;
bool signupOK = false;

//Color values
uint8_t red = 0;
uint8_t blue = 0;
uint8_t green = 0;
int prevRed = 0, prevGreen = 0, prevBlue = 0;
int redLastSeen = 0, greenLastSeen = 0, blueLastSeen = 0;

//Pins
const uint8_t RED_PIN = 15;
const uint8_t GREEN_PIN = 12;
const uint8_t BLUE_PIN = 14;
const uint8_t SENSOR_PIN = 4;


void setup(){
  pinMode(SENSOR_PIN, INPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  fb_config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  fb_config.database_url = DATABASE_URL;

  /* Sign up */
  Serial.println("Signing into Firebase...");
  if (Firebase.signUp(&fb_config, &fb_auth, "", "")){
    Serial.println("Success!");
    signupOK = true;
  } else {
    Serial.println("Firebase sign-in failed.");
    Serial.printf("%s\n", fb_config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  fb_config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&fb_config, &fb_auth);
  Firebase.reconnectWiFi(true);
}


void loop(){
  //Update if enough time has passed
  if(millis() - millisLastUpdated > UPDATE_DELAY){
    updateColor();
    millisLastUpdated = millis();
  }

  //Detect pondering
  if(digitalRead(SENSOR_PIN) == HIGH){
    Serial.println("Motion detected.");
    onPonder();
  }

  checkColorsForTurnoff();
}


void updateColor(){
  int updatedRed = getColor("r");
  int updatedGreen = getColor("g");
  int updatedBlue = getColor("b");
  if(updatedRed && redLastSeen <= 0){
    Serial.println("Red updated");
    redLastSeen = millis();
  }
  if(updatedGreen && greenLastSeen <= 0){
    Serial.println("Green updated");
    greenLastSeen = millis();
  }
  if(updatedBlue && blueLastSeen <= 0){
    Serial.println("Blue updated");
    blueLastSeen = millis();
  }
  fadeRGB(updatedRed, updatedGreen, updatedBlue);
}


void onPonder(){
  millisLastPondered = millis();
  //Update this orb's color
  if(red){
    fadeRGB(red, prevGreen, prevBlue);
    if (sendColor(1, "r")){
      Serial.println("Successfully sent RED");
    }
  }
  if(green){
    fadeRGB(prevRed, green, prevBlue);
    if (sendColor(1, "g")){
      Serial.println("Successfully sent GREEN");
    }
  }
  if(blue){
    fadeRGB(prevRed, prevGreen, blue);
    if (sendColor(1, "b")){
      Serial.println("Successfully sent BLUE");
    }
  }
}


bool sendColor(int color, String colorId){
  if((millis() - millisLastSent) > SEND_DELAY){
    millisLastSent = millis();
    return (Firebase.RTDB.setInt(&fb_data, "c/" + colorId, color));
  }
  return false;
}


int getColor(String colorId){
  if (Firebase.RTDB.getInt(&fb_data, "/c/" + colorId)) {
    if (fb_data.dataType() == "int") {
      return fb_data.intData() * 255;
    }
  }
  return -1;
}


/**
 * Turn off any light that has been on for longer than the specified duration
 */
void checkColorsForTurnoff(){
  //Red
  if(redLastSeen > 0 && millis() - redLastSeen > LIGHT_DURATION){
    Serial.println("Turning off red");
    sendColor(0, "r");
    redLastSeen = 0;
  }

  //Green
  if(greenLastSeen > 0 && millis() - greenLastSeen > LIGHT_DURATION){
    Serial.println("Turning off green");
    sendColor(0, "g");
    greenLastSeen = 0;
  }

  //Blue
  if(blueLastSeen > 0 && millis() - blueLastSeen > LIGHT_DURATION){
    Serial.println("Turning off blue");
    sendColor(0, "b");
    blueLastSeen = 0;
  }
}


/**
 * Fades into the specified RGB color values
 */
void fadeRGB(int redVal, int greenVal, int blueVal){
  //Calculate intervals for fading
  double redInterval = ((double)redVal - (double)prevRed) / 255.0;
  double greenInterval = ((double)greenVal - (double)prevGreen) / 255.0;
  double blueInterval = ((double)blueVal - (double)prevBlue) / 255.0;
  double red = prevRed, green = prevGreen, blue = prevBlue;

  //Fade into new color
  for(int i = 0; i < 255; i++){
    red += redInterval;
    green += greenInterval;
    blue += blueInterval;

    //Set values to LED
    analogWrite(RED_PIN, map(red, 0, 255, 0, 1023));
    analogWrite(GREEN_PIN, map(green, 0, 255, 0, 1023));
    analogWrite(BLUE_PIN, map(blue, 0, 255, 0, 1023));

    delay(1);
  }

  prevRed = redVal;
  prevGreen = greenVal;
  prevBlue = blueVal;
  
  //Set values to LED
  analogWrite(RED_PIN, map(redVal, 0, 255, 0, 1023));
  analogWrite(GREEN_PIN, map(greenVal, 0, 255, 0, 1023));
  analogWrite(BLUE_PIN, map(blueVal, 0, 255, 0, 1023));
}
