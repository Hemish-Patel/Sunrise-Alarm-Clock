#include <arduino.h>

const int ledPin = 33;

void setup() {
  // Initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Turn the LED on
  digitalWrite(ledPin, HIGH);
  
  delay(250); // Wait for 0.25 seconds

  // Turn the LED off
  digitalWrite(ledPin, LOW);
  delay(250); // Wait for 0.5 seconds
}