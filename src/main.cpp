
#include <Arduino.h>

// Motor driver pins
const int STEP_PIN = 26;
const int DIR_PIN = 27;

// Control pins
const int START_BUTTON = 18;
const int STOP_BUTTON = 19;
const int DIRECTION_SWITCH = 21;
const int SPEED_POT = 34;
const int LIMIT_SWITCH = 23;

// Fault control pins
const int FAULT_BUTTON = 22;
const int RESET_BUTTON = 32;

// Motor states
bool motorRunning = false;
bool faultLatched = false;

// Previous button states
bool previousStart = HIGH;
bool previousReset = HIGH;
bool previousLimit = HIGH;

// Motor timing
unsigned long lastStepTime = 0;

void setup() {
    Serial.begin(115200);

    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);

    pinMode(START_BUTTON, INPUT_PULLUP);
    pinMode(STOP_BUTTON, INPUT_PULLUP);
    pinMode(LIMIT_SWITCH, INPUT_PULLUP);

    pinMode(FAULT_BUTTON, INPUT_PULLUP);
    pinMode(RESET_BUTTON, INPUT_PULLUP);

    pinMode(DIRECTION_SWITCH, INPUT);
    pinMode(SPEED_POT, INPUT);

    digitalWrite(STEP_PIN, LOW);

    Serial.println("INDUSTRIAL MOTOR CONTROLLER");
    Serial.println("System ready");
}

void loop() {

    // Read controls
    bool startPressed = digitalRead(START_BUTTON);
    bool stopPressed = digitalRead(STOP_BUTTON);
    bool limitTriggered = digitalRead(LIMIT_SWITCH);
    bool faultTriggered = digitalRead(FAULT_BUTTON);
    bool resetPressed = digitalRead(RESET_BUTTON);

    bool resetEdge =
        resetPressed == LOW && previousReset == HIGH;

    // 1. FAULT DETECTION
    if (faultTriggered == LOW) {

        if (!faultLatched) {
            Serial.println("MOTOR FAULT DETECTED");
        }

        faultLatched = true;
        motorRunning = false;
        digitalWrite(STEP_PIN, LOW);
    }

    // 2. AUTOMATIC LIMIT PROTECTION
    if (limitTriggered == LOW) {

        if (previousLimit == HIGH) {
            Serial.println("LIMIT SWITCH ACTIVATED");
        }

        motorRunning = false;
        digitalWrite(STEP_PIN, LOW);
    }

    // 3. MANUAL STOP
    if (stopPressed == LOW && motorRunning) {

        motorRunning = false;
        digitalWrite(STEP_PIN, LOW);

        Serial.println("MOTOR STOPPED");
    }

    // 4. FAULT RESET
    if (resetEdge && faultLatched) {

        if (faultTriggered == HIGH) {
            faultLatched = false;
            Serial.println("FAULT RESET SUCCESSFUL");
        }
        else {
            Serial.println("CANNOT RESET: FAULT ACTIVE");
        }
    }

    // 5. MOTOR START
    if (startPressed == LOW &&
        previousStart == HIGH &&
        !resetEdge &&
        !faultLatched &&
        faultTriggered == HIGH &&
        stopPressed == HIGH &&
        limitTriggered == HIGH) {

        motorRunning = true;
        lastStepTime = micros();

        Serial.println("MOTOR STARTED");
    }

    previousStart = startPressed;
    previousReset = resetPressed;
    previousLimit = limitTriggered;

    // 6. DIRECTION CONTROL
    bool clockwise = digitalRead(DIRECTION_SWITCH);
    digitalWrite(DIR_PIN, clockwise ? HIGH : LOW);

    // 7. SPEED CONTROL
    int potValue = analogRead(SPEED_POT);

    unsigned long stepInterval =
        map(potValue, 0, 4095, 20000, 2000);

    // 8. MOTOR STEP GENERATION
    if (motorRunning) {

        unsigned long now = micros();

        if (now - lastStepTime >= stepInterval) {

            lastStepTime = now;

            digitalWrite(STEP_PIN, HIGH);
            delayMicroseconds(5);
            digitalWrite(STEP_PIN, LOW);
        }
    }
}
