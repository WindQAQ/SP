CC = gcc
CFLAGS = -std=c99 -O2
TARGET = bidding_system host player player_bonus

all: $(TARGET)

%: %.c
	$(CC) $< -o $@ $(CFLAGS)

clean:
	rm bidding_system host player player_bonus -rf