#include "pin.h"

#include <avr/io.h>
#include <avr/interrupt.h>

typedef struct {
  uint8_t port_addr;
  uint8_t ddr_addr;
  uint8_t pin_addr;
  uint8_t pcmsk_addr;
} Port_Info;

typedef struct {
  Port_Info *port_info;
  uint8_t bv;
} Pin_Info;

typedef enum {
  //Port_A,
  Port_B,
  Port_C,
  Port_D
} Port;

static Port_Info port_infos[] = {
  //{0},
  {
    .port_addr  = _SFR_IO_ADDR(PORTB),
    .ddr_addr   = _SFR_IO_ADDR(DDRB),
    .pin_addr   = _SFR_IO_ADDR(PINB),
    .pcmsk_addr = _SFR_MEM_ADDR(PCMSK0),
  },
  {
    .port_addr  = _SFR_IO_ADDR(PORTC),
    .ddr_addr   = _SFR_IO_ADDR(DDRC),
    .pin_addr   = _SFR_IO_ADDR(PINC),
    .pcmsk_addr = _SFR_MEM_ADDR(PCMSK1),
  },
  {
    .port_addr  = _SFR_IO_ADDR(PORTD),
    .ddr_addr   = _SFR_IO_ADDR(DDRD),
    .pin_addr   = _SFR_IO_ADDR(PIND),
    .pcmsk_addr = _SFR_MEM_ADDR(PCMSK2),
  }
};

static Pin_Info pins[Number_Of_Pins];

#define REGISTER_PIN(name, port, pin_num) \
  ({pins[(name)].port_info = &port_infos[(port)]; \
    pins[(name)].bv = _BV(pin_num);})

void pin_register_all() {
  REGISTER_PIN(Pin_Status_LED,    Port_C, 3);
  REGISTER_PIN(Pin_Wifi_Enable,   Port_D, 6);
  REGISTER_PIN(Pin_Battery_Test_Enable, Port_D, 4);

  // NOTE: May require change to ISR vector in softserial.c
  REGISTER_PIN(Pin_Softserial_TX, Port_B, 2);
  REGISTER_PIN(Pin_Softserial_RX, Port_B, 1);

  REGISTER_PIN(Pin_Debug_Out, Port_C, 2);

  // TODO: Check whether enabling more bits in PCICR than needed wastes power
  PCICR |= _BV(PCIE0) | _BV(PCIE1) | _BV(PCIE2);
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

void pin_enable_interrupt(Pin pin) {
  _SFR_MEM8(pins[pin].port_info->pcmsk_addr) |= pins[pin].bv;
}

void pin_disable_interrupt(Pin pin) {
  _SFR_MEM8(pins[pin].port_info->pcmsk_addr) &= ~pins[pin].bv;
}
