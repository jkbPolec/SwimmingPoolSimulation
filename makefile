# Kompilator i flagi
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99

# Nazwy plików wykonywalnych
TARGETS = kasjer klient main

# Pliki źródłowe i nagłówkowe
COMMON_HEADER = common.h
KASJER_SRC = kasjer.c
KLIENT_SRC = klient.c
MAIN_SRC = main.c

# Reguła domyślna
all: $(TARGETS)

# Kompilacja main
main: $(MAIN_SRC) $(COMMON_HEADER)
	$(CC) $(CFLAGS) -o $@ main.c

# Kompilacja kasjera
kasjer: $(KASJER_SRC) $(COMMON_HEADER)
	$(CC) $(CFLAGS) -o $@ $(KASJER_SRC)

# Kompilacja klienta
klient: $(KLIENT_SRC) $(COMMON_HEADER)
	$(CC) $(CFLAGS) -o $@ $(KLIENT_SRC)

# Czyszczenie plików wykonywalnych
clean:
	rm -f $(TARGETS)

# Reguła phony (dla all i clean, by uniknąć konfliktu z plikami o tych nazwach)
.PHONY: all clean
