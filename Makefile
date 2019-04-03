all: prog

prog: main.c framework.c coroutine.c fiber.c redis.c amd64/fiber.o
	gcc -std=gnu99 -o prog main.c framework.c coroutine.c fiber.c redis.c amd64/fiber.o

amd64/fiber.o: amd64/fiber.s
	as --64 -o amd64/fiber.o amd64/fiber.s

clean:
	rm -f prog amd64/fiber.o
