// NeoPixel Music Visualizer
// For BeagleY-AI's MCU R5
// Responds to amplitude values written by A72 core

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <math.h>
#include "r5_shared_data.h"

// Memory
#define SHARED_MEM_BTCM_START 0x00000000
static volatile uint8_t *pR5Base = NULL;

// NeoPixel Configuration
#define NEO_NUM_LEDS          8
#define MAX_AMPLITUDE        15  // Matches scale factor on A72 side

// NeoPixel Timing (same as original)
#define NEO_ONE_ON_NS       700
#define NEO_ONE_OFF_NS      600
#define NEO_ZERO_ON_NS      350
#define NEO_ZERO_OFF_NS     800
#define NEO_RESET_NS      60000

// Delay macros (same as original)
volatile int junk_delay = 0;
#define DELAY_350_NS() {}
#define DELAY_600_NS() {for (junk_delay=0; junk_delay<9 ;junk_delay++);}
#define DELAY_700_NS() {for (junk_delay=0; junk_delay<16 ;junk_delay++);}
#define DELAY_800_NS() {for (junk_delay=0; junk_delay<23 ;junk_delay++);}
#define DELAY_NS(ns) do {int target = k_cycle_get_32() + k_ns_to_cyc_near32(ns); \
    while(k_cycle_get_32() < target) ; } while(false)

#define NEO_DELAY_ONE_ON()     DELAY_700_NS()
#define NEO_DELAY_ONE_OFF()    DELAY_600_NS()
#define NEO_DELAY_ZERO_ON()    DELAY_350_NS()
#define NEO_DELAY_ZERO_OFF()   DELAY_800_NS()
#define NEO_DELAY_RESET()      {DELAY_NS(NEO_RESET_NS);}

// Device tree nodes
#define NEOPIXEL_NODE DT_ALIAS(neopixel)
static const struct gpio_dt_spec neopixel = GPIO_DT_SPEC_GET(NEOPIXEL_NODE, gpios);

static void initialize_gpio(const struct gpio_dt_spec *pPin, int direction) {
    if (!gpio_is_ready_dt(pPin)) {
        printf("ERROR: GPIO pin not ready\n");
        exit(EXIT_FAILURE);
    }

    int ret = gpio_pin_configure_dt(pPin, direction);
    if (ret < 0) {
        printf("ERROR: GPIO Pin Configure issue\n");
        exit(EXIT_FAILURE);
    }
}

// Send color data to NeoPixel
static void R5_setColour(uint32_t colour) {
    for (int i = 31; i >= 0; i--) {
        if(colour & ((uint32_t)0x1 << i)) {
            gpio_pin_set_dt(&neopixel, 1);
            NEO_DELAY_ONE_ON();
            gpio_pin_set_dt(&neopixel, 0);
            NEO_DELAY_ONE_OFF();
        } else {
            gpio_pin_set_dt(&neopixel, 1);
            NEO_DELAY_ZERO_ON();
            gpio_pin_set_dt(&neopixel, 0);
            NEO_DELAY_ZERO_OFF();
        }
    }
}

// Convert HSV to RGB (for color effects)
uint32_t hsv_to_rgb(float h, float s, float v) {
    float r, g, b;
    int i = (int)(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch(i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }

    return ((uint8_t)(r * 255) << 16) | ((uint8_t)(g * 255) << 8) | (uint8_t)(b * 255);
}

int main(void) {
    printf("Music Visualizer R5\n");

    // Initialize GPIO
    initialize_gpio(&neopixel, GPIO_OUTPUT_ACTIVE);

    pR5Base = (volatile void*)SHARED_MEM_BTCM_START;
    uint32_t last_amplitude = 0;
    float hue = 0.0f;

    while (true) {
        gpio_pin_set_dt(&neopixel, 0);
        DELAY_NS(NEO_RESET_NS);

        // Read amplitude value from shared memory
        uint32_t amplitude = MEM_UINT32(pR5Base + AMP_OFFSET);
        
        // Update hue for color cycling (slow change over time)
        hue += 0.001f;
        if (hue >= 1.0f) hue = 0.0f;

        // Simple VU meter mode - lights up LEDs proportional to amplitude
        if (amplitude != last_amplitude) {
            int leds_to_light = (amplitude * NEO_NUM_LEDS) / MAX_AMPLITUDE;
            
            for (int j = 0; j < NEO_NUM_LEDS; j++) {
                if (j < leds_to_light) {
                    // Calculate color - hue changes along strip, brightness based on position
                    float led_hue = hue + (j * 0.12f);
                    if (led_hue >= 1.0f) led_hue -= 1.0f;
                    float brightness = 0.2f + (0.8f * j / leds_to_light);
                    uint32_t color = hsv_to_rgb(led_hue, 1.0f, brightness);
                    R5_setColour(color);
                } else {
                    R5_setColour(0x00000000); // Off
                }
            }
            
            last_amplitude = amplitude;
        }

        // Pulse effect for high amplitudes
        if (amplitude > (MAX_AMPLITUDE * 0.8)) {
            static uint32_t pulse_brightness = 0;
            static bool increasing = true;
            
            if (increasing) {
                pulse_brightness += 0x020202;
                if (pulse_brightness >= 0x101010) increasing = false;
            } else {
                pulse_brightness -= 0x020202;
                if (pulse_brightness <= 0x020202) increasing = true;
            }
            
            // Apply pulse to all lit LEDs
            for (int j = 0; j < NEO_NUM_LEDS; j++) {
                if (j < ((amplitude * NEO_NUM_LEDS) / MAX_AMPLITUDE)) {
                    uint32_t base_color = hsv_to_rgb(hue + (j * 0.12f), 1.0f, 0.8f);
                    R5_setColour(base_color + pulse_brightness);
                }
            }
            
            k_busy_wait(5 * 1000); // Faster updates for pulse effect
        } else {
            k_busy_wait(20 * 1000); // Normal update rate
        }
    }

    return 0;
}