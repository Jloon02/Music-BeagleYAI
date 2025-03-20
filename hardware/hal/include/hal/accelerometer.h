// accelerometer.h
// Handles the acceleometer from i2c
// Takes into account debouncing

#ifndef _ACCELEROMETER_H_
#define _ACCELEROMETER_H_

#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define I2C_DEVICE_ADDRESS 0x19
#include <stdbool.h>

void Accelerometer_init(void);
void Accelerometer_cleanup(void);
bool accelerometer_x_sound(void);
bool accelerometer_y_sound(void);
bool accelerometer_z_sound(void);
#endif