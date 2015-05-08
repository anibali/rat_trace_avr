#ifndef __SOFTSERIAL_H__
#define __SOFTSERIAL_H__

void softserial_init();

void softserial_putc(char c);
void softserial_puts(const char *str);
int softserial_printf(const char* format, ...);

int softserial_available();
char softserial_getc();
int softserial_getsn(char* s, int n);

// For debugging
void softserial_dump();

#endif
