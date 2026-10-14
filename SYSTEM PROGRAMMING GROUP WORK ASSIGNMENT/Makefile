# Makefile — Smart City Traffic Light Data System (ICS 2305, Question 2)
# Build with mingw32-make (or plain `make` in a shell that has it).
# Fallback one-liner:
#   gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -g main.c sensor.c gc.c -o traffic.exe

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Werror -g
BIN     = traffic.exe
SRC     = main.c sensor.c gc.c
OBJ     = $(SRC:.c=.o)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

main.o: main.c traffic.h
	$(CC) $(CFLAGS) -c main.c -o main.o

sensor.o: sensor.c traffic.h
	$(CC) $(CFLAGS) -c sensor.c -o sensor.o

gc.o: gc.c traffic.h
	$(CC) $(CFLAGS) -c gc.c -o gc.o

clean:
	del /Q $(OBJ) $(BIN) 2>NUL

.PHONY: all clean
