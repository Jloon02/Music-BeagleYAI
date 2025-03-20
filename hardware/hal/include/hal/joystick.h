// joystick module 
// Part of the Hardware Abstraction Layer (HAL) 

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

// Device bus & adress
#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define I2C_DEVICE_ADDRESS_JOYSTICK 0x48

// Register in TLA2024
#define REG_CONFIGURATION 0x01
#define REG_DATA 0x00

// Configuration reg contents for continously sampling different channels
#define TLA2024_CHANNEL_CONF_0 0x83C2
#define TLA2024_CHANNEL_CONF_1 0x83D2
#define TLA2024_CHANNEL_CONF_2 0x83E2
#define TLA2024_CHANNEL_CONF_3 0x83F2

#define RESTING_Y 814
#define RESTING_X 831

#define RIGHT 1625
#define LEFT 12
#define DOWN 1617
#define UP 7

#define TRUE_RIGHT 100
#define TRUE_LEFT -100
#define TRUE_DOWN 100
#define TRUE_UP -100

#define THRESHOLD 90

#include <stdbool.h>
typedef enum {
    DIR_NONE,
    DIR_UP,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT
} Direction;

void Joystick_init(void);
void Joystick_cleanup(void);
bool joystick_button_clicked(void);
Direction get_direction(void);
#endif