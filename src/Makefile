# Makefile – Compilación del shell WISH
# Autor: José Alfredo Martínez Valdés

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
OBJS = wish.o builtins.o
TARGET = ../bin/wish

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

wish.o: wish.c builtins.h
	$(CC) $(CFLAGS) -c wish.c -o wish.o

builtins.o: builtins.c builtins.h
	$(CC) $(CFLAGS) -c builtins.c -o builtins.o

clean:
	rm -f $(OBJS) $(TARGET)

wish_final: wish_final.c
	gcc -Wall -Wextra -std=c11 -g -o ../bin/wish_final wish_final.c

wish_victory: wish_victory.c
	$(CC) -Wall -Wextra -std=c11 -g -o ../bin/wish_victory wish_victory.c

wish_victory_v2:
	gcc -Wall -Wextra -std=c11 -g -o ../bin/wish_victory_v2 wish_victory_v2.c
