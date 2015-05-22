# Linpop-Server makefile

OBJS = main.o server.o
CFLAGS = `pkg-config --cflags glib-2.0`
LIBS = -lpthread `pkg-config --libs glib-2.0`
CC = gcc
EXEC = server

all: $(EXEC)
	
$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(CFLAGS) $(LIBS)
	rm -f *.o

clean: all
	rm -f $(EXEC) *.o

# end of makefile
