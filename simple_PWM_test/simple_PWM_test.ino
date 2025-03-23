/*
  ESP32-S3 code to control PWM of pins through the serial monitor.

  Macros:
  #define IR_850 2
  #define BLUE 42
  #define RED 40
  #define WHITE 38
  #define GREEN 36
  #define VIOLET 16
  #define IR_950 7
  #define Far_red_740 5

  PWM configuration:
  - Frequency: Irrelevant for analogWrite
  - Resolution: 8 bits (implied by analogWrite)
  - Maximum duty cycle: 255 (implied by analogWrite)
*/

#include <Arduino.h>
#include "driver/mcpwm.h"

// Define the pins using macros (as provided)
#define IR_850 2
#define BLUE 42
#define RED 40
#define WHITE 38
#define GREEN 36
#define VIOLET 16
#define IR_950 7
#define Far_red_740 5

#define LM3409_pin 13 

// Global variable to store the currently selected pin
int selected_pin = -1; // -1 indicates no pin is selected initially

void setup() {
  Serial.begin(115200); // Initialize serial communication
  LM3409_setup();
}

void loop() {
  if (Serial.available() > 0) {
    String inputString = Serial.readStringUntil('\n');
    inputString.trim(); // Remove leading/trailing whitespace

    if (selected_pin == -1) {
      // Pin Selection Menu
      int selection = inputString.toInt(); // Convert the string to an integer

      switch (selection) {
        case 1:
          selected_pin = IR_850;
          Serial.print("Controlling IR_850 Pin ");
          printPWMOptions();
          break;
        case 2:
          selected_pin = BLUE;
          Serial.print("Controlling BLUE Pin ");
          printPWMOptions();
          break;
        case 3:
          selected_pin = RED;
          Serial.print("Controlling RED Pin ");
          printPWMOptions();
          break;
        case 4:
          selected_pin = WHITE;
          Serial.print("Controlling WHITE Pin ");
          printPWMOptions();
          break;
        case 5:
          selected_pin = GREEN;
          Serial.print("Controlling GREEN Pin ");
          printPWMOptions();
          break;
        case 6:
          selected_pin = VIOLET;
          Serial.print("Controlling VIOLET Pin ");
          printPWMOptions();
          break;
        case 7:
          selected_pin = IR_950;
          Serial.print("Controlling IR_950 Pin ");
          printPWMOptions();
          break;
        case 8:
          selected_pin = Far_red_740;
          Serial.print("Controlling Far_red_740 Pin ");
          printPWMOptions();
          break;
        case 9:
          selected_pin = LM3409_pin;
          Serial.print("Controlling LM3409_pin ");
          printPWMOptions();
          break;
        default:
          Serial.println("Invalid Selection");
          printPinList();
          break;
      }
    } else {
      // PWM Control Menu
      int selection = inputString.toInt(); // Convert the string to an integer
      int current_duty; // Declare these outside the cases
      int new_duty;
      int lm34_duty;

      switch (selection) {
        case 1:
          Serial.println("PWM On and duty cycle set to 50%");
          // PWM is already "on" in the sense that the pin is configured.
          // You just need to set a duty cycle. To start, set it to half.
          if(selected_pin == LM3409_pin)
          {
            lm34_duty = 50;
            pwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, lm34_duty);
          }
          else
          {
            analogWrite(selected_pin, 128); // Start with 50% duty cycle
          }
          break;
        case 2:
          Serial.println("PWM Off");
          if(selected_pin == LM3409_pin)
          {
            lm34_duty = 0;
            pwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, lm34_duty);
          }
          else
          {
            analogWrite(selected_pin, 0); // Start with 50% duty cycle
          }
          break;
        case 3:
          Serial.println("Increase Duty Cycle (+10)");
          if(selected_pin == LM3409_pin)
          {
            lm34_duty +=10;
            if(lm34_duty > 255)
            {
              lm34_duty = 255;
            }
            pwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, lm34_duty);
          }
          else
          {
            current_duty = map(analogRead(selected_pin), 0, 1023, 0, 255); // Map the analog read to 0-255
            new_duty = current_duty + 10;
            if (new_duty > 255) {
              new_duty = 255;
            }
            analogWrite(selected_pin, new_duty);
          }
          Serial.print("New Duty Cycle: ");
          Serial.println(new_duty);
          break;
        case 4:
          Serial.println("Decrease Duty Cycle (-10)");
          if(selected_pin == LM3409_pin)
          {
            lm34_duty -=10;
            if(lm34_duty < 0)
            {
              lm34_duty = 0;
            }
            pwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, lm34_duty);
          }
          else
          {
            current_duty = map(analogRead(selected_pin), 0, 1023, 0, 255); // Map the analog read to 0-255
            new_duty = current_duty - 10;
            if (new_duty < 0) {
              new_duty = 0;
            }
            analogWrite(selected_pin, new_duty);
          }
          Serial.print("New Duty Cycle: ");
          Serial.println(new_duty);
          break;
        case 5:
          selected_pin = -1; // Go back to pin selection
          Serial.println("Returning to Pin Selection");
          printPinList();
          break;
        case 6:
          digitalWrite(selected_pin, !digitalRead(selected_pin)); // Toggles the pin's state
          Serial.print("Toggled pin: ");
          Serial.println(selected_pin);
          break;
        default:
          Serial.println("Invalid Selection");
          printPWMOptions();
          break;
      }
    }
  }
}

void LM3409_setup()
{
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, LM3409_pin);
    mcpwm_config_t pwm_config = {};
    pwm_config.frequency = 500;
    pwm_config.cmpr_a = 0;
    pwm_config.cmpr_b = 0;
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

}

static void pwm_set_duty(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num, float duty_cycle) {
  mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
  mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cycle);
  mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}

void printPinList() {
  Serial.println("Select a pin to control PWM:");
  Serial.println("1. IR_850 (Pin 2)");
  Serial.println("2. BLUE (Pin 42)");
  Serial.println("3. RED (Pin 40)");
  Serial.println("4. WHITE (Pin 38)");
  Serial.println("5. GREEN (Pin 36)");
  Serial.println("6. VIOLET (Pin 16)");
  Serial.println("7. IR_950 (Pin 7)");
  Serial.println("8. Far_red_740 (Pin 5)");
  Serial.println("9. LM3409 Ti Driver (Pin 13)");

  Serial.println("Enter the number of the pin:");
}

void printPWMOptions() {
  Serial.println("PWM Control Options:");
  Serial.println("1. Switch On PWM");
  Serial.println("2. Switch Off PWM");
  Serial.println("3. Increase Duty Cycle (+10)");
  Serial.println("4. Decrease Duty Cycle (-10)");
  Serial.println("5. Return to Pin Selection");
  Serial.println("6. Simple digital toggle");
  Serial.println("Enter the number of the option:");
}