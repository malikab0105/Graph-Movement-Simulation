.PHONY: milestone1 milestone2 clean

CC = gcc
CFLAGS = -Wall -std=c99

milestone1:
	$(CC) $(CFLAGS) milestone1/main.c milestone1/graph.c -o milestone1/milestone1 -lm

milestone2:
	$(CC) $(CFLAGS) milestone2/main.c milestone2/graph.c milestone2/draw.c -o milestone2/milestone2 -lraylib -lm -lpthread -ldl -lrt -lX11

clean:
	rm -f milestone1/milestone1 milestone2/milestone2