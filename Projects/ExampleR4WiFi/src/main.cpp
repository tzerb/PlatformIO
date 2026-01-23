// /*
//   Blink

//   Turns an LED on for one second, then off for one second, repeatedly.

//   Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
//   it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
//   the correct LED pin independent of which board is used.
//   If you want to know what pin the on-board LED is connected to on your Arduino
//   model, check the Technical Specs of your board at:
//   https://www.arduino.cc/en/Main/Products

//   modified 8 May 2014
//   by Scott Fitzgerald
//   modified 2 Sep 2016
//   by Arturo Guadalupi
//   modified 8 Sep 2016
//   by Colby Newman

//   This example code is in the public domain.

//   https://www.arduino.cc/en/Tutorial/BuiltInExamples/Blink
// */
#include <Arduino.h>
// testing a stepper motor with a Pololu A4988 driver board or equivalent
// on an Uno the onboard led will flash with each step
// this version uses delay() to manage timing
#include "Arduino_LED_Matrix.h"
#include "FspTimer.h"

FspTimer audio_timer;
volatile uint64_t count=0;
uint64_t start_time=0;

// callback method used by timer
void timer_callback(timer_callback_args_t __attribute((unused)) *p_args) {
  count++;
}

bool beginTimer(float rate) {
  uint8_t timer_type = GPT_TIMER;
  int8_t tindex = FspTimer::get_available_timer(timer_type);
  if (tindex < 0){
    tindex = FspTimer::get_available_timer(timer_type, true);
  }
  if (tindex < 0){
    return false;
  }

  FspTimer::force_use_of_pwm_reserved_timer();

  if(!audio_timer.begin(TIMER_MODE_PERIODIC, timer_type, tindex, rate, 0.0f, timer_callback)){
    return false;
  }

  if (!audio_timer.setup_overflow_irq()){
    return false;
  }

  if (!audio_timer.open()){
    return false;
  }

  if (!audio_timer.start()){
    return false;
  }
  return true;
}

byte enablePinNeg = 11;
byte enablePin = 10;
byte directionPin = 9;
byte stepPin = 8;
byte potPin = A0;
byte switchPin = A5;
int numberOfSteps = 200;
byte ledPin = 13;
int pulseWidthMicros = 1;  // microseconds
int millisbetweenSteps = 1; // milliseconds - or try 1000 for slower steps

ArduinoLEDMatrix _matrix;

class Indicators {
  byte frame[8][12];
  public:
    Indicators() {
    }

    void clear() {
      for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 12; j++) {
          frame[i][j] = 0;
        }
      }
      _matrix.renderBitmap(frame, 8, 12);
    }

    void clear(int i) {
        for(int j = 0; j < 12; j++) {
          frame[i][j] = 0;
        }
      _matrix.renderBitmap(frame, 8, 12);
    }

    void all() {
      for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 12; j++) {
          frame[i][j] = 1;
        }
      }
      _matrix.renderBitmap(frame, 8, 12);
    }

    void all(int i) {
        for(int j = 0; j < 12; j++) {
          frame[i][j] = 1;
        }
      _matrix.renderBitmap(frame, 8, 12);
    }

    void indicator(int i, int v) {
      clear(i);
      frame[i][v] = 1;
      _matrix.renderBitmap(frame, 8, 12);
    }
};

Indicators indicators;// = Indicators();


void setup() { 
  beginTimer(8000);
  start_time = millis();

  Serial.begin(115200);
  analogReadResolution(12);
  _matrix.begin();
  indicators.all();

  Serial.println("Starting StepperTest");
  digitalWrite(ledPin, LOW);

  delay(1000);
  indicators.clear();
  indicators.indicator(0, 5);
  indicators.indicator(0, 6);

  pinMode(directionPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(enablePinNeg, OUTPUT);
  pinMode(enablePin, OUTPUT);
  
  digitalWrite(enablePinNeg, LOW);
  digitalWrite(enablePin, HIGH);
  digitalWrite(directionPin, HIGH);
}

int64_t lastSwitchVal = 0;
int n = 0;

void readPot() {
  int64_t potVal = analogRead(potPin);
  int64_t switchVal = analogRead(switchPin);

  if (lastSwitchVal > (int)(switchVal*12.0F/4250.0))
  {
    pulseWidthMicros++;
  }
  lastSwitchVal = (int)(switchVal*12.0F/4250.0);
  millisbetweenSteps = potVal;

  indicators.indicator(0, n++ % 12);
  //indicators.indicator(1, potVal % 12);
  indicators.indicator(2, switchVal / 200 % 12);
  //indicators.indicator(3, millisbetweenSteps % 100 % 12);
  indicators.indicator(4, millisbetweenSteps / 100 % 12 );
  indicators.indicator(5, pulseWidthMicros % 12);
  indicators.indicator(6, (int)(potVal*12.0F/4250.0) % 12);
  indicators.indicator(7, (int)(switchVal*12.0F/4250.0) % 12);
  //indicators.indicator(7, count%12);
}

void loop() { 

  readPot();

  digitalWrite(stepPin, HIGH);
  delayMicroseconds(pulseWidthMicros);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(millisbetweenSteps);

  // digitalWrite(directionPin, LOW);
  // for(int n = 0; n < numberOfSteps; n++) {
  //   indicators.indicator(6, potVal());

  //   indicators.indicator(0, 11-n*12/numberOfSteps);
  //   digitalWrite(stepPin, HIGH);
  //   // delayMicroseconds(pulseWidthMicros); // probably not needed
  //   digitalWrite(stepPin, LOW);
    
  //   delay(millisbetweenSteps);
    
  // }  
}
