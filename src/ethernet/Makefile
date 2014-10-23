objects = Wiz5100.o UDP.o ../hal/SPI.o

libether.a: $(objects)

include ../Makefile.common

CPPFLAGS += -I../base -I../hal

-include $(objects:.o=.d)
