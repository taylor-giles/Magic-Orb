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
#define WIFI_SSID "SECRET"
#define WIFI_PASSWORD "SECRET"

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
#define RED_PIN 15
#define GREEN_PIN 12
#define BLUE_PIN 14
#define SENSOR_PIN 4

//Firebase objs
FirebaseData fb_data;
FirebaseAuth fb_auth;
FirebaseConfig fb_config;

//Program values
unsigned long timeLastUpdated = 0;
unsigned long timeLastSent = 0;

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
Color myColor = red;


void setup(){
  //Pin modes
  pinMode(SENSOR_PIN, INPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  //Connect to WiFi
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

  //Set up Firebase credentials
  fb_config.api_key = API_KEY; /* Assign the api key (required) */
  fb_config.database_url = DATABASE_URL; /* Assign the RTDB URL (required) */

  //Sign in to Firebase
  Serial.println("Signing into Firebase...");
  if (Firebase.signUp(&fb_config, &fb_auth, "", "")){
    Serial.println("Success!");
  } else {
    Serial.println("Firebase sign-in failed.");
    Serial.printf("%s\n", fb_config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  fb_config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  //Begin Firebase
  Firebase.begin(&fb_config, &fb_auth);
  Firebase.reconnectWiFi(true);
}


void loop(){
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

    //If this color was just turned on and was not on before...
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

    //Ensure display is correct
    //(Deactivate wont occur for colors that this orb turns off)
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
 * Returns <true> if at least <maxDelay> millis have passed since the 
 * time specified by <startTime>
 */
bool checkDelay(long startTime, long maxDelay){
  return millis() - startTime >= maxDelay;
}
