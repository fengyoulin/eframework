all: prog

prog: main.c framework.c coroutine.c fiber.c amd64/fiber.o
	gcc -std=gnu99 -o prog main.c framework.c coroutine.c fiber.c amd64/fiber.o

amd64/fiber.o: amd64/fiber.asm
	nasm -f elf64 -o amd64/fiber.o amd64/fiber.asm

clean:
	rm -f prog amd64/fiber.o
