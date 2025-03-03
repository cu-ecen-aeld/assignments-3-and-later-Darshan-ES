CC?=gcc
CC_CROSS ?= $(CROSS_COMPILE)gcc
CFLAGS?=-Wall -pthread
TARGET?=aesdsocket
SRC_File ?=aesdsocket.c
OBJ_File?=$(SRC_File :.c=.o)
LDFLAGS ?= -lpthread -lrt -Wl,--hash-style=gnu 
# Build Application
all: $(TARGET)

# target Build
$(TARGET): $(SRC_File)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC_File)

cross-compile: $(SRC_File)
	$(CC_CROSS) $(CFLAGS) -o $(TARGET) $(SRC_File)
	
#clean	
clean:
	rm -f $(TARGET) $(OBJ)
	
# Phony targets
.PHONY: all clean 

