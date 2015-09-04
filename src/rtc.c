#include "rtc.h"

#include "i2c.h"

#define DS1372_Address 0xD0

typedef enum {
  DS1372_Reg_Clock0 = 0x00,
  DS1372_Reg_Clock1,
  DS1372_Reg_Clock2,
  DS1372_Reg_Clock3,
  DS1372_Reg_Alarm0,
  DS1372_Reg_Alarm1,
  DS1372_Reg_Alarm2,
  DS1372_Reg_Control,
  DS1372_Reg_Status,
  DS1372_Reg_ID0,
  DS1372_Reg_ID1,
  DS1372_Reg_ID2,
  DS1372_Reg_ID3,
  DS1372_Reg_ID4,
  DS1372_Reg_ID5,
  DS1372_Reg_ID6,
  DS1372_Reg_ID7
} DS1372_Reg;

void rtc_init() {
  i2c_init();

  rtc_write_control(0x0E);
  rtc_write_status(0x00);
}

void rtc_read_id(uint64_t *id) {
  // TODO: Error handling
  i2c_read_register((uint8_t*)id, 8, DS1372_Address,
    DS1372_Reg_ID0, true, NULL);
}

uint32_t rtc_read_seconds() {
  uint32_t seconds = -1;

  // TODO: Error handling
  i2c_read_register((uint8_t*)&seconds, 4, DS1372_Address,
    DS1372_Reg_Clock0, true, NULL);

  return seconds;
}

void rtc_write_seconds(uint32_t seconds) {
  // TODO: Error handling
  i2c_write_register((uint8_t*)&seconds, 4, DS1372_Address,
    DS1372_Reg_Clock0, NULL);
}

uint8_t rtc_read_status() {
  // TODO: Error handling
  uint8_t data;
  i2c_read_register(&data, 1, DS1372_Address,
    DS1372_Reg_Status, true, NULL);
  return data;
}

void rtc_write_status(uint8_t status) {
  // TODO: Error handling
  i2c_write_register(&status, 1, DS1372_Address,
    DS1372_Reg_Status, NULL);
}

uint8_t rtc_read_control() {
  // TODO: Error handling
  uint8_t data;
  i2c_read_register(&data, 1, DS1372_Address,
    DS1372_Reg_Control, true, NULL);
  return data;
}

void rtc_write_control(uint8_t control) {
  // TODO: Error handling
  i2c_write_register(&control, 1, DS1372_Address,
    DS1372_Reg_Control, NULL);
}
