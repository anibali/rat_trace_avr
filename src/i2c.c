#include "i2c.h"

#include <avr/io.h>
#include <util/twi.h>

#define I2C_CLOCK_FREQUENCY 100000

static inline uint8_t i2c_wait_for_transmit() {
  while(!(TWCR & _BV(TWINT)));
  return TW_STATUS;
}

void i2c_init() {
  // Set prescale to 1
  TWSR = 0;
  // Set I2C clock rate
  TWBR = (((F_CPU / I2C_CLOCK_FREQUENCY) - 16) / 2);
  // Enable the two-wire interface
  TWCR = _BV(TWEN);
}

void i2c_start(I2C_Error *error) {
  // Send start condition
  TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);

  uint8_t status = i2c_wait_for_transmit();
  if(error != NULL) {
    *error = (status == TW_START || status == TW_REP_START) ?
      I2C_Error_None : I2C_Error_Unknown;
  }
}

void i2c_stop() {
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
}

void i2c_write_byte(uint8_t data, I2C_Error *error) {
  TWDR = data;
  TWCR = _BV(TWINT) | _BV(TWEN);

  uint8_t status = i2c_wait_for_transmit();
  if(error != NULL) {
    *error = (status == TW_MT_DATA_ACK) ? I2C_Error_None : I2C_Error_Unknown;
  }
}

uint8_t i2c_read_byte(bool nack, I2C_Error *error) {
  if(nack) {
    TWCR = _BV(TWINT) | _BV(TWEN);
  } else {
    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN);
  }

  uint8_t status = i2c_wait_for_transmit();
  if(error != NULL) {
    *error = (nack && status == TW_MR_DATA_NACK) || (!nack && TW_MR_DATA_ACK) ?
      I2C_Error_None : I2C_Error_Unknown;
  }

  return TWDR;
}

void i2c_write_sla(uint8_t slave_addr, I2C_Error *error) {
  TWDR = slave_addr | TW_WRITE;
  TWCR = _BV(TWINT) | _BV(TWEN);

  uint8_t status = i2c_wait_for_transmit();
  if(error != NULL) {
    *error = (status == TW_MT_SLA_ACK) ? I2C_Error_None : I2C_Error_Unknown;
  }
}

void i2c_read_sla(uint8_t slave_addr, I2C_Error *error) {
  TWDR = slave_addr | TW_READ;
  TWCR = _BV(TWINT) | _BV(TWEN);

  uint8_t status = i2c_wait_for_transmit();
  if(error != NULL) {
    *error = (status == TW_MR_SLA_ACK) ? I2C_Error_None : I2C_Error_Unknown;
  }
}

void i2c_read_register(uint8_t *data, uint8_t n_bytes, uint8_t slave_addr,
  uint8_t reg_addr, bool end_with_nack, I2C_Error *error)
{
  I2C_Error _error = I2C_Error_None;

  i2c_start(&_error);
  if(_error != I2C_Error_None) goto finish;
  i2c_write_sla(slave_addr, &_error);
  if(_error != I2C_Error_None) goto finish;
  i2c_write_byte(reg_addr, &_error);
  if(_error != I2C_Error_None) goto finish;
  i2c_start(&_error);
  if(_error != I2C_Error_None) goto finish;
  i2c_read_sla(slave_addr, &_error);
  if(_error != I2C_Error_None) goto finish;
  for(int i = 0; i < n_bytes; ++i) {
    data[i] = i2c_read_byte(end_with_nack && i == n_bytes - 1, &_error);
    if(_error != I2C_Error_None) goto finish;
  }

finish:
  if(error != NULL) *error = _error;
  i2c_stop();
}

void i2c_write_register(uint8_t *data, uint8_t n_bytes, uint8_t slave_addr,
  uint8_t reg_addr, bool resend_addr, I2C_Error *error)
{
  I2C_Error _error;

  i2c_start(&_error);
  if(_error != I2C_Error_None) goto finish;
  i2c_write_sla(slave_addr, &_error);
  if(_error != I2C_Error_None) goto finish;
  i2c_write_byte(reg_addr, &_error);
  if(_error != I2C_Error_None) goto finish;
  if(resend_addr) {
    i2c_start(&_error);
    if(_error != I2C_Error_None) goto finish;
    i2c_write_sla(slave_addr, &_error);
    if(_error != I2C_Error_None) goto finish;
  }
  for(int i = 0; i < n_bytes; ++i) {
    i2c_write_byte(data[i], &_error);
    if(_error != I2C_Error_None) goto finish;
  }

finish:
  if(error != NULL) *error = _error;
  i2c_stop();
}
