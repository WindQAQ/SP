CC = gcc
CFLAGS = -std=gnu99 -O2
TARGET = bidding_system customer bidding_system_EDF

all: $(TARGET)

bidding_system: bidding_system.c
	$(CC) $< -o $@ $(CFLAGS)

customer: customer.c
	$(CC) $< -o $@ $(CFLAGS)

bidding_system_EDF: bidding_system_EDF.c
	$(CC) $< -o $@ $(CFLAGS)

clean:
	rm bidding_system customer bidding_system_EDF -rf
