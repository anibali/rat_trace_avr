#include "pin.h"

#include <avr/io.h>

typedef struct {
  uint8_t port_addr;
  uint8_t port_bv;
  uint8_t ddr_addr;
  uint8_t ddr_bv;
  uint8_t pin_addr;
  uint8_t pin_bv;
} Pin_Info;

Pin_Info pins[Number_Of_Pins];

void pin_init() {
  pins[Pin_Status_LED].port_addr = _SFR_IO_ADDR(PORTB);
  pins[Pin_Status_LED].port_bv = _BV(PORTB5);
  pins[Pin_Status_LED].ddr_addr = _SFR_IO_ADDR(DDRB);
  pins[Pin_Status_LED].ddr_bv = _BV(DDB5);
  pins[Pin_Status_LED].pin_addr = _SFR_IO_ADDR(PINB);
  pins[Pin_Status_LED].pin_bv = _BV(PINB5);

  pins[Pin_IR_Enable].port_addr = _SFR_IO_ADDR(PORTD);
  pins[Pin_IR_Enable].port_bv = _BV(PORTD2);
  pins[Pin_IR_Enable].ddr_addr = _SFR_IO_ADDR(DDRD);
  pins[Pin_IR_Enable].ddr_bv = _BV(DDD2);
  pins[Pin_IR_Enable].pin_addr = _SFR_IO_ADDR(PIND);
  pins[Pin_IR_Enable].pin_bv = _BV(PIND2);

  pins[Pin_Softserial_TX].port_addr = _SFR_IO_ADDR(PORTB);
  pins[Pin_Softserial_TX].port_bv = _BV(PORTB5);
  pins[Pin_Softserial_TX].ddr_addr = _SFR_IO_ADDR(DDRB);
  pins[Pin_Softserial_TX].ddr_bv = _BV(DDB5);
  pins[Pin_Softserial_TX].pin_addr = _SFR_IO_ADDR(PINB);
  pins[Pin_Softserial_TX].pin_bv = _BV(PINB5);

  pins[Pin_Softserial_RX].port_addr = _SFR_IO_ADDR(PORTB);
  pins[Pin_Softserial_RX].port_bv = _BV(PORTB0);
  pins[Pin_Softserial_RX].ddr_addr = _SFR_IO_ADDR(DDRB);
  pins[Pin_Softserial_RX].ddr_bv = _BV(DDB0);
  pins[Pin_Softserial_RX].pin_addr = _SFR_IO_ADDR(PINB);
  pins[Pin_Softserial_RX].pin_bv = _BV(PINB0);
}

void pin_set_direction(Pin pin, Pin_Direction direction) {
  if(direction == Direction_Input) {
    _SFR_IO8(pins[pin].ddr_addr) &= ~pins[pin].ddr_bv;
  } else {
    _SFR_IO8(pins[pin].ddr_addr) |= pins[pin].ddr_bv;
  }
}

void pin_digital_write(Pin pin, Logic_Level value) {
  if(value == Logic_Low) {
    _SFR_IO8(pins[pin].port_addr) &= ~pins[pin].port_bv;
  } else {
    _SFR_IO8(pins[pin].port_addr) |= pins[pin].port_bv;
  }
}

void pin_digital_toggle(Pin pin) {
  _SFR_IO8(pins[pin].port_addr) ^= pins[pin].port_bv;
}

Logic_Level pin_digital_read(Pin pin) {
  if(_SFR_IO8(pins[pin].pin_addr) & pins[pin].pin_bv)
    return Logic_High;
  else
    return Logic_Low;
}
