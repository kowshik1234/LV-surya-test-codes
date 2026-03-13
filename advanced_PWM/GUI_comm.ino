#include <Arduino.h>
#include "driver/mcpwm.h"

// --- Hardware Mapping ---
// Verify all pins against your specific ESP32 board schematic.
#define IR_850      2
#define BLUE        42
#define RED         40
#define WHITE       38
#define GREEN       36
#define VIOLET      16
#define IR_950      7
#define FAR_RED_740 5
#define LM3409_PIN  13

// --- PWM Configuration ---
#define LEDC_TIMER_BITS 8       // 8-bit resolution: duty range 0–255
#define LEDC_FREQ_HZ    500     // Hz
#define MCPWM_FREQ_HZ   500     // Hz — LM3409 PWM input frequency
#define SERIAL_BAUD     115200

// --- Frame Protocol ---
#define FRAME_START  'S'
#define FRAME_TERM   ';'
#define BUF_CAPACITY 64         // "S,255,255,...x9" is ~38 chars; 64 is safe
#define NUM_VALUES   9          // 8 LEDC channels + 1 MCPWM channel

// LEDC channels 0–7 map to these pins, in order
static const uint8_t LED_PINS[8] = {
    IR_850, BLUE, RED, WHITE, GREEN, VIOLET, IR_950, FAR_RED_740
};

// Non-blocking accumulation state
static char    rxBuf[BUF_CAPACITY];
static uint8_t rxIdx = 0;

// -----------------------------------------------------------------------
// LM3409 — MCPWM peripheral (legacy API)
//
// WARNING: The legacy MCPWM API is deprecated in ESP-IDF v5.x /
// Arduino Core 3.x. It still compiles and works but will emit
// deprecation warnings. To eliminate them, replace with the new
// ESP-IDF MCPWM driver, or simply reassign LM3409_PIN to a spare
// LEDC channel (ch 8 or higher) and remove this section entirely.
// -----------------------------------------------------------------------
static void lm3409_init(void) {
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, LM3409_PIN);

    mcpwm_config_t cfg = {};
    cfg.frequency    = MCPWM_FREQ_HZ;
    cfg.cmpr_a       = 0.0f;
    cfg.cmpr_b       = 0.0f;             // OPR_B unused
    cfg.counter_mode = MCPWM_UP_COUNTER;
    cfg.duty_mode    = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &cfg);
}

// raw: 0–255 in, converted to 0.0–100.0 % duty for MCPWM.
static void lm3409_write(int raw) {
    raw = constrain(raw, 0, 255);
    float duty = (raw * 100.0f) / 255.0f;

    // Operate only on OPR_A — the operator tied to LM3409_PIN via MCPWM0A.
    // Do NOT touch OPR_B; it is uninitialized and unconnected.
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}

// -----------------------------------------------------------------------
// Parser
//
// frame — null-terminated string, e.g. "S,0,128,255,0,0,0,0,0,42"
//         The FRAME_TERM character has already been consumed.
// out   — caller-supplied array of NUM_VALUES ints.
// Returns true only if exactly NUM_VALUES values are parsed cleanly.
// -----------------------------------------------------------------------
static bool parse_frame(const char* frame, int out[NUM_VALUES]) {
    if (frame[0] != FRAME_START || frame[1] != ',') return false;

    // strtok modifies the string in place — work on a local copy.
    char tmp[BUF_CAPACITY];
    strncpy(tmp, frame + 2, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    int   count = 0;
    char* tok   = strtok(tmp, ",");

    while (tok != NULL && count < NUM_VALUES) {
        char* end = NULL;
        long  val = strtol(tok, &end, 10);

        if (end == tok) return false;   // No digits consumed — reject immediately

        out[count++] = (int)constrain(val, 0L, 255L);
        tok = strtok(NULL, ",");
    }

    return (count == NUM_VALUES);       // Fail if too few values
}

// -----------------------------------------------------------------------
// Apply a fully validated frame to hardware.
// -----------------------------------------------------------------------
static void apply_frame(const int vals[NUM_VALUES]) {
    for (int i = 0; i < 8; i++) {
        ledcWriteChannel(i, (uint32_t)vals[i]);
    }
    lm3409_write(vals[8]);
}

// -----------------------------------------------------------------------
// Validate and apply the accumulated null-terminated frame in rxBuf.
// -----------------------------------------------------------------------
static void process_frame(void) {
    // Strip any stray trailing CR/LF that arrived before the terminator.
    int len = (int)strlen(rxBuf);
    while (len > 0 && (rxBuf[len - 1] == '\r' || rxBuf[len - 1] == '\n')) {
        rxBuf[--len] = '\0';
    }

    // Skip any leading whitespace / line-endings from a corrupted prefix.
    const char* start = rxBuf;
    while (*start == '\r' || *start == '\n' || *start == ' ') ++start;

    if (*start == '\0') return;     // Nothing meaningful — discard silently.

    int vals[NUM_VALUES] = {};
    if (parse_frame(start, vals)) {
        apply_frame(vals);
        Serial.println("OK");
    } else {
        Serial.print("ERR:BAD_FRAME ");
        Serial.println(rxBuf);     // Echo the offending frame for diagnostics
    }
}

// -----------------------------------------------------------------------
// setup()
// -----------------------------------------------------------------------
void setup() {
    Serial.begin(SERIAL_BAUD);
    // No Serial.setTimeout() — non-blocking accumulation does not use
    // readBytesUntil(), so a timeout has no useful effect here.

    lm3409_init();

    // Requires ESP32 Arduino Core 3.x (ledcAttachChannel / ledcWriteChannel).
    for (int i = 0; i < 8; i++) {
        ledcAttachChannel(LED_PINS[i], LEDC_FREQ_HZ, LEDC_TIMER_BITS, i);
        ledcWriteChannel(i, 0);    // All LED channels off at startup
    }
    lm3409_write(0);               // LM3409 off at startup

    Serial.println("READY");
}

// -----------------------------------------------------------------------
// loop() — Non-blocking single-byte accumulation.
//
// Primary terminator : ';'   (preferred — use serial.write(b"S,...,v9;"))
// Fallback terminator: '\n'  (accepted — handles Python print() / \n frames)
// '\r' is stripped silently  (Windows \r\n line endings, Arduino IDE monitor)
// -----------------------------------------------------------------------
void loop() {
    while (Serial.available() > 0) {
        char c = (char)Serial.read();

        if (c == FRAME_TERM || c == '\n') {
            // End of frame — process whatever we have accumulated.
            rxBuf[rxIdx] = '\0';
            if (rxIdx > 0) {
                process_frame();
            }
            rxIdx = 0;              // Reset accumulator for the next frame.

        } else if (c == '\r') {
            // Silently discard CR (harmless prefix to \n on Windows).

        } else {
            if (rxIdx < BUF_CAPACITY - 1) {
                rxBuf[rxIdx++] = c;
            } else {
                // No terminator before buffer filled — corrupted stream.
                Serial.println("ERR:OVERFLOW");
                rxIdx = 0;          // Discard and wait for the next frame.
            }
        }
    }
}
