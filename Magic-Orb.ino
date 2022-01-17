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

//Library for getting WiFi credentials automatically from AP webpage
#include <WiFiManager.h>

// Credentials for WiFiManager Access Point (for config page)
#define WM_SSID "SECRET"
#define WM_PASSWORD "SECRET"

// Firebase API Key (Project Settings -> Web API Key)
#define API_KEY "SECRET"

// URL of the Realtime Database
#define DATABASE_URL "SECRET" 

//Number of millis between updates
#define UPDATE_DELAY 10000

//Min. number of millis between sends 
#define SEND_DELAY 5000

//Duration (in millis) to allow a light to stay on
#define LIGHT_DURATION 120000

//Number of colors
#define NUM_COLORS 3

//Pins
#define RED_PIN 5       //D1
#define GREEN_PIN 4     //D2
#define BLUE_PIN 0      //D3
#define SELECT_GREEN 14 //D5
#define SELECT_BLUE 12  //D6
#define SENSOR_PIN 13   //D7

//Firebase objs
FirebaseData fb_data;
FirebaseAuth fb_auth;
FirebaseConfig fb_config;

//Program values
unsigned long timeLastUpdated = 0;
unsigned long timeLastSent = 0;

//WiFiManager-related values
bool isConnected = false;
unsigned long timeLastBlinked = 0;
bool isBlinkOn = false;
const int BLINK_DELAY = 1000; //ms between blinks

//A struct to store data for each of red, green, and blue
typedef struct color {
  String id;
  long timeLastActivated;
  bool isOn;
  uint8_t pin;
} Color;

//Save the colors as structs for easy access with loops
Color red{"r", 0, false, RED_PIN};
Color green{"g", 0, false, GREEN_PIN};
Color blue{"b", 0, false, BLUE_PIN};
Color colors[3] = {red, green, blue};
Color myColor;

//WiFi Manager
WiFiManager wifiManager;


void setup(){
  //Pin modes
  pinMode(SENSOR_PIN, INPUT);
  pinMode(SELECT_GREEN, INPUT_PULLUP);
  pinMode(SELECT_BLUE, INPUT_PULLUP);
  for(int i = 0; i < 3; i++){
    pinMode(colors[i].pin, OUTPUT);
    digitalWrite(colors[i].pin, LOW);
  }
  Serial.begin(115200);
  Serial.println();

  //Determine which color this orb uses
  myColor = red;
  Serial.print(digitalRead(SELECT_GREEN));
  Serial.println(digitalRead(SELECT_BLUE));
  if(digitalRead(SELECT_GREEN) == LOW){
    myColor = green;
  } else if(digitalRead(SELECT_BLUE) == LOW){
    myColor = blue;
  }
  Serial.print("Chosen color: ");
  Serial.println(myColor.id);

  //Connect to WiFi using saved credentials. If connection fails, open config page with AP
  wifiManager.setConfigPortalBlocking(false);
  isConnected = wifiManager.autoConnect(WM_SSID, WM_PASSWORD);
  if(isConnected){
    //Set up Firebase credentials
    fb_config.api_key = API_KEY; /* Assign the api key (required) */
    fb_config.database_url = DATABASE_URL; /* Assign the RTDB URL (required) */
  
    //Sign in to Firebase
    Serial.println("Signing into Firebase...");
    if (Firebase.signUp(&fb_config, &fb_auth, "", "")){
      Serial.println("Success!");
      /* Assign the callback function for the long running token generation task */
      fb_config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
    
      //Begin Firebase
      Firebase.begin(&fb_config, &fb_auth);
      Firebase.reconnectWiFi(true);
    } else {
      isConnected = false;
      Serial.println("Firebase sign-in failed.");
      Serial.printf("%s\n", fb_config.signer.signupError.message.c_str());
    }
  } else {
    Serial.println("Connection failed.");
    Serial.println("Starting config page with AP...");
  }
}


