#include <Arduino.h>
#include "driver/mcpwm.h"

// --- Hardware Mapping ---
#define IR_850       2
#define BLUE         42
#define RED          40
#define WHITE        38
#define GREEN        36
#define VIOLET       16
#define IR_950       7
#define Far_red_740  5
#define LM3409_pin   13

// --- PWM Configuration ---
#define LEDC_TIMER_BIT 8     // 0-255 range
#define LEDC_BASE_FREQ 500   // 500 Hz
#define SERIAL_BAUD    115200 // Faster baud rate recommended

// Array of pins for the 8 LEDC channels
const uint8_t LED_PINS[8] = {IR_850, BLUE, RED, WHITE, GREEN, VIOLET, IR_950, Far_red_740};

// Buffer for incoming serial data
char inputBuffer[64];
int values[9];

/**
 * Initialize the LM3409 via the MCPWM peripheral.
 * Note: MCPWM expects duty cycles in percentage (0.0 to 100.0).
 */
void LM3409_setup() {
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, LM3409_pin);
    mcpwm_config_t pwm_config = {};
    pwm_config.frequency = 500;
    pwm_config.cmpr_a = 0;
    pwm_config.cmpr_b = 0;
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
}

/**
 * Sets duty cycle for the LM3409.
 * Converts 0-255 input to 0.0-100.0 float for the driver.
 */
void update_lm3409(int rawValue) {
    float dutyPercent = (rawValue * 100.0) / 255.0;
    if (dutyPercent > 100.0) dutyPercent = 100.0;
    
    mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, dutyPercent);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}

/**
 * Parses a string in the format "S,v1,v2,v3,v4,v5,v6,v7,v8,v9"
 * Returns true only if all 9 values are found.
 */
bool parse_frame(char* frame) {
    if (frame[0] != 'S' || frame[1] != ',') return false;

    char* token = strtok(frame + 2, ","); // Skip "S,"
    int count = 0;

    while (token != NULL && count < 9) {
        values[count] = atoi(token);
        token = strtok(NULL, ",");
        count++;
    }
    
    return (count == 9); // Success only if we got exactly 9 values
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    Serial.setTimeout(10); // Reduce blocking time for Serial.readStringUntil
    Serial.println("System Ready. Send frame as S,v1...v9;");

    LM3409_setup();

    // Initialize 8 LEDC channels using the Core 3.x API
    for (int i = 0; i < 8; i++) {
        ledcAttachChannel(LED_PINS[i], LEDC_BASE_FREQ, LEDC_TIMER_BIT, i);
    }
}

void loop() {
    if (Serial.available() > 0) {
        // Read until the terminator ';'
        size_t len = Serial.readBytesUntil(';', inputBuffer, sizeof(inputBuffer) - 1);
        inputBuffer[len] = '\0'; // Null-terminate the string

        if (parse_frame(inputBuffer)) {
            // 1. Update first 8 LEDs (Standard PWM)
            for (int i = 0; i < 8; i++) {
                ledcWriteChannel(i, values[i]);
            }

            // 2. Update LM3409 (MCPWM with Scaling)
            update_lm3409(values[8]);
            
            // Optional: Debug output
            // Serial.println("Frame Applied Successfully");
        } else {
            Serial.println("Error: Invalid Frame Format");
        }
    }
}
