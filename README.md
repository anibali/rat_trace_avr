This project contains a basic framework of useful services for the Arduino Uno.

The point of writing it in AVR C like this is to hopefully have it work on
AVR chips, not just the Arduino Uno.

## Building and running

### Dependencies

* make
* binutils-avr
* avr-libc
* avrdude
* gcc-avr

### Compiling

    make

The default `make` task builds an Intel HEX file, `led_blink.hex`, which can be
uploaded to the Arduino Uno.

### Uploading

    sudo make upload

Flashes `led_blink.hex` to the Arduino Uno, which is assumed to be connected
and available on `/dev/ttyACM0`.

### Watching

A status LED should be visibly blinking on the Arduino. However, this program
also sends an encrypted message via the USB serial connection every time the
LED blinks.

To decrypt and display the message, you need to run the server program while
the Arduino Uno is connected and available on `/dev/ttyACM0`:

1. `cd server`
2. `./server.sh`

You should be able to see output from the program running on the Arduino
being streamed (the string "Blink!" is printed each time the LED blinks).

Note: Make sure you terminate the "server" program before running
`sudo make upload`.

## Boring copyright stuffs

The aes-gcm code comes from http://w1.fi/ and is distributed under the BSD
license. I didn't write the encryption code (thank Jouni Malinen for that), I
just stripped it back and made it work on the Arduino.

## References

* http://www.appelsiini.net/2011/simple-usart-with-avr-libc
* https://balau82.wordpress.com/2011/03/29/programming-arduino-uno-in-pure-c/