void loop(){
  if(isConnected){
    //Update if enough time has passed
    if(checkDelay(timeLastUpdated, UPDATE_DELAY)){
      updateDisplay();
    }
  
    //Detect pondering
    if(digitalRead(SENSOR_PIN) == HIGH){
      Serial.println("Motion detected.");
      onPonder();
    }
  
    //Turn off any colors that need to be turned off
    checkColorsForTurnoff();
  } else {
    //WifiManager config portal processing
    if(wifiManager.process()){
      ESP.restart();
    }

    //Blink the red LED
    if(checkDelay(timeLastBlinked, BLINK_DELAY)){
      timeLastBlinked = millis();
      digitalWrite(red.pin, isBlinkOn);
      isBlinkOn = !isBlinkOn;
    }
  }
}


/**
 * Gets updated color values from Firebase and updates the orb's display
 */
void updateDisplay(){
  Serial.print("Updating... ");
  for(int i = 0; i < NUM_COLORS; i++){
    //Get the updated color
    int updatedColor = getColor(colors[i].id);
    Serial.print(colors[i].id);
    Serial.print(updatedColor);
    Serial.print(colors[i].isOn);
    Serial.print(" ");

    //If this color was just turned on and was off before...
    if(updatedColor > 0 && !colors[i].isOn){
      //Update the timeLastActivated
      colors[i].timeLastActivated = millis();

      //Activate the light
      activateColorDisplay(colors[i]);
    }

    //If this color was just turned off and was on before...
    if(updatedColor <= 0 && colors[i].isOn){
      //Deactivate the light
      deactivateColorDisplay(colors[i]);
    }
    
    //Update this color's "isOn" status
    colors[i].isOn = (updatedColor > 0);

    //Ensure display is correct (in case of fade failure (analog issues, etc.))
    digitalWrite(colors[i].pin, colors[i].isOn);
  }
  Serial.println();
  timeLastUpdated = millis();
}


/**
 * Callback for the activation of the orb - 
 * Activates the color of this orb in the database
 */
void onPonder(){
  //Activate this orb's color in the database
  if(setColor(1, myColor.id)){
    //Force an update from database
    updateDisplay();
    Serial.println("Successfully sent color");
  }
}


/**
 * Send the given value for the given color to the database
 * 
 * Returns false on failure or if the appropriate delay has not been satisfied, true otherwise.
 */
bool setColor(int value, String id){
  if(checkDelay(timeLastSent, SEND_DELAY)){
    timeLastSent = millis();
    Serial.print("Sending ");
    Serial.print(id);
    Serial.println(value);
    return (Firebase.RTDB.setInt(&fb_data, "c/" + id, value));
  }
  return false;
}


/**
 * Queries Firebase for the latest value of the specified color
 */
int getColor(String colorId){
  if (Firebase.RTDB.getInt(&fb_data, "/c/" + colorId)) {
    if (fb_data.dataType() == "int") {
      return fb_data.intData();
    }
  }
  return -1;
}


/**
 * Turn off any light that has been on for longer than the specified duration
 */
void checkColorsForTurnoff(){
  for(int i = 0; i < NUM_COLORS; i++){
    if(colors[i].isOn && checkDelay(colors[i].timeLastActivated, LIGHT_DURATION)){
      Serial.print("Turning off ");
      Serial.println(colors[i].id);
      if(setColor(0, colors[i].id)){
        Serial.println("Turnoff successful");
        deactivateColorDisplay(colors[i]);
        colors[i].isOn = false;
      }
    }
  }
}


/**
 * Fades the specified color from "off" to "on"
 */
void activateColorDisplay(Color color){
  for(int i = 0; i < 256; i++){
    analogWrite(color.pin, i);
    delay(1);  //Duration of animation = 256ms
  }
}

/**
 * Fades the specified color from "on" to "off"
 */
void deactivateColorDisplay(Color color){
  for(int i = 255; i >= 0; i--){
    analogWrite(color.pin, i);
    delay(1);  //Duration of animation = 256ms
  }
}


/**
 * Returns true if at least <maxDelay> millis have passed since the 
 * time specified by <startTime>, false otherwise
 */
bool checkDelay(long startTime, long maxDelay){
  return millis() - startTime >= maxDelay;
}
