all: prog

prog: main.c framework.c coroutine.c fiber.c redis.c http.c basic/basic.c fastcgi.c structure/bufferio.c structure/hashtable.c structure/stringtab.c structure/headertab.c structure/buffer.c structure/string.c structure/vector.c amd64/fiber.o
	gcc -g -std=gnu99 -o prog main.c framework.c coroutine.c fiber.c redis.c http.c basic/basic.c fastcgi.c structure/bufferio.c structure/hashtable.c structure/stringtab.c structure/headertab.c structure/buffer.c structure/string.c structure/vector.c amd64/fiber.o

amd64/fiber.o: amd64/fiber.s
	as -g --64 -o amd64/fiber.o amd64/fiber.s

clean:
	rm -f prog amd64/fiber.o
