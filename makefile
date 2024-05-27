CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = myshell

all: $(TARGET)

$(TARGET): myshell.o
	$(CC) $(CFLAGS) -o $(TARGET) myshell.o

myshell.o: myshell.c
	$(CC) $(CFLAGS) -c myshell.c

clean:
	rm -f *.o $(TARGET)
