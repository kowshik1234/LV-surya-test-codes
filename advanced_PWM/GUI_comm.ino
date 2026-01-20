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

// Define PWM channel numbers (0-7, ESP32-S3 has 8 independent PWM channels)
#define IR_850_CHANNEL 0
#define BLUE_CHANNEL 1
#define RED_CHANNEL 2
#define WHITE_CHANNEL 3
#define GREEN_CHANNEL 4
#define VIOLET_CHANNEL 5
#define IR_950_CHANNEL 6
#define Far_red_740_CHANNEL 7

/* PWM Configuration macros*/
#define LEDC_TIMER_8_BIT 8
// use 500 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ 500

// Global array to store current duty cycles for each channel
int dutyCycles[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int lm3409Duty = 0;

int values[9];  // Array to store the 9 values

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


void setup() {
  Serial.begin(9600);  // Initialize serial at 9600 baud
  Serial.println("Ready to receive frames");  // Debug message

  LM3409_setup();

  ledcAttachChannel(IR_850, LEDC_BASE_FREQ, LEDC_TIMER_8_BIT, IR_850_CHANNEL);
  ledcAttachChannel(BLUE, LEDC_BASE_FREQ, LEDC_TIMER_8_BIT, BLUE_CHANNEL);
  ledcAttachChannel(RED, LEDC_BASE_FREQ, LEDC_TIMER_8_BIT, RED_CHANNEL);
  ledcAttachChannel(WHITE, LEDC_BASE_FREQ, LEDC_TIMER_8_BIT, WHITE_CHANNEL);
  ledcAttachChannel(GREEN, LEDC_BASE_FREQ, LEDC_TIMER_8_BIT, GREEN_CHANNEL);
  ledcAttachChannel(VIOLET, LEDC_BASE_FREQ, LEDC_TIMER_8_BIT, VIOLET_CHANNEL);
  ledcAttachChannel(IR_950, LEDC_BASE_FREQ, LEDC_TIMER_8_BIT, IR_950_CHANNEL);
  ledcAttachChannel(Far_red_740, LEDC_BASE_FREQ, LEDC_TIMER_8_BIT, Far_red_740_CHANNEL);

}

int process_frame(String frame)
{
  int ret = -1;
  if (frame.startsWith("S,"))
  {   
      // Check start of frame
      frame = frame.substring(2);  // Remove "S,"
      int index = 0;
      while (frame.length() > 0 && index < 9) 
      {
        int commaIndex = frame.indexOf(',');
        String valStr;
        if (commaIndex == -1) {
          valStr = frame;  // Last value
        } else {
          valStr = frame.substring(0, commaIndex);
          frame = frame.substring(commaIndex + 1);
        }
        values[index] = valStr.toInt();  // Convert to int and store
        index++;
      
      ret = 1;
      }
  }
  else
  {
    ret = 0;
  }

  return ret;
}

void loop() {
  if (Serial.available() > 0) {
    String frame = Serial.readStringUntil(';');  // Read until ';'
    int ret = process_frame(frame);
    if(ret)
    {
      for(int i=0; i<9;i++)
      {
        if(i==8)
        {
          // Serial.println("LM3409 value written %d",values[i]);
          pwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, (values[i] / 255.0) * 100.0);
        }
        else
        {
          ledcWriteChannel(i, values[i]);
        }
      }
    }
  }
}
