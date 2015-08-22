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

#if 0
void softserial_puts(const char *str);
int softserial_printf(const char* format, ...);
void softserial_clear_buffer();
bool softserial_getc_timeout(char *c, const int timeout_ms);
int softserial_getsn(char* s, int n);
int softserial_readline(char *s, int n);

// For debugging
void softserial_dump();
#endif

#endif
