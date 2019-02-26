CC = g++
CFLAGS = -std=c++11 -I$(IDIR) -g

IDIR = ./include/
SRCDIR = ./src/

SOURCES = $(SRCDIR)*.cpp\

all: Gsql

Gsql: $(SOURCES)
	$(CC) $(SOURCES) $(CFLAGS) -o $@ -lstdc++fs

run:
	./Gsql

clean:
	rm Gsql
