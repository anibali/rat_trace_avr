#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>
#include <util/delay.h>

// http://www.appelsiini.net/2011/simple-usart-with-avr-libc

void uart_putchar(char c, FILE *stream);
char uart_getchar(FILE *stream);
int uart_getchar_timeout(FILE *stream, char *c);

void uart_init();

extern FILE uart_output;
extern FILE uart_input;

#endif
