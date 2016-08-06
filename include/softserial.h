#ifndef __SOFTSERIAL_H__
#define __SOFTSERIAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

extern FILE softserial_output;
extern FILE softserial_input;

void softserial_init();

void softserial_putc(char c);
char softserial_getc();
int softserial_available();

#endif
