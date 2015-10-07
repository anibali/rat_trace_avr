#ifndef __I2C_H__
#define __I2C_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  I2C_Error_None,
  I2C_Error_Unknown
} I2C_Error;

void i2c_init();
void i2c_start(I2C_Error *error);
void i2c_stop();
void i2c_write_byte(uint8_t data, I2C_Error *error);
uint8_t i2c_read_byte(bool nack, I2C_Error *error);
void i2c_write_sla(uint8_t slave_addr, I2C_Error *error);
void i2c_read_sla(uint8_t slave_addr, I2C_Error *error);

void i2c_read_register(uint8_t *data, uint8_t n_bytes, uint8_t slave_addr,
  uint8_t reg_addr, bool end_with_nack, I2C_Error *error);
void i2c_write_register(uint8_t *data, uint8_t n_bytes, uint8_t slave_addr,
  uint8_t reg_addr, bool resend_addr, I2C_Error *error);

#endif
