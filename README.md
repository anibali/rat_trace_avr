## Building and running

### Dependencies

* make
* binutils-avr
* avr-libc
* avrdude
* gcc-avr

### Compiling

    make

The default `make` task builds an Intel HEX file, which can be
uploaded to the Arduino Uno.

### Uploading

    sudo make upload

Flashes the HEX file to the Arduino Uno, which is assumed to be connected
and available on `/dev/ttyACM0`.

### Watching

To display debug messages, you need to run the server program while
the Arduino Uno is connected and available on `/dev/ttyACM0`:

1. `cd server`
2. `./server.sh`

Note: Make sure you terminate the "server" program before running
`sudo make upload`.

## References

* http://www.appelsiini.net/2011/simple-usart-with-avr-libc
* https://balau82.wordpress.com/2011/03/29/programming-arduino-uno-in-pure-c/
