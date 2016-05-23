CC = /opt/fontys/soekris/bin/i586-linux-gcc
CFLAGS = -O2 -Wall -Werror -I $(HEADERS)
LDFLAGS = -lusb -lrt
HEADERS = ../Includes
SOURCES = gamepadd.c $(HEADERS)/shmgamepad.c
TARGETS = gamepadd

all:
	@$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(TARGETS)

send:
	@nc 10.0.0.42 1337 < $(TARGETS)