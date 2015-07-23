#ifndef __PIN_H__
#define __PIN_H__

typedef enum {
  Logic_Low,
  Logic_High
} Logic_Level;

typedef enum {
  Direction_Input,
  Direction_Output
} Pin_Direction;

typedef enum {
  Pin_Status_LED,
  Pin_IR_Enable,
  Pin_Softserial_TX,
  Pin_Softserial_RX,

  Number_Of_Pins
} Pin;

void pin_init();

void pin_set_direction(Pin pin, Pin_Direction direction);

void pin_digital_write(Pin pin, Logic_Level value);
void pin_digital_toggle(Pin pin);

Logic_Level pin_digital_read(Pin pin);

#endif
