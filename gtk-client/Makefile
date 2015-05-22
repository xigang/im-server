#makefile

OBJS = lin.o chat_dlg.o client_socket.o linwnd.o treeview.o user_info_set.o
CFLAGS = `pkg-config gtk+-2.0 --cflags`
LIBS = `pkg-config gtk+-2.0 --libs` -lpthread
CC = gcc
EXEC = client

all: $(EXEC)
	
$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(CFLAGS) $(LIBS)
	rm -f *.o

clean: all
	rm -f $(EXEC) *.o

# end of makefile
