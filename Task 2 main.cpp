#include "mbed.h"
#include "arm_book_lib.h"

// Pin Definitions
DigitalIn gasSensor(D2);           // Gas detector 
DigitalIn tempSensor(D3);          // Temperature detector 
DigitalIn keypad[4] = {DigitalIn(D4), DigitalIn(D5), DigitalIn(D6), DigitalIn(D7)}; // 
DigitalIn submitButton(BUTTON1);   // Submit button (B1)

DigitalOut alarmLed(LED1);         // Alarm LED 
DigitalOut lockLed(LED2);          // Lockout indicator LED

// System States
enum SystemState { IDLE, ALARM, EMERGENCY, LOCKED };
SystemState currentState = IDLE;

// Security Code Settings
const int correctCode[4] = {1, 2, 3, 4};  //  code: 1234
int enteredCode[4] = {0};
int codePosition = 0;
int failedAttempts = 0;

// Timers
Timer lockTimer;                   // Tracks 60-second lockout
Timer emergencyTimer;              // Controls rapid alarm flashing
Timer lockLedTimer;                // Controls lockout LED blinking

// Function Prototypes
void resetSystem();
void checkSensors();
void handleEmergency();
void handleLocked();
void readKeypad();

int main() {
    // Configure inputs with pull-down resistors
    gasSensor.mode(PullDown);
    tempSensor.mode(PullDown);
    for (int i = 0; i < 4; i++) keypad[i].mode(PullDown);
    submitButton.mode(PullDown);

    // Initialize LEDs
    alarmLed = OFF;
    lockLed = OFF;

    while (true) {
        checkSensors(); // Monitor gas/temperature sensors

        switch (currentState) {
            case IDLE:
                break;
            case ALARM:
                alarmLed = ON; // Solid LED for non-emergency alarm
                break;
            case EMERGENCY:
                handleEmergency(); // Rapid flashing and code entry
                break;
            case LOCKED:
                handleLocked(); // Lockout with slow blinking
                break;
        }
    }
}

// Reset all states and timers
void resetSystem() {
    currentState = IDLE;
    alarmLed = OFF;
    lockLed = OFF;
    codePosition = 0;
    failedAttempts = 0;
    emergencyTimer.stop();
    lockTimer.stop();
    lockLedTimer.stop();
}

// Check gas/temperature sensors and update state
void checkSensors() {
    bool gas = gasSensor;
    bool temp = tempSensor;

    if (currentState == IDLE || currentState == ALARM) {
        if (gas && temp) {
            currentState = EMERGENCY;
            emergencyTimer.start();
        } else if (gas || temp) {
            currentState = ALARM;
        }
    }
}

// Handle emergency mode (rapid flashing + code entry)
void handleEmergency() {
    // Rapid alarm flashing (200ms cycle)
    if (emergencyTimer.read_ms() % 200 < 100) alarmLed = ON;
    else alarmLed = OFF;

    readKeypad(); // Check for code input

    // Submit code on button press
    if (submitButton) {
        bool correct = true;
        for (int i = 0; i < 4; i++) {
            if (enteredCode[i] != correctCode[i]) correct = false;
        }

        if (correct) {
            resetSystem();
        } else {
            failedAttempts++;
            if (failedAttempts >= 5) {
                currentState = LOCKED;
                lockTimer.start();
                lockLedTimer.start();
                failedAttempts = 0;
            }
        }
        codePosition = 0; // Reset code entry
        ThisThread::sleep_for(200ms); // Debounce
    }
}

// Handle lockout state (60-second timer + LED blinking)
void handleLocked() {
    if (lockTimer.read() >= 60) {
        resetSystem();
    } else {
        // Slow LED blinking (1Hz)
        if (lockLedTimer.read_ms() % 1000 < 500) lockLed = ON;
        else lockLed = OFF;
    }
}

// Read keypad inputs and populate enteredCode[]
void readKeypad() {
    for (int i = 0; i < 4; i++) {
        if (keypad[i] && codePosition < 4) {
            enteredCode[codePosition] = i + 1; // Map D4-D7 to digits 1-4
            codePosition++;
            ThisThread::sleep_for(200ms); // Debounce
        }
    }
}
