CC = g++
CFLAGS = -std=c++11 -I$(IDIR) -g

IDIR = ./include/
SRCDIR = ./src/

SOURCES = $(SRCDIR)*.cpp\

all: Gsql

Gsql: $(SOURCES)
	$(CC) $(SOURCES) $(CFLAGS) -O -o $@ -lstdc++fs -lreadline

run:
	./Gsql

clean:
	rm Gsql
