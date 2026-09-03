/*
 * Controls:
 * - Motor 1: Duckweed collection movement
 * - Motor 2: Lifting mechanism
 * - LED status indicator
 *
 * Coverage data is received from the ESP32-CAM.
 */

#include <SoftwareSerial.h>

// ESP32 serial communication
#define rxPin 0
#define txPin 1

SoftwareSerial espSerial(rxPin, txPin);

// Stepper Motor 1 - Net Movement
#define STEP_MOTOR1 2
#define DIR_MOTOR1 3

// Stepper Motor 2 - Lifting Mechanism
#define STEP_MOTOR2 4
#define DIR_MOTOR2 5

// Status LED
#define LED_PIN 13

// Duckweed coverage required before harvesting
const int threshold = 80;

// Start with invalid coverage to prevent accidental harvesting
volatile int coverage = -1;

// Prevent multiple harvesting cycles from running simultaneously
volatile bool isHarvesting = false;

// Make sure valid ESP32 data has been received first
bool systemInitialized = false;


void setup() {

    // Motor control pins
    pinMode(STEP_MOTOR1, OUTPUT);
    pinMode(DIR_MOTOR1, OUTPUT);

    pinMode(STEP_MOTOR2, OUTPUT);
    pinMode(DIR_MOTOR2, OUTPUT);

    pinMode(LED_PIN, OUTPUT);

    // Initialize outputs
    digitalWrite(STEP_MOTOR1, LOW);
    digitalWrite(DIR_MOTOR1, LOW);

    digitalWrite(STEP_MOTOR2, LOW);
    digitalWrite(DIR_MOTOR2, LOW);

    digitalWrite(LED_PIN, LOW);

    // Communication with ESP32
    espSerial.begin(9600);

    // Serial monitor
    Serial.begin(9600);

    // Startup indicator
    for (int i = 0; i < 3; i++) {

        digitalWrite(LED_PIN, HIGH);
        delay(500);

        digitalWrite(LED_PIN, LOW);
        delay(500);
    }

    Serial.println("Waiting for valid data from ESP32...");
}


void loop() {

    static unsigned long lastCheckTime = 0;

    // Check approximately every second
    if (millis() - lastCheckTime >= 1000) {

        lastCheckTime = millis();

        // Check for coverage sent by ESP32
        if (espSerial.available()) {

            String input = espSerial.readStringUntil('\n');

            input.trim();

            if (input.length() > 0) {

                int newCoverage = input.toInt();

                // Validate coverage
                if (newCoverage >= 0 && newCoverage <= 100) {

                    coverage = newCoverage;

                    // Blink LED to indicate data received
                    digitalWrite(LED_PIN, HIGH);
                    delay(100);
                    digitalWrite(LED_PIN, LOW);

                    Serial.print("Received Coverage: ");
                    Serial.println(coverage);

                    systemInitialized = true;

                } else {

                    Serial.println("Invalid coverage data received.");
                }
            }
        }


        // Begin harvesting when threshold is reached
        if (systemInitialized) {

            if (coverage >= threshold && !isHarvesting) {

                digitalWrite(LED_PIN, HIGH);

                isHarvesting = true;

                harvestDuckweed();

                isHarvesting = false;

                // Force system to wait for a new reading
                coverage = -1;

                digitalWrite(LED_PIN, LOW);
            }
        }
    }

    delay(10);
}


void harvestDuckweed() {

    Serial.println("Starting harvesting process...");

    // Motor 1 moves collection mechanism forward
    Serial.println("Motor 1 moving forward (collecting duckweed)");

    moveStepper(
        STEP_MOTOR1,
        DIR_MOTOR1,
        1100,
        HIGH
    );

    delay(1000);


    // Motor 2 lifts collection mechanism
    Serial.println("Motor 2 moving upward (lifting net)");

    moveStepper(
        STEP_MOTOR2,
        DIR_MOTOR2,
        500,
        HIGH
    );


    // Allow lifting mechanism to settle
    delay(5000);


    Serial.println("Delay to take out duckweed (5 seconds)");

    delay(5000);


    // Lower the lifting mechanism
    Serial.println("Motor 2 moving downward (lowering net)");

    moveStepper(
        STEP_MOTOR2,
        DIR_MOTOR2,
        500,
        LOW
    );


    // Return collection mechanism to starting position
    Serial.println("Motor 1 moving backward (returning to original position)");

    moveStepper(
        STEP_MOTOR1,
        DIR_MOTOR1,
        1100,
        LOW
    );


    Serial.println("Harvesting process completed");

    // Wait before another harvesting cycle
    delay(10000);
}


void moveStepper(
    int stepPin,
    int dirPin,
    int steps,
    bool direction
) {

    digitalWrite(dirPin, direction);

    for (int i = 0; i < steps; i++) {

        digitalWrite(stepPin, HIGH);

        delayMicroseconds(2000);

        digitalWrite(stepPin, LOW);

        delayMicroseconds(2000);
    }
}
