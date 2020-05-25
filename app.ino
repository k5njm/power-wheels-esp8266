//  Motor Control PWM

/*
Pinout:

D5  | 14 | WHITE   |   PEDAL       - Momentary Switch for the throttle pedal
D6  | 12 | BLACK   |   SHIFTER_A   - Toggle switch, Shifter/Gear selector.
D7  | 13 | RED     |   SHIFTER_B   - Toggle switch, Shifter/Gear selector.
D0  | 16 | BROWN   |   DIR1        - Cytron Motor Controller, Left Motor Direction
D1  | 05 | BLUE    |   PWM1        - Cytron Motor Controller, Left Motor PWM
D2  | 04 | RED     |   DIR2        - Cytron Motor Controller, Right Motor Direction
D8  | 15 | ORANGE  |   PWM2        - Cytron Motor Controller, Right Motor PWM
GND | GND| GRAY    |   GND         - Gnd Reference to Cytron motor controller, and switches


Pins are defined in code as GPIO number, not D number.

D0:  GPIO16         Wake up from sleep      | No pull-up resistor, but pull-down instead, should be connected to RST to wake up
D1:  GPIO5          SCL (I²C)
D2:  GPIO4          SDA (I²C)
D3:  GPIO0          Boot mode select | 3.3V | No Hi-Z
D4:  GPIO2          Boot mode select | TX1 	| 3.3V (boot only) 	Don’t connect to ground at boot time
D5:  GPIO14         SCK (SPI)
D6:  GPIO12         MISO (SPI)
D7:  GPIO13         MOSI (SPI)
D8:  GPIO15         SS (SPI)         | 0V   | Pull-up resistor not usable
D9:  GPIO3  (RX)    RX0                     | Not usable during Serial transmission
D10: GOIO1  (TX)    TX0	                    | Not usable during Serial transmission
D11: GPIO9  (SD2)  
D12: GPIO10 (SD3)


Notes: https://tttapa.github.io/ESP8266/Chap04%20-%20Microcontroller.html
Pin Mappings: https://www.electronicwings.com/nodemcu/nodemcu-gpio-with-arduino-ide
*/


// Pins used to tell the motor which direction to spin
#define  LEFT_MOTOR_DIR_PIN  16  // NodeMCU D0
#define  RIGHT_MOTOR_DIR_PIN 4  // NodeMCU D2

// Pins used to specify the PWM rate
//  must be one of 3, 5, 6, 9, 10 or 11 for Atmel 328 PWM
//  or any pin for ESP8266
#define  LEFT_MOTOR_PWM_PIN  5  // NodeMCU D1
#define  RIGHT_MOTOR_PWM_PIN 15 // NodeMCU D8

// Pushbutton
#define PEDAL_PIN 14        // NodeMCU D5
int pedal_state;            // the current reading from the input pin
int last_pedal_state = LOW; // the previous reading from the input pin


// Power Wheels Shifter Pins
#define POWER_WHEELS_SHIFTER_A_PIN 12 // NodeMCU D6
#define POWER_WHEELS_SHIFTER_B_PIN 13// NodeMCU D7
int shifter_state;          // Current state: 0: Top/High speed, 1: Middle/Low speed, 2: Bottom/Reverse/Low Speed
int shifter_state_prev;     
int shifter_a_reading;
int shifter_b_reading;

// TOP: A: HIGH | B: LOW  | Full Speed
// MID: A: HIGH | B: HIGH | Low Speed
// BOT: A: LOW  | B: HIGH | Reverse

// Ramping
#define PEDAL_ACTIVATED 0
#define RAMPING_SPEED 3000 //ms to ramp up or down
#define MAX_PWM 1023 // 254 for Arduino, 1023 for ESP8266
// #define R ((MAX_PWM * log10(2))/(log10(1024)))
float motor_power = 0;
float set_power = 0;


// State Machine
int system_state = 0;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;

void setup() {
    pinMode(LEFT_MOTOR_DIR_PIN, OUTPUT); // Initialize pin for output
    pinMode(RIGHT_MOTOR_DIR_PIN, OUTPUT); // Initialize pin for output

    digitalWrite(LEFT_MOTOR_DIR_PIN,HIGH); // set DIR pin HIGH or LOW
    digitalWrite(RIGHT_MOTOR_DIR_PIN,HIGH); // set DIR pin HIGH or LOW

    pinMode(PEDAL_PIN, INPUT_PULLUP); // declare push button as input
    pinMode(POWER_WHEELS_SHIFTER_A_PIN, INPUT_PULLUP); // declare push button as input
    pinMode(POWER_WHEELS_SHIFTER_B_PIN, INPUT_PULLUP); // declare push button as input



    Serial.begin(115200);
    Serial.println("Motor Example");

}

