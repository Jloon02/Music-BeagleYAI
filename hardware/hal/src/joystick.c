#include <assert.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "hal/gpio.h"
#include "hal/i2c.h"
#include "hal/joystick.h"
#include "hal/timeFunction.h"

// Code taken and adapted from https://opencoursehub.cs.sfu.ca/bfraser/solutions/433/guide-code/i2c_adc_tla2024/tla2024_demo.c

// the push button is GPIO5 -> gpiochip2 15
// Which channel to sample?
#define GPIO_CHIP_JOYSTICKBUTTON 2
#define GPIO_PUSH_PIN 15  // GPIO pin number for button

// Allow module to ensure it has been initialized (once!)
static bool is_initialized = false;
struct GpioLine* s_lineButton = NULL;
static bool prevPressed = false;

// Variables to determine hold time
static long long buttonPressStartTime = 0;
static bool isButtonPressed = false;

static int joystick_read_y() {
    int i2c_file_desc = I2C_init_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS_JOYSTICK);

    // Select the channel
    I2C_write_reg16(i2c_file_desc, REG_CONFIGURATION, TLA2024_CHANNEL_CONF_0);
    uint16_t raw_read = I2C_read_reg16(i2c_file_desc, REG_DATA);
    close(i2c_file_desc);

    // Convert byte order and shift bits into place
    uint16_t value = ((raw_read & 0xFF) << 8) | ((raw_read & 0xFF00) >> 8);
    value = value >> 4;

    return value;
}


static int joystick_read_x() {
    int i2c_file_desc = I2C_init_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS_JOYSTICK);

    // Select the channel
    I2C_write_reg16(i2c_file_desc, REG_CONFIGURATION, TLA2024_CHANNEL_CONF_1);
    uint16_t raw_read = I2C_read_reg16(i2c_file_desc, REG_DATA);
    close(i2c_file_desc);

    // Convert byte order and shift bits into place
    uint16_t value = ((raw_read & 0xFF) << 8) | ((raw_read & 0xFF00) >> 8);
    value = value >> 4;

    return value;
} 

static int normalize(int value, int x_min, int x_max) {
    return ((value - x_min) * 200 / (x_max - x_min)) - 100;
}

Direction get_direction() {
    int xValue = normalize(joystick_read_x(), LEFT, RIGHT);
    int yValue = normalize(joystick_read_y(), UP, DOWN);
    if (xValue <= -THRESHOLD) {
        return DIR_UP;
    } else if (xValue >= THRESHOLD) {
        return DIR_DOWN;
    } else if (yValue <= -THRESHOLD) {
        return DIR_LEFT;
    } else if (yValue >= THRESHOLD) {
        return DIR_RIGHT;
    } else {
        return DIR_NONE;
    }
}

bool joystick_button_clicked(void)
{
    int currPush = gpiod_line_get_value((struct gpiod_line*)s_lineButton);
    if (currPush < 0) {
        perror("Failed to read joystick button GPIO");
        return false;
    }

    // Falling edge detection: button was just pressed (1 → 0)
    if (currPush == 0 && !prevPressed) {
        printf("Joystick: Button Pressed\n");
        prevPressed = true;
        buttonPressStartTime = get_time_in_ms();
        isButtonPressed = true;
        return true;
    } else if (currPush == 1) {
        // Button released
        prevPressed = false;
        isButtonPressed = false;
    }
    return false;
}

bool joystick_button_is_pressed(void)
{
    int currPush = gpiod_line_get_value((struct gpiod_line*)s_lineButton);
    return (currPush == 0);  // Active low
}

bool joystick_button_is_held_for_ms(long long duration_ms)
{
    if (!isButtonPressed) return false;
    
    long long currentTime = get_time_in_ms();
    if ((currentTime - buttonPressStartTime) >= duration_ms) {
        // Reset the hold detection
        isButtonPressed = false;
        return true;
    }
    return false;
}

// ─────────────────────────────────────────────
// ─────────────── Init & Cleanup ──────────────
// ─────────────────────────────────────────────

void Joystick_init(void)
{
    assert(!is_initialized);
    s_lineButton = Gpio_openForEvents(GPIO_CHIP_JOYSTICKBUTTON, GPIO_PUSH_PIN);
    gpiod_line_request_input((struct gpiod_line*)s_lineButton, "Joystick Button");
    is_initialized = true;
    
}

void Joystick_cleanup(void)
{
    // Free any memory, close files, ...
    assert(is_initialized);
    Gpio_close(s_lineButton);
    is_initialized = false;
}