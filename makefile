# Kompilator i flagi
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99

# Nazwy plików wykonywalnych
TARGETS = kasjer klient main ratownik manager

# Pliki źródłowe i nagłówkowe
COMMON_HEADER = common.h
SRC_FILES = $(addsuffix .c, $(TARGETS))

# Reguła domyślna
all: $(TARGETS)

# Automatyczna reguła dla każdego celu
%: %.c $(COMMON_HEADER)
	$(CC) $(CFLAGS) -o $@ $<

# Czyszczenie plików wykonywalnych
clean:
	rm -f $(TARGETS)

# Reguła phony (dla all i clean, by uniknąć konfliktu z plikami o tych nazwach)
.PHONY: all clean
