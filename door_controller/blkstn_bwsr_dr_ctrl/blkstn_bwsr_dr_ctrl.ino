#include <MsTimer2.h>
#include <EEPROM.h>
#include <TimerOne.h>

#define arr_length(x)  (sizeof(x)/sizeof(*x))   // Find number of elements in array x.

#define OPEN HIGH
#define CLOSED LOW
#define END true
#define MOVE_NONE 0
#define MOVE_OPEN 1
#define MOVE_CLOSED 2

#define NUM_STALLS 4
#define MOVE_TIME_MS 4000              // Move time in milliseconds.

#define DEBUG_MODE 1

// #define POS_OPEN 1150                     // Open position.
// #define POS_CLOSED 1920                   // Closed position.
#define POS_OPEN 1150                     // Open position.
#define POS_CLOSED 1800                   // Closed position.
// #define POS_OPEN 1400                     // Open position.
// #define POS_CLOSED 1700                   // Closed position.
#define POS_OVERSHOOT 20                  // Amount to overshoot endpoints.

char bufDebug[32];                        // Debug character buffer.

byte pin_sw[NUM_STALLS] = {               // Array of pin numbers for switches.
   9, 8, 6, 7
};

byte pin_svo_pwm[NUM_STALLS] = {          // Array of pin numbers for servo PWM.
   13, 12, 11, 10
};

byte pin_pwr_sense = 5;                   // Power sense pin. TODO: Replace with actual pin location.

byte sw[NUM_STALLS];                      // Switch state array.
byte dir[NUM_STALLS];                     // Direction of movement array.
unsigned int pos[NUM_STALLS];             // Door position array.


bool enUpServo[NUM_STALLS];               // Array of enables for the update function.

unsigned int cntEnUpServo = 0;            // Counter for number of enabled servos for update.
unsigned long cntTimerServoExp = 0;       // Counter for number of servo timer expirations.
volatile boolean timerServoExp;           // Flag to signal servo timer expiration.


// Setup function.
void setup() {
   // // Setup logic variables: w/o EEPROM.
   // for (int i = 0; i < NUM_STALLS; i++) {
      // pos[i] = POS_CLOSED;  // TODO: Replace with memory read of last position.
      // dir[i] = OPEN;
   // }
   
   
  // Setup logic variables: w/ EEPROM.
  int eeAddr = 0;
  
  EEPROM.get(eeAddr,pos);    // Get entire position array.
  
  // Get postition array one element at a time.
  // for (int i = 0; i < NUM_STALLS; i++) {
     // EEPROM.get(eeAddr,pos[i]);
     // eeAddr += sizeof(pos[i]);
  // }
   
   
   // Setup servos.
   for (int i = 0; i < NUM_STALLS; i++) {
      pinMode(pin_svo_pwm[i], OUTPUT);
      digitalWrite(pin_svo_pwm[i], LOW);
      enUpServo[i] = true;
   }
   
   updateServo();                      // TODO: Replace with slow movement to correct memory position.
   
   
   // Setup switches.
   for (int i = 0; i < NUM_STALLS; i++) {
      pinMode(pin_sw[i], INPUT_PULLUP);
   }
   
   
   // Setup servo timer interrupt.
   MsTimer2::set(1, isr_timerServo);   // period between updates in milliseconds.
   MsTimer2::start();
   // Timer1.initialize(1);                     // Initialize timer1 period in microseconds.
   // Timer1.attachInterrupt(isr_timerServo);   // Attach ISR to timer1.
   
   
   if (DEBUG_MODE) {
      //Initialize serial.
      Serial.begin(9600);
      sprintf(bufDebug, "Serial Debug");
      Serial.println(bufDebug);
      sprintf(bufDebug, "------------");
      Serial.println(bufDebug);
   }
}


// Main loop function.
void loop() {
   // Update switches.
   for (int i = 0; i < NUM_STALLS; i++) {
      sw[i] = digitalRead(pin_sw[i]);
   }
   
   
   // Event handlers.
   if (timerServoExp) {
      if (cntEnUpServo == 0) {
         cntTimerServoExp = 0;
         cntEnUpServo = 0;
         updatePos();
      }
      else {
         if (cntTimerServoExp == 100) {
            cntTimerServoExp = 0;
            cntEnUpServo = 0;
            updatePos();
            
            // sprintf(bufDebug, "cntTimerServoExpMax = %i\n", cntTimerServoExpMax);
            // Serial.println(bufDebug);
         }
         else {
            cntTimerServoExp++;
         }
      }
   }
   
   // if (timerServoExp) {
      // if (cntEnUpServo == 0) {
         // cntTimerServoExp = 0;
         // cntEnUpServo = 0;
         // updatePos();
      // }
      // else {
         // long POS_RANGE = POS_OPEN - POS_CLOSED;
         // POS_RANGE = abs(POS_RANGE);
         
         // unsigned long cntTimerServoExpMax = MOVE_TIME_MS / (unsigned long)cntEnUpServo / POS_RANGE;
         
         // if (cntTimerServoExpMax < 1) {   // Servo timer counter max range check.
            // cntTimerServoExpMax = 1;
         // }
         
         // if (cntTimerServoExp == cntTimerServoExpMax) {
            // cntTimerServoExp = 0;
            // cntEnUpServo = 0;
            // updatePos();
            
            // // sprintf(bufDebug, "cntTimerServoExpMax = %i\n", cntTimerServoExpMax);
            // // Serial.println(bufDebug);
         // }
         // else {
            // cntTimerServoExp++;
         // }
      // }
   // }
   
   
   // Check if power down is occuring by reading power sense.
   if (digitalRead(pin_pwr_sense) == LOW) {
      powerDown();
   }
}


