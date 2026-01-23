#include <Arduino.h>
#include <TinyWire.h>

// I2C slave address (change as needed)
#define I2C_SLAVE_ADDRESS 0x08

// LED pin - PB1 (physical pin 6)
// Note: PB0=SDA, PB2=SCL are used for I2C
#define LED_PIN 1

// Flash control variables
volatile uint8_t flashCount = 0;
volatile uint8_t flashSpeed = 100;  // delay in ms

// Called when master sends data
void receiveEvent(int numBytes) {
  if (numBytes >= 1 && TinyWire.available()) {
    flashCount = TinyWire.read();
  }
  if (numBytes >= 2 && TinyWire.available()) {
    flashSpeed = TinyWire.read();
  }
}

// Called when master requests data
void requestEvent() {
  // Send back current flash count (0 = done flashing)
  TinyWire.write(flashCount);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LED_PIN, HIGH);

  // Initialize I2C slave
  TinyWire.begin(I2C_SLAVE_ADDRESS);
  TinyWire.onReceive(receiveEvent);
  TinyWire.onRequest(requestEvent);
}

void loop() {
  // Flash LED if requested
  if (flashCount > 0) {
    digitalWrite(LED_PIN, HIGH);
    delay(flashSpeed);
    digitalWrite(LED_PIN, LOW);
    delay(flashSpeed);
    flashCount--;
  }
}
