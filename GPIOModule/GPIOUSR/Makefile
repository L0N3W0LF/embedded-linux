CC = /opt/fontys/soekris/bin/i586-linux-gcc
CFLAGS = -O2 -Wall -Werror
SOURCES = gpio.c
TARGETS = gpio

all:
	@$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(TARGETS)

clean:
	@rm -rf $(TARGETS) $(OBJECTS)