#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --------------------------------------------------
// Two ultrasonic sensors + I2C OLED (VDD 5V, GND, SCL=A5, SDA=A4)
// --------------------------------------------------
// Sensor 1 pins
const uint8_t TRIG1 = 11;
const uint8_t ECHO1 = 12;
// Sensor 2 pins
// NOTE: HC-SR04 sensors require 5V power. If using 3.3V:
// - Sensor may not work reliably
// - ECHO pin outputs 5V which can damage 3.3V Arduino pins
// - Use voltage divider (2x 10kΩ resistors) on ECHO pin if using 3.3V
const uint8_t TRIG2 = 9;
const uint8_t ECHO2 = 10;

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Threshold: distance below this (cm) = occupied
const int OCCUPIED_THRESHOLD_CM = 14;

int distance1Cm = 0;
int distance2Cm = 0;
bool slot1Free = false;
bool slot2Free = false;

// --------------------------------------------------
int readDistanceCm(uint8_t trigPin, uint8_t echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // timeout 30ms
  if (duration == 0) return 0;                  // treat timeout as 0 (error)
  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(9600);

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  Wire.begin(); // uses SDA=A4, SCL=A5
  delay(50);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // common I2C OLED address
    Serial.println(F("OLED not found at 0x3C/0x3D. Check wiring."));
  } else {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Smart");
    display.println("Parking");
    display.display();
    delay(1000);
  }
}

void loop() {
  distance1Cm = readDistanceCm(TRIG1, ECHO1);
  distance2Cm = readDistanceCm(TRIG2, ECHO2);

  slot1Free = distance1Cm >= OCCUPIED_THRESHOLD_CM && distance1Cm != 0;
  slot2Free = distance2Cm >= OCCUPIED_THRESHOLD_CM && distance2Cm != 0;

  uint8_t freeCount = 0;
  if (slot1Free) freeCount++;
  if (slot2Free) freeCount++;

  // Serial diagnostics (legacy format for debugging)
  Serial.print("S1: ");
  Serial.print(distance1Cm);
  Serial.print(" cm ");
  Serial.print(slot1Free ? "FREE" : "OCC");
  Serial.print(" | S2: ");
  Serial.print(distance2Cm);
  Serial.print(" cm ");
  Serial.print(slot2Free ? "FREE" : "OCC");
  Serial.print(" | Free: ");
  Serial.print(freeCount);
  
  // Debug Sensor 2 if not working
  if (distance2Cm == 0) {
    Serial.print(" [S2 ERROR]");
  }
  Serial.println();
  
  // JSON format for web dashboard
  Serial.print("JSON:");
  Serial.print("{");
  Serial.print("\"slot1\":{");
  Serial.print("\"distance\":");
  Serial.print(distance1Cm);
  Serial.print(",\"status\":\"");
  Serial.print(slot1Free ? "FREE" : "OCCUPIED");
  Serial.print("\"},");
  Serial.print("\"slot2\":{");
  Serial.print("\"distance\":");
  Serial.print(distance2Cm);
  Serial.print(",\"status\":\"");
  Serial.print(slot2Free ? "FREE" : "OCCUPIED");
  Serial.print("\"},");
  Serial.print("\"freeCount\":");
  Serial.print(freeCount);
  Serial.print(",\"totalSlots\":2");
  Serial.print(",\"overallStatus\":\"");
  if (freeCount == 0) {
    Serial.print("FULL");
  } else if (freeCount == 2) {
    Serial.print("FREE");
  } else {
    Serial.print("PARTIAL");
  }
  Serial.print("\"}");
  Serial.println();

  // Dashboard Display
  display.clearDisplay();
  
  // Header - Overall Status
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  if (freeCount == 0) {
    display.println("FULL");
  } else if (freeCount == 2) {
    display.println("FREE");
  } else {
    display.println("PARTIAL");
  }
  
  // Separator line
  display.drawLine(0, 18, 128, 18, WHITE);
  
  // Slot 1 Status
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 22);
  display.print("Slot 1: ");
  display.println(slot1Free ? "FREE" : "OCCUPIED");
  
  // Slot 2 Status
  display.setCursor(0, 34);
  display.print("Slot 2: ");
  display.println(slot2Free ? "FREE" : "OCCUPIED");
  
  // Footer - Available count
  display.setCursor(0, 48);
  display.print("Available: ");
  display.print(freeCount);
  display.print("/2");
  
  // Visual indicators (boxes)
  // Slot 1 indicator box
  display.drawRect(90, 22, 20, 10, WHITE);
  if (slot1Free) {
    display.fillRect(92, 24, 16, 6, WHITE); // Filled = free
  }
  
  // Slot 2 indicator box
  display.drawRect(90, 34, 20, 10, WHITE);
  if (slot2Free) {
    display.fillRect(92, 36, 16, 6, WHITE); // Filled = free
  }
  
  display.display();

  delay(500); // adjust if you want faster/slower updates
}
