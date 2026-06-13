.PHONY: milestone1 milestone2 milestone3 milestone4 milestone5 clean

CC = gcc
CFLAGS = -Wall -std=c99

milestone1:
	$(CC) $(CFLAGS) milestone1/main.c milestone1/graph.c -o milestone1/dijkstra -lm

milestone2:
	$(CC) $(CFLAGS) milestone2/main.c milestone2/graph.c milestone2/draw.c -o milestone2/sim -lraylib -lm -lpthread -ldl -lrt -lX11

milestone3:
	$(CC) $(CFLAGS) milestone3/main.c milestone3/graph.c milestone3/draw.c milestone3/animate.c -o milestone3/sim -lraylib -lm -lpthread -ldl -lrt -lX11

milestone4:
	$(CC) $(CFLAGS) milestone4/main.c milestone4/graph.c milestone4/draw.c milestone4/animate.c -o milestone4/sim -lraylib -lm -lpthread -ldl -lrt -lX11

milestone5:
	$(CC) $(CFLAGS) milestone5/main.c milestone5/graph.c milestone5/draw.c milestone5/animate.c -o milestone5/sim -lraylib -lm -lpthread -ldl -lrt -lX11

clean:
	rm -f milestone1/dijkstra milestone2/sim milestone3/sim milestone4/sim milestone5/sim
