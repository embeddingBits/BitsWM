# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -I/usr/include/freetype2
LDFLAGS = -lX11 -lXft

# Target executable
TARGET = bitswm

# Source and object files
SOURCES = bitswm.c client.c utils.c
OBJECTS = $(SOURCES:.c=.o)

# Header files
HEADERS = bitswm.h config.h 

# Default target
all: $(TARGET)

# Link object files into executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files into object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install the executable (default: /usr/local/bin)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Phony targets
.PHONY: all clean install uninstall
