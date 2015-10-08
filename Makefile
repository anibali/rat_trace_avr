CC = avr-gcc
CFLAGS = -Os -std=c99 -D_DEBUG -DF_CPU=8000000UL -mmcu=atmega328p
CXX = avr-g++
CXXFLAGS = -Os -DF_CPU=8000000UL -mmcu=atmega328p
OBJCOPY = avr-objcopy
OBJDIR = obj
SRCDIR = src
INCLUDEDIR = include
vpath %.c $(SRCDIR)
vpath %.h $(INCLUDEDIR)
BINNAME = arduino
OBJS = $(addprefix $(OBJDIR)/, util.o uart.o wifi.o softserial.o adc.o pin.o \
	i2c.o rtc.o report.o clock.o proximity.o)

default: build clean

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	@mkdir -p $@

build: main
	$(OBJCOPY) -O ihex -R .eeprom $(OBJDIR)/$(BINNAME) $(BINNAME).hex
	$(OBJCOPY) -O ihex -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 $(OBJDIR)/$(BINNAME) $(BINNAME).eep

main: $(OBJS) $(OBJDIR)/main.o
	$(CC) $(CFLAGS) $(OBJS) $(OBJDIR)/main.o -o $(OBJDIR)/$(BINNAME)

upload:
	avrdude -F -V -c usbasp -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:$(BINNAME).hex -U eeprom:w:$(BINNAME).eep

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -I$(INCLUDEDIR) -c -o $@ $<

clean:
	-rm -rf $(OBJDIR)

.force:
