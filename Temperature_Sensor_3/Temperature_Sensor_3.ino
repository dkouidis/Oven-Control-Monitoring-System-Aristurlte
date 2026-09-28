#include <SPI.h>

#define DataOut 19
#define DataIn 23
#define SCLK 18
#define CS 32
unsigned char byte1;
unsigned char byte2;

// SPI settings: 0.5 MHz (max 10 MHz), SPI mode 1 (CPOL = 0, CPHA = 1)
SPISettings tmp126SPI(500000, MSBFIRST, SPI_MODE1);


void setup() {
  Serial.begin(115200);
  SPI.begin();

  pinMode(CS, OUTPUT);
  pinMode(DataOut, OUTPUT);
  pinMode(DataIn, INPUT);
  pinMode(SCLK, OUTPUT);

}

void loop() {
  SPI.beginTransaction(tmp126SPI);
    digitalWrite(CS, LOW);
    SPI.transfer(0x01);
    SPI.transfer(0x00);
    byte1 = SPI.transfer(0xFF);
    byte2 = SPI.transfer(0xFF);

    digitalWrite(CS, HIGH);
    SPI.endTransaction();

    float t126 = (((int16_t) byte1 << 8 | byte2) >> 2) * 0.03125;

    delay(1000);

    Serial.print("Sensor ");
    Serial.print(" : ");
    Serial.print(t126);
    Serial.println(" C");

}
