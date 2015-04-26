#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "serial.h"

int main() {
  int fd = serial_init("/dev/ttyACM0");
  if(fd < 0) return 1;

  const int LINE_LENGTH = 1024;
  char line[LINE_LENGTH];
  int line_pos = 0;
  char input;
  while(1) {
    int n = read(fd, &input, 1);

    if(n == 1) {
      if(input == '\n') {
        line[line_pos] = '\0'; // Null-terminate the string
        printf("%s\n", line); // Echo line
        line_pos = 0;

        /*if(strcmp(line, "AT+RST\n")) {
          const char *reply = "Wow\n";
          write(fd, reply, 4);
        }*/
      } else {
        if(line_pos < LINE_LENGTH - 1) {
          line[line_pos++] = input;
        } else {
          printf("WARN: Line length too small\n");
        }
      }
    }
  }

  return 0;
}
