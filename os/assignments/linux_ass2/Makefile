
CC=$(CROSS_COMPILE)gcc
CFLAGS= -c -g -Wall -I./
LDFLAGS= -lpthread -lrt

FILE_NAME= ass_2_3

SOURCES= $(FILE_NAME).c
OBJECTS=$(SOURCES:.c=.o)
TARGET= $(FILE_NAME).bin

all: $(TARGET)

$(TARGET): $(SOURCES) $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.c.o: 
	$(CC) $(CFLAGS) $< -o $@
	
clean:
	$(RM) *.o *.bin