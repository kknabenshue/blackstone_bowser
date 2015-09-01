
#include <MsTimer2.h>

#define PIN_SW_01 6
#define PIN_SERVO_FET 25
#define PIN_SERVO_PWM 9
#define OPEN HIGH
#define CLOSED LOW

volatile boolean timerServoExp;
unsigned int sw01, pos;
const unsigned int posClosed = 1150;    // closed position
const unsigned int posOpen = 1920;      // open position

void setup() {// setup logic variables
    pos = posClosed;  // TODO: Replace with memory read of last position.
    
    // setup servo
    pinMode(PIN_SERVO_PWM, OUTPUT);
    digitalWrite(PIN_SERVO_PWM, LOW);
    updateServo();    // TODO: Replace with slow movement to correct memory position.
    
    // setup switches
    pinMode(PIN_SW_01, INPUT_PULLUP);
    
    // setup servo timer interrupt
    MsTimer2::set(3, isr_timerServo);   // period between updates in milliseconds
    MsTimer2::start();
}

void loop() {
    // Update switches
    sw01 = digitalRead(PIN_SW_01);
    
    // event handlers
    if (timerServoExp) {
        updatePos();
    }
}

void isr_timerServo() {
    timerServoExp = true;
}

void updatePos() {
    if (sw01 == OPEN) {
      if (pos != posOpen) {
        pos++;
        updateServo();
      }
    }
    else if (sw01 == CLOSED) {
      if (pos != posClosed) {
        pos--;
        updateServo();
      }
    }
    
    timerServoExp = false;
}

void updateServo() {
    long startTime = micros();
    digitalWrite(PIN_SERVO_PWM, HIGH);
    while (micros() - startTime < pos);
    digitalWrite(PIN_SERVO_PWM, LOW);
}










