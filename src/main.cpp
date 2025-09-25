#include <Arduino.h>

void setup() {
  Serial.begin(9600);   // Start the serial communication
  Serial.println("hi"); // Print "hi" once at startup
}

void loop() {
  // Nothing here; runs repeatedly
}
