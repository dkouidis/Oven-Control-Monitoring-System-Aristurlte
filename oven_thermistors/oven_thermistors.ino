#include <math.h>
#include <movingAvg.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Thermistor constants
#define SERIES  100000 // 100 kohm resistor
const float Beta = 3892.0;
const float Temp = 298.15; 

// Define ESP32 pins
int THERMISTORPINS[] = {13, 33, 32, 36};  
const int NUM_SENSORS = 4;               
const int POTENTIOMETER = 34;


float Vin = 3.3; 
float Vout = 0;
float sensorValue = 0;
float R_therm = 0; 
float steinhart = 0;
float avgTemp = 0;
movingAvg steinhartmvavg(3); 

const int zcd = 35; // Zero cross detection pin 
const int angleControl[] = {25,26,17,16};  // Trigger pins for TRIAC 
volatile bool zero_cross_detected = false;

// LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);


// PI Variables
int error = 0;
int previous_error = 0;
int totalError = 0;

float kp = 200;
float ki = 20;

float proportional = 0;
float integral = 0;
float pi = 0;


int target = 10;
int endGame = 75; // SET THIS AT THE WILLING TEMPERATURE

bool manualPI = false;

unsigned long timer = 0;
unsigned long timerIncrease = 0;
unsigned long timerMonitor = 0;
 

float read_average_temperature() {
    float totalTemp = 0;
    int count = 0;
    float tempReadings[NUM_SENSORS];

    // Read all sensors first
    for (int i = 0; i < NUM_SENSORS; i++) {
        int sensorValue = analogRead(THERMISTORPINS[i]);
        float Vout = Vin * (sensorValue / 4095.0); // 12-bit ADC
        float R_therm = SERIES * Vout / (Vin - Vout);
        float steinhart = log(R_therm / SERIES);
        steinhart = (1.0 / Temp) + (steinhart / Beta);
        steinhart = 1.0 / steinhart;
        float temperatureC = steinhart - 273.15;
        tempReadings[i] = temperatureC;
    }
    // Compute average of first 2
    float refAvg = (tempReadings[0] + tempReadings[1]) / 2.0;
    totalTemp = tempReadings[0] + tempReadings[1];
    count = 2;
    // Check 3rd and 4th sensors
    for (int i = 2; i < NUM_SENSORS; i++) {
        if (abs(tempReadings[i] - refAvg) <= 6.0) {
            totalTemp += tempReadings[i];
            count++;
        } 
    }
    avgTemp = totalTemp / count;
    return steinhartmvavg.reading(avgTemp);
}




// Monitor LCD
void monitor() {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Goal");
    lcd.setCursor(5,0);
    lcd.print(endGame);
    lcd.print(".00");
    lcd.print(" ");

    lcd.setCursor(11,0);
    lcd.print("T:");
    lcd.print(target);
    lcd.print("  ");

    lcd.setCursor(0,1);
    lcd.print("Temp");
    lcd.setCursor(5,1);
    lcd.print(read_average_temperature());

    lcd.setCursor(11,1);
    lcd.print("E:");
    lcd.setCursor(13,1);
    lcd.print(error);
    lcd.print("  ");
}


// PI Control
void pid() {

    previous_error = error;
    error = target - round(avgTemp); //or -steinhart

    if (manualPI) return;

    if (previous_error * error < 0) {
      totalError = 0;
    } else {
      totalError += error;
      totalError = constrain(totalError, -200, 200);
    }

    proportional = error * kp;
    integral = totalError * ki; 
    pi = proportional + integral; 
    
    pi = constrain(pi, 330, 2600);
}

// Increase Target
void increase() {
  if (round((endGame - target)) >= 1) {
    target += 1;
  }
}


void IRAM_ATTR zero_cross_detection() {
    zero_cross_detected = true;
}

void setup() { 
  pinMode(zcd, INPUT);
  pinMode(POTENTIOMETER, INPUT);

  for(int i=0; i<3; i++){
    pinMode(angleControl[i], OUTPUT);
  }
  
  for (int i = 0; i < NUM_SENSORS; i++) {
      pinMode(THERMISTORPINS[i], INPUT);
  }
  
  attachInterrupt(digitalPinToInterrupt(zcd), zero_cross_detection, FALLING);

  Serial.begin(115200);  // Start Serial Monitor

  lcd.init();
  lcd.backlight();
  lcd.home();
  
  timer = millis();
  timerMonitor = millis();
  timerIncrease = millis();

  steinhartmvavg.begin();
}

void loop() {
    //int potValue = analogRead(POTENTIOMETER);               // Read potentiometer
    //endGame = map(potValue, 0, 4095, 20, 100);              // Map to temp range
    //endGame = constrain(endGame, 20, 100);                  // Constrain just in case

    if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

        float newPI = input.substring(3).toFloat();
        newPI = constrain(newPI, 330, 2600);
        if (newPI >= 0) {
            pi = newPI;
            manualPI = true;
            Serial.print("Manual PI set to: ");
            Serial.println(pi);
        } else {
            Serial.println("Invalid PI value.");
        }
      

    if (input.equalsIgnoreCase("AUTO")) {
        manualPI = false;
        Serial.println("Switched to automatic PI control.");
      }
    }

    if (target > endGame) {
      target = endGame;
    }

    if ((millis() - timer) >= 1000) {
      read_average_temperature();
      pid();
      monitor();
      timer = millis();
    }
    
    if (millis() - timerMonitor >= 1000) {
      Serial.print("End Game : ");
      Serial.print(endGame);
      Serial.print(" | Target Temp: ");
      Serial.print(target);
      Serial.print(" | Measured Temp: ");
      Serial.print(avgTemp);
      Serial.print(" | Error: ");
      Serial.print(error);
      Serial.print(" | PI Output: ");
      Serial.println(pi);
      timerMonitor = millis();
    }

   // if ((millis() - timer) >= 1000) {
     // pid();
    //}
    
    if (error >= 0 && zero_cross_detected) {
        delayMicroseconds(9700 - pi);

        for(int i=0; i<4; i++){
        digitalWrite(angleControl[i], HIGH);
        delayMicroseconds(70);
        digitalWrite(angleControl[i], LOW);
        }
        zero_cross_detected = false;
    }
     
    if ((millis() - timerIncrease) >= 1500) {
        increase();
        timerIncrease = millis();
    }
 
}