CC=gcc
LD=ld
SRC=./src
BIN=./bin

debug:
	$(CC) $(SRC)/guest.c -o $(BIN)/guest -g
	$(CC) $(SRC)/host.c -o $(BIN)/host -g
