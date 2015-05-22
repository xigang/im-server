#makefile

OBJS = main.o server.o mysql_connect.o
CFLAGS = `pkg-config --cflags glib-2.0` `mysql_config --cflags`
LIBS = -pthread `pkg-config --libs glib-2.0` `mysql_config --libs`
CC = gcc
EXEC = server_DB

all: $(EXEC)
	
$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(CFLAGS) $(LIBS)
	rm -f *.o

clean: all
	rm -f $(EXEC) *.o

# end of makefile
