#include <MsTimer2.h>
#include <EEPROM.h>

#define EN_DEBUG_MODE false               // Enable debug mode.
#define EN_INIT_MODE false                // Enable init mode.
#define EN_EEPROM true                    // Enable EEPROM logic.

#define arr_length(x)  (sizeof(x)/sizeof(*x))   // Find number of elements in array x.

#define OPEN LOW
#define CLOSED HIGH
#define MOVE_NONE 0
#define MOVE_OPEN 1
#define MOVE_CLOSED 2

#define NUM_STALLS 4                      // Number of stalls.
#define MOVE_TIME_MS 4000                 // Move time in milliseconds.

#define POS_CLOSED 600                    // Open position.
#define POS_OPEN 2400                     // Closed position.
#define POS_STEP 10                       // Step size.

char bufDebug[32];                        // Debug character buffer.

byte pin_sw[NUM_STALLS] = {               // Array of pin numbers for switches.
   7, 6, 8, 9
};

byte pin_svo_pwm[NUM_STALLS] = {          // Array of pin numbers for servo PWM.
   13, 12, 11, 10
};

int pin_pwr_sense = A0;                   // Power sense pin.

byte pin_servo_fet = 4;                   // Servo FET pin.

byte sw[NUM_STALLS];                      // Switch state array.
unsigned int pos[NUM_STALLS];             // Door position array.


bool enUpServo[NUM_STALLS];               // Array of enables for the update function.

unsigned int cntEnUpServo = 0;            // Counter for number of enabled servos for update.
unsigned long cntTimerServoExp = 0;       // Counter for number of servo timer expirations.
volatile boolean timerServoExp;           // Flag to signal servo timer expiration.


// Setup function.
void setup() {
   // Setup logic variables: w/o EEPROM.
   if (!EN_EEPROM || EN_INIT_MODE) {
      for (int i = 0; i < NUM_STALLS; i++) {
         pos[i] = POS_OPEN;
      }
   }
   
   
   // Setup logic variables: w/ EEPROM.
   if (EN_EEPROM && !EN_INIT_MODE) {
      int eeAddr = 0;
      EEPROM.get(eeAddr,pos);    // Get entire position array.
   }
   
   
   // Setup servos.
   pinMode(pin_servo_fet, OUTPUT);
   digitalWrite(pin_servo_fet, LOW);
   
   for (int i = 0; i < NUM_STALLS; i++) {
      pinMode(pin_svo_pwm[i], OUTPUT);
      digitalWrite(pin_svo_pwm[i], LOW);
      enUpServo[i] = true;
   }
   
   updateServo();
   
   
   // Setup switches.
   for (int i = 0; i < NUM_STALLS; i++) {
      pinMode(pin_sw[i], INPUT_PULLUP);
   }
   
   
   // Setup power sense.
   pinMode(pin_pwr_sense, INPUT);
   
   
   // Setup servo timer interrupt.
   MsTimer2::set(20, isr_timerServo);   // Period between updates in milliseconds.
   MsTimer2::start();
   
   
   //Initialize serial.
   if (EN_DEBUG_MODE) {
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
   // if (timerServoExp) {
      // if (cntEnUpServo == 0) {
         // cntTimerServoExp = 0;
         // cntEnUpServo = 0;
         // updatePos();
      // }
      // else {
         // if (cntTimerServoExp == 100) {
            // cntTimerServoExp = 0;
            // cntEnUpServo = 0;
            // updatePos();
         // }
         // else {
            // cntTimerServoExp++;
         // }
      // }
   // }
   
   if (timerServoExp) {
      cntEnUpServo = 0;
      updatePos();
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
   if (EN_EEPROM) {
      if (analogRead(pin_pwr_sense) < 860) {       // round(4.2 V / 5.0 V * 1023) = 860
         powerDown();
      }
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
      
      // Move if switch is at OPEN, but door is not at OPEN.
      if (sw[i] == OPEN) {
         if (pos[i] != POS_OPEN) {
            move[i] = MOVE_OPEN;
         }
      }
      // Move if switch is at CLOSED, but door is not at CLOSED.
      else if (sw[i] == CLOSED) {
         if (pos[i] != POS_CLOSED) {
            move[i] = MOVE_CLOSED;
         }
      }
      
      if (move[i] == MOVE_OPEN) {
         pos[i] = pos[i] + POS_STEP;
         enUpServo[i] = true;
         cntEnUpServo++;
      }
      else if (move[i] == MOVE_CLOSED) {
         pos[i] = pos[i] - POS_STEP;
         enUpServo[i] = true;
         cntEnUpServo++;
      }
      
      if (pos[i] > POS_OPEN) {
         pos[i] = POS_OPEN;
      }
      else if (pos[i] < POS_CLOSED) {
         pos[i] = POS_CLOSED;
      }
   }
   
   updateServo();
   timerServoExp = false;
}


// Update servo postion.
void updateServo() {
   for (int i = 0; i < NUM_STALLS; i++) {
      if (enUpServo[i]) {
         digitalWrite(pin_svo_pwm[i], HIGH);
         delayMicroseconds(pos[i]);
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
      
      if (EN_DEBUG_MODE) {
         sprintf(bufDebug, "Power Down...Bye Bye...");
         Serial.println(bufDebug);
      }
      
      // Wait forever...
   }
}







