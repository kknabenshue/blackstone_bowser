#include <MsTimer2.h>

#define arr_length(x)  (sizeof(x)/sizeof(*x))   // Find number of elements in array x.

#define OPEN HIGH
#define CLOSED LOW

#define NUM_STALLS 4

unsigned int pin_sw[NUM_STALLS] = {             // Array of pin numbers for switches.
   6, 6, 6, 6                          // TODO: Replace with actual pin locations.
};

unsigned int pin_svo_pwm[NUM_STALLS] = {        // Array of pin numbers for servo PWM.
   9, 9, 9, 9                          // TODO: Replace with actual pin locations.
};

unsigned int sw[NUM_STALLS], pos[NUM_STALLS];   // Switch state and door position arrays.

volatile boolean timerServoExp;
const unsigned int posClosed = 1150;   // closed position
const unsigned int posOpen = 1920;     // open position


// Setup function.
void setup() {
   // setup logic variables
   for (int i = 0; i < NUM_STALLS; i++) {
      pos[i] = posClosed;  // TODO: Replace with memory read of last position.
   }
   
   // setup servos
   for (int i = 0; i < NUM_STALLS; i++) {
      pinMode(pin_svo_pwm[i], OUTPUT);
      digitalWrite(pin_svo_pwm[i], LOW);
      updateServo();                      // TODO: Replace with slow movement to correct memory position.
   }
   
   // setup switches
   for (int i = 0; i < NUM_STALLS; i++) {
      pinMode(pin_sw[i], INPUT_PULLUP);
   }
   
   // setup servo timer interrupt
   MsTimer2::set(3, isr_timerServo);   // period between updates in milliseconds
   MsTimer2::start();
}


// Main loop function.
void loop() {
   // Update switches
   for (int i = 0; i < NUM_STALLS; i++) {
      sw[i] = digitalRead(pin_sw[i]);
   }

   // event handlers
   if (timerServoExp) {
     updatePos();
   }
}


// Timer interrupt.
void isr_timerServo() {
   timerServoExp = true;
}


// Update position values.
void updatePos() {
   for (int i = 0; i < NUM_STALLS; i++) {
      // Move if switch is at OPEN, but door is not at OPEN.
      if (sw[i] == OPEN) {
         if (pos[i] != posOpen) {
            pos[i]++;
            updateServo();
         }
      }
      // Move if switch is at CLOSED, but door is not at CLOSED.
      else if (sw[i] == CLOSED) {
         if (pos[i] != posClosed) {
            pos[i]--;
            updateServo();
         }
      }
   }

   timerServoExp = false;
}


// Update servo postion.
void updateServo() {
   for (int i = 0; i < NUM_STALLS; i++) {
      long startTime = micros();
      digitalWrite(pin_svo_pwm[i], HIGH);
      while (micros() - startTime < pos[i]); // Pulse PWM high for position length.
      digitalWrite(pin_svo_pwm[i], LOW);
   }
}










