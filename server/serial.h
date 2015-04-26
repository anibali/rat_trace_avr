#ifndef __SERIAL_H__
#define __SERIAL_H__

int set_interface_attribs(int fd, int speed, int parity);
void set_blocking(int fd, int should_block);
int serial_init(const char *device_name);

#endif
