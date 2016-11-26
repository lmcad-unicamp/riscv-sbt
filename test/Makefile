CC := riscv64-unknown-elf-gcc
CFLAGS := -Wall -Werror -O2

all: hello hello64

hello.o: hello.c
	$(CC) -m32 -c $(CFLAGS) -o$@ $<

hello: hello.o
	$(CC) -m32 -o$@ $<

hello64.o: hello.c
	$(CC) -c $(CFLAGS) -o$@ $<

hello64: hello64.o
	$(CC) -o$@ $<

clean:
	rm -f hello hello.o hello64 hello64.o