// Timer interrupt.
void isr_timerServo() {
   timerServoExp = true;
}


// Update position values.
void updatePos() {
   byte move[NUM_STALLS];
   
   for (int i = 0; i < NUM_STALLS; i++) {
      move[i] = MOVE_NONE;
      
      if (sw[i] == OPEN) {
         if (pos[i] > POS_OPEN) {
            move[i] = MOVE_OPEN;
         }
         else if (pos[i] == POS_OPEN) {
            if (dir[i] == OPEN) {
               move[i] = MOVE_OPEN;
            }
         }
         else if (pos[i] < POS_OPEN) {       // Passed open.
            if (pos[i] == POS_OPEN - POS_OVERSHOOT) {      // End of range.
               dir[i] = CLOSED;
               move[i] = MOVE_CLOSED;
            }
            else if (dir[i] == OPEN) {
               move[i] = MOVE_OPEN;
            }
            else if (dir[i] == CLOSED) {
               move[i] = MOVE_CLOSED;
            }
         }
      }
      else if (sw[i] == CLOSED) {
         if (pos[i] < POS_CLOSED) {
            move[i] = MOVE_CLOSED;
         }
         else if (pos[i] == POS_CLOSED) {
            if (dir[i] == CLOSED) {
               move[i] = MOVE_CLOSED;
            }
         }
         else if (pos[i] > POS_CLOSED) {     // Passed closed.
            if (pos[i] == POS_CLOSED + POS_OVERSHOOT) {    // End of range.
               dir[i] = OPEN;
               move[i] = MOVE_OPEN;
            }
            else if (dir[i] == OPEN) {
               move[i] = MOVE_OPEN;
            }
            else if (dir[i] == CLOSED) {
               move[i] = MOVE_CLOSED;
            }
         }
      }
      
      if (move[i] == MOVE_OPEN) {
         pos[i]--;
         enUpServo[i] = true;
         cntEnUpServo++;
      }
      else if (move[i] == MOVE_CLOSED) {
         pos[i]++;
         enUpServo[i] = true;
         cntEnUpServo++;
      }
   }
   
   updateServo();
   timerServoExp = false;
}

// void updatePos() {
   // for (int i = 0; i < NUM_STALLS; i++) {
      // // Move if switch is at OPEN, but door is not at OPEN.
      // if (sw[i] == OPEN) {
         // if (pos[i] != POS_OPEN) {
            // pos[i]--;
            // enUpServo[i] = true;
            // cntEnUpServo++;
         // }
      // }
      // // Move if switch is at CLOSED, but door is not at CLOSED.
      // else if (sw[i] == CLOSED) {
         // if (pos[i] != POS_CLOSED) {
            // pos[i]++;
            // enUpServo[i] = true;
            // cntEnUpServo++;
         // }
      // }
   // }
   
   // updateServo();
   // timerServoExp = false;
// }


// Update servo postion.
void updateServo() {
   for (int i = 0; i < NUM_STALLS; i++) {
      if (enUpServo[i]) {
         long startTime = micros();
         digitalWrite(pin_svo_pwm[i], HIGH);
         while (micros() - startTime < pos[i]); // Pulse PWM high for position length.
         digitalWrite(pin_svo_pwm[i], LOW);
         enUpServo[i] = false;                  // Clear enable.
      }
   }
}


// Power down procedure.
void powerDown() {
   while (true) {
      noInterrupts();   // Disable interrupts.
      
      // Write current servo positions to nonvolatile memory.
      int eeAddr = 0;
      
      EEPROM.put(eeAddr,pos);    // Put entire position array.
      
      // Put position array one element at a time.
      // for (int i = 0; i < NUM_STALLS; i++) {
         // EEPROM.put(eeAddr,pos[i]);
         // eeAddr += sizeof(pos[i]);
      // }
      
      // Wait forever...
   }
}