void loop() {
    currentMillis = millis();
    pedal_state = digitalRead(PEDAL_PIN);

    if ( currentMillis % 1000 == 0) {
        Serial.print("system_state: ");
        Serial.print(system_state);
        Serial.print(", motor power: ");
        Serial.print(motor_power);
        Serial.print(", set power: ");
        Serial.print(set_power);
        // Serial.print(", millis in cycle: ");
        // Serial.println(currentMillis - previousMillis);
        Serial.print(" pedal: ");
        Serial.print(pedal_state);
        Serial.print(", shifter_a: ");
        Serial.print(shifter_a_reading);
        Serial.print(", shifter_b: ");
        Serial.print(shifter_b_reading);
        Serial.print(", shifter_state: ");
        Serial.print(shifter_state);
        Serial.print(", shifter_state_prev: ");
        Serial.println(shifter_state_prev);
    }

    switch (system_state) {
    case 0: // Motors off
        if (pedal_state == PEDAL_ACTIVATED) {
           system_state = 2;
        }
        break;
    case 1: // Motors ramping down
        if (pedal_state == PEDAL_ACTIVATED) {
           system_state = 2;
        }
        else {
            if (set_power > 0) {
            set_power = set_power - 0.04;
            }
            else {
             system_state = 0;   
            }
        }        
        break;
    case 2: // Motors ramping up
        if (pedal_state == PEDAL_ACTIVATED) {
            if (set_power < MAX_PWM) {
            set_power = set_power + 0.04;
            }
            else {
            system_state = 3;
            }
        }
        else {
           system_state = 1; 
        }        
        break;
    case 3: // Motors on full
        if (pedal_state != PEDAL_ACTIVATED) {
           system_state = 1;
        }
        break;
    default:
        // statements
        break;
    }


    // Read/change shifter state only when we're not moving
    if (set_power < 10) {
        shifter_a_reading = digitalRead(POWER_WHEELS_SHIFTER_A_PIN);
        shifter_b_reading = digitalRead(POWER_WHEELS_SHIFTER_B_PIN);

        if (shifter_a_reading & !shifter_b_reading) {
            shifter_state = 0; // Top Position
        }
        else if (shifter_a_reading & shifter_b_reading) {
            shifter_state = 1; // Middle Position
        }
        else  if (!shifter_a_reading & shifter_b_reading) {
            shifter_state = 2; // Bottom Position
        }
       
        
    }


   switch (shifter_state) {
    case 0: // Top - High Speed
        motor_power = set_power;
        if (shifter_state != shifter_state_prev) {
            digitalWrite(LEFT_MOTOR_DIR_PIN, HIGH); // set DIR pin HIGH or LOW
            digitalWrite(RIGHT_MOTOR_DIR_PIN, HIGH); // set DIR pin HIGH or LOW
        }
        break;
    case 1: // Middle - Low Speed
        motor_power = set_power / 2;
        if (shifter_state != shifter_state_prev) {
            digitalWrite(LEFT_MOTOR_DIR_PIN, HIGH); // set DIR pin HIGH or LOW
            digitalWrite(RIGHT_MOTOR_DIR_PIN, HIGH); // set DIR pin HIGH or LOW
        }    
        break;
    case 2: // Bottom - Reverse & Low Speed
        motor_power = set_power / 2;
        if (shifter_state != shifter_state_prev) {
            digitalWrite(LEFT_MOTOR_DIR_PIN, LOW); // set DIR pin HIGH or LOW
            digitalWrite(RIGHT_MOTOR_DIR_PIN, LOW); // set DIR pin HIGH or LOW
        }
        break;
    default:
        motor_power = 0; // Fault Condition
        break;
    }
    
    analogWrite(LEFT_MOTOR_PWM_PIN, motor_power);
    analogWrite(RIGHT_MOTOR_PWM_PIN, motor_power);
    previousMillis = currentMillis;
    shifter_state_prev = shifter_state;
}
