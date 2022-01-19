/**
 * Magic Orb testing code
 * Lights will flash R, G, B, W 3times
 * Then will turn white whenever motion is detected
 * 
 * @author Taylor Giles
 */


//Pins
#define RED_PIN 5       //D1
#define GREEN_PIN 4     //D2
#define BLUE_PIN 0      //D3
#define SENSOR_PIN 13   //D7

long lastTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  //Check lights
  for(int i = 0; i < 3; i++){
    allOff();
    Serial.println("R");
    digitalWrite(RED_PIN, HIGH);
    delay(1000);
    allOff();
    Serial.println("G");
    digitalWrite(GREEN_PIN, HIGH);
    delay(1000);
    allOff();
    Serial.println("B");
    digitalWrite(BLUE_PIN, HIGH);
    delay(1000);
    allOff();
    Serial.println("W");
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, HIGH);
    delay(1000);
  }
}

void loop() {
  if(digitalRead(SENSOR_PIN) == HIGH){
    Serial.println("Motion");
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, HIGH);
  } else {
    allOff();
  }

}

void allOff(){
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
}
