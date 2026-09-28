#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "TMP126.h"

// === TMP126 SPI Configuration ===
#define SCLK  18
#define CS    32
#define SI    23
#define SO    19

TMP126 myTMP(CS, SI, SO, SCLK);

// === TRIAC & Control Pins ===
const int zcd = 27;                           // Zero cross detection pin 
const int angleControl[] = {25, 26, 17, 16};  // Trigger pins for TRIAC

// === Other Inputs ===
const int POTENTIOMETER = 34;

// === LCD Display ===
LiquidCrystal_I2C lcd(0x27, 20, 4);

// === PI Variables ===
float error = 0;
float previous_error = 0;
float totalError = 0;

float kp = 350;
float ki = 6;

float proportional = 0;
float integral = 0;
float pi = 0;

volatile bool zero_cross_detected = false;

int target = 10;
int endGame = 60; // Target peak temperature --> Change this to the desired temperature

bool manualPI = false;

unsigned long timer = 0;
unsigned long timerIncrease = 0;
unsigned long timerMonitor = 0;

float avgTemp = 0; // Holds last temperature reading

// === Read temperature from TMP126 ===
float read_average_temperature() {
    float temperatureC = myTMP.getTemp();
    avgTemp = temperatureC;
    return temperatureC;
}

// === Monitor LCD ===
void monitor() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Goal ");
    lcd.print(endGame);
    lcd.print(".00 ");

    lcd.setCursor(11, 0);
    lcd.print("T:");
    lcd.print(target);
    lcd.print("  ");

    lcd.setCursor(0, 1);
    lcd.print("Temp ");
    lcd.print(read_average_temperature(), 1);

    lcd.setCursor(11, 1);
    lcd.print("E:");
    lcd.setCursor(13, 1);
    lcd.print((int)error);
    lcd.print("  ");
}

// === PI Control ===
void pid() {
    previous_error = error;
    error = target - (avgTemp);

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
    pi = constrain(pi, 330, 5500);
}

// === Increase Target Temperature Gradually ===
void increase() {
    if (round((endGame - target)) >= 1) {
        target += 1;
    }
}

// === Zero Cross Detection Interrupt ===
void IRAM_ATTR zero_cross_detection() {
    zero_cross_detected = true;
}

// === Setup ===
void setup() {
    pinMode(zcd, INPUT);
    pinMode(POTENTIOMETER, INPUT);

    for (int i = 0; i < 4; i++) {
        pinMode(angleControl[i], OUTPUT);
    }

    attachInterrupt(digitalPinToInterrupt(zcd), zero_cross_detection, FALLING);

    Serial.begin(115200);
    lcd.init();
    lcd.backlight();
    lcd.home();

    myTMP.begin(); // Initialize TMP126

    timer = millis();
    timerMonitor = millis();
    timerIncrease = millis();
}

// === Main Loop ===
void loop() {

    //int potValue = analogRead(POTENTIOMETER);               // Read potentiometer
    //endGame = map(potValue, 0, 4095, 20, 100);              // Map to temp range
    //endGame = constrain(endGame, 20, 100);                  // Constrain just in case


    // Handle serial commands for manual PI override
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        float newPI = input.substring(3).toFloat();
        newPI = constrain(newPI, 330, 5500);

        if (newPI >= 0) {
            pi = newPI;
            manualPI = true;
            Serial.print("Manual PI set to: ");
            Serial.println(pi);
        } else if (input.equalsIgnoreCase("AUTO")) {
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
        Serial.print("End Game: ");
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

    if (error >= 0 && zero_cross_detected) {
        delayMicroseconds(9700 - pi);

        for (int i = 0; i < 4; i++) {
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
