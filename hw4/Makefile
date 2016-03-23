CC = gcc
CFLAG = -O2 -lpthread -std=c99
all = merger

merger: merger.c
	$(CC) $< -o $@ $(CFLAG)

clean:
	rm merger -f
