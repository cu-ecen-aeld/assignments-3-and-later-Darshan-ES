# Variables
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -O2
TARGET = writer
SRC_File = writer.c
OBJ_File = $(SRC_File:.c=.o)
TARGET_DIR = .

# Build Application
all: $(TARGET)
# target Build
$(TARGET): $(OBJ_File)
	$(CC) -o $(TARGET_DIR)/$@ $(OBJ_File) $(CFLAGS)
# Compile object files
%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@ 
# Clean target and remove build files
clean:
	echo "Cleaning up previous generated files..."
	rm -f $(OBJ_File) $(TARGET)
# Phony targets
.PHONY: all clean

