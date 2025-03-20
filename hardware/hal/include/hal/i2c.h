// For read/writing bits
#ifndef _I2C_H_
#define _I2C_H_

#include <stdint.h>

int I2C_init_bus(char* bus, int address);
void I2C_write_reg16(int i2c_file_desc, uint8_t reg_addr, uint16_t value);
uint16_t I2C_read_reg16(int i2c_file_desc, uint8_t reg_addr);
uint8_t I2C_read_reg8(int i2c_file_desc, uint8_t reg_addr);

#endif