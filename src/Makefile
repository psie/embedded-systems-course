PROGRAMS = led_blink.hex test_analog.hex test_eeprom.hex test_serial.hex \
	   test_spi.hex test_temp_sensor.hex test_timer0.hex test_dhcp.hex

all: libs $(PROGRAMS)

include Makefile.common
-include $(subst .cpp,.d,$(wildcard *.cpp))

led_blink.elf: led_blink.o hal/libhal.a
test_analog.elf: test_analog.o hal/libhal.a
test_eeprom.elf: test_eeprom.o hal/libhal.a
test_serial.elf: test_serial.o hal/libhal.a
test_spi.elf: test_spi.o hal/libhal.a
test_temp_sensor.elf: test_temp_sensor.o hal/libhal.a
test_timer0.elf: test_timer0.o hal/libhal.a
test_dhcp.elf: test_dhcp.o hal/libhal.a ethernet/libether.a

CPPFLAGS += -Ibase -Ihal -Iethernet
LDFLAGS += -Lbase -Lhal -Lethernet

libs:
	make -C base
	make -C hal
	make -C ethernet

clean::
	make -C base clean
	make -C hal clean
	make -C ethernet clean

.PHONY: all libs
