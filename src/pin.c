#include "pin.h"

#include <avr/io.h>

typedef struct {
  uint8_t port_addr;
  uint8_t ddr_addr;
  uint8_t pin_addr;
} Port_Info;

typedef struct {
  Port_Info *port_info;
  uint8_t bv;
} Pin_Info;

typedef enum {
  Port_A = 0,
  Port_B,
  Port_C,
  Port_D
} Port;

static Port_Info port_infos[] = {
  {0},
  {
    .port_addr  = _SFR_IO_ADDR(PORTB),
    .ddr_addr   = _SFR_IO_ADDR(DDRB),
    .pin_addr   = _SFR_IO_ADDR(PINB),
  },
  {
    .port_addr  = _SFR_IO_ADDR(PORTC),
    .ddr_addr   = _SFR_IO_ADDR(DDRC),
    .pin_addr   = _SFR_IO_ADDR(PINC),
  },
  {
    .port_addr  = _SFR_IO_ADDR(PORTD),
    .ddr_addr   = _SFR_IO_ADDR(DDRD),
    .pin_addr   = _SFR_IO_ADDR(PIND),
  }
};

static Pin_Info pins[Number_Of_Pins];

#define REGISTER_PIN(name, port, pin_num) \
  ({pins[(name)].port_info = &port_infos[(port)]; \
    pins[(name)].bv = _BV(pin_num);})

void pin_register_all() {
  REGISTER_PIN(Pin_Status_LED,    Port_B, 5);
  REGISTER_PIN(Pin_IR_Enable,     Port_D, 2);
  REGISTER_PIN(Pin_Softserial_TX, Port_B, 1);
  REGISTER_PIN(Pin_Softserial_RX, Port_B, 0);
}

void pin_set_direction(Pin pin, Pin_Direction direction) {
  if(direction == Direction_Input) {
    _SFR_IO8(pins[pin].port_info->ddr_addr) &= ~pins[pin].bv;
  } else {
    _SFR_IO8(pins[pin].port_info->ddr_addr) |= pins[pin].bv;
  }
}

void pin_digital_write(Pin pin, Logic_Level value) {
  if(value == Logic_Low) {
    _SFR_IO8(pins[pin].port_info->port_addr) &= ~pins[pin].bv;
  } else {
    _SFR_IO8(pins[pin].port_info->port_addr) |= pins[pin].bv;
  }
}

void pin_digital_toggle(Pin pin) {
  _SFR_IO8(pins[pin].port_info->port_addr) ^= pins[pin].bv;
}

Logic_Level pin_digital_read(Pin pin) {
  if(_SFR_IO8(pins[pin].port_info->pin_addr) & pins[pin].bv)
    return Logic_High;
  else
    return Logic_Low;
}
