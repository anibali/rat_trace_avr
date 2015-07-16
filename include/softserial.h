#ifndef __SOFTSERIAL_H__
#define __SOFTSERIAL_H__

#include <stdint.h>
#include <stdbool.h>

void softserial_init();

void softserial_putc(char c);
void softserial_puts(const char *str);
int softserial_printf(const char* format, ...);

int softserial_available();
void softserial_clear_buffer();
char softserial_getc();
bool softserial_getc_timeout(char *c, const int timeout_ms);
int softserial_getsn(char* s, int n);
int softserial_readline(char *s, int n);

// For debugging
void softserial_dump();

#endif
