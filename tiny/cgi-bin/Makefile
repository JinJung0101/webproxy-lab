CC = gcc
CFLAGS = -O2 -Wall -I ..

all: new_adder

new_adder: new_adder.c
	$(CC) $(CFLAGS) -o new_adder new_adder.c  
adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c

clean:
	rm -f adder *~
	rm -f new_adder *~
