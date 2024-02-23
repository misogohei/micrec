
SRC=micrec.c
CC=cc
LDFLAGS=-lasound -lmp3lame
CFLAGS=-g -Wall -pedantic
# CFLAGS=-O2 -Wall

micrec: $(SRC)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f micrec out.mp3

test: micrec
	./micrec plughw:CARD=Device,DEV=0 5
