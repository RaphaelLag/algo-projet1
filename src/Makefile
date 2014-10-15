CC=gcc -g -Wall 

all: altex

altex: altex.c altex-utilitaire.o
	$(CC) `pkg-config --cflags --libs cairo freetype2` -lm -o altex altex.c altex-utilitaire.o

altex-utilitaire.o: altex-utilitaire.c altex-utilitaire.h
	$(CC) `pkg-config --cflags --libs cairo freetype2` -c altex-utilitaire.c

clean:
	rm altex-utilitaire.o altex

