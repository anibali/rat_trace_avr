CC = avr-gcc
CFLAGS = -Os -std=c99 -DF_CPU=8000000UL -mmcu=atmega328p
CXX = avr-g++
CXXFLAGS = -Os -DF_CPU=8000000UL -mmcu=atmega328p
OBJDIR = build
SRCDIR = src
INCLUDEDIR = include
vpath %.c $(SRCDIR)
vpath %.h $(INCLUDEDIR)
BINNAME = arduino
OBJS = $(addprefix $(OBJDIR)/, util.o uart.o wifi.o softserial.o adc.o pin.o \
	i2c.o rtc.o)

default: main clean

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	@mkdir -p $@

main: $(OBJS) $(OBJDIR)/main.o
	$(CC) $(CFLAGS) $(OBJS) $(OBJDIR)/main.o -o $(OBJDIR)/$(BINNAME)
	avr-objcopy -O ihex -R .eeprom $(OBJDIR)/$(BINNAME) $(BINNAME).hex

upload:
	avrdude -F -V -c usbasp -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:$(BINNAME).hex

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -I$(INCLUDEDIR) -c -o $@ $<

clean:
	-rm -rf $(OBJDIR)

.force:
