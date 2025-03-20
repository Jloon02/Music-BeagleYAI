#include "hal/accelerometer.h"
#include "hal/i2c.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Connected to I2C_1 at address 0x19
// Register WHO_AM_I: i2cget -y 1 0x19 0x0F

// Control regsiters are:
// 0x20 to 0x25

// To enable accelerometer in normal mode with all axis
// i2cset -y 1 0x19 0x20 0x57

// To read X-axis data:
// low byte: i2cget -y 1 0x19 0x28
// high byte: i2cget -y 1 0x19 0x29

// To read Y-axis data;
// low byte: i2cget -y 1 0x19 0x2A
// high byte: i2cget -y 1 0x19 0x2B

// To read Z-axis data
// low byte: i2cget -y l 0x19 0x2C
// high byte: i2cget -y 1 0x19 0x2D

#define DEBOUNCE_TIME_MS 475  // Adjust this value based on how sensitive you want it

static struct timespec last_trigger_time = {0, 0};  // To store the last trigger time

static bool isInitialized = false;

static float baseline_x = -400.0f;  // Average of X readings
static float baseline_y = 340.0f;  // Average of Y readings
static float baseline_z = 16580.0f;  // Resting Z-axis value at 1g in raw counts
static float threshold_z = 4000.0f;   // Threshold for detecting movement (adjust based on calibration

// Function to read a signed 16-bit acceleration value from two registers (low and high)
static int16_t read_axis(int reg_low, int reg_high)
{
    int i2c_fd = I2C_init_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS);
    
    uint8_t l = I2C_read_reg8(i2c_fd, reg_low);
    uint8_t h = I2C_read_reg8(i2c_fd, reg_high);
    close(i2c_fd);
    
    // Combine low and high into a 16-bit signed integer (2's complement)
    return (int16_t)((h << 8) | l);
}

bool accelerometer_x_sound(void)
{
    int x_raw = read_axis(0x28, 0x29);  // OUT_X_L, OUT_X_H
    
    // Check if X value is above the threshold indicating movement (taking into account the baseline)
    if (x_raw > baseline_x + threshold_z || x_raw < baseline_x - threshold_z) {
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);  // Get current time

        // Calculate the time difference (in milliseconds) from last trigger time
        long time_diff_ms = (current_time.tv_sec - last_trigger_time.tv_sec) * 1000 + 
                            (current_time.tv_nsec - last_trigger_time.tv_nsec) / 1000000;

        // If enough time has passed since the last trigger (debounce check)
        if (time_diff_ms >= DEBOUNCE_TIME_MS) {
            // Significant movement detected, trigger sound
            last_trigger_time = current_time;  // Update last trigger time
            return true;
        }
    }

    return false;
}

// Function to check if the accelerometer Y-axis has triggered a sound (with debounce)
bool accelerometer_y_sound(void)
{
    int y_raw = read_axis(0x2A, 0x2B);  // OUT_Y_L, OUT_Y_H
    
    // Check if Y value is above the threshold indicating movement (taking into account the baseline)
    if (y_raw > baseline_y + threshold_z || y_raw < baseline_y - threshold_z) {
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);  // Get current time

        // Calculate the time difference (in milliseconds) from last trigger time
        long time_diff_ms = (current_time.tv_sec - last_trigger_time.tv_sec) * 1000 + 
                            (current_time.tv_nsec - last_trigger_time.tv_nsec) / 1000000;

        // If enough time has passed since the last trigger (debounce check)
        if (time_diff_ms >= DEBOUNCE_TIME_MS) {
            // Significant movement detected, trigger sound
            last_trigger_time = current_time;  // Update last trigger time
            return true;
        }
    }

    return false;
}

// Function to check if the accelerometer Z-axis has triggered a sound (with debounce)
bool accelerometer_z_sound(void)
{
    int z_raw = read_axis(0x2C, 0x2D);  // OUT_Z_L, OUT_Z_H
    
    // Check if Z value is above the threshold indicating movement (taking into account the baseline)
    if (z_raw > baseline_z + threshold_z || z_raw < baseline_z - threshold_z) {
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);  // Get current time

        // Calculate the time difference (in milliseconds) from last trigger time
        long time_diff_ms = (current_time.tv_sec - last_trigger_time.tv_sec) * 1000 + 
                            (current_time.tv_nsec - last_trigger_time.tv_nsec) / 1000000;

        // If enough time has passed since the last trigger (debounce check)
        if (time_diff_ms >= DEBOUNCE_TIME_MS) {
            // Significant movement detected, trigger sound
            last_trigger_time = current_time;  // Update last trigger time
            return true;
        }
    }

    return false;
}

static void enableAccelerometer() {
    // Initialize the I2C bus
    int i2c_fd = I2C_init_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS);

    // Write 0x57 to register 0x20 (CTRL_REG1)
    uint8_t ctrl_reg1 = 0x20;
    uint8_t ctrl_reg1_val = 0x57;
    I2C_write_reg16(i2c_fd, ctrl_reg1, ctrl_reg1_val);

    // Close I2C file descriptor
    close(i2c_fd);
}

// ─────────────────────────────────────────────
// ─────────────── Init & Cleanup ──────────────
// ─────────────────────────────────────────────

void Accelerometer_init(void) 
{
    assert(!isInitialized);
    enableAccelerometer();
    isInitialized = true;
}

void Accelerometer_cleanup(void)
{
    assert(isInitialized);
    isInitialized = false;
}