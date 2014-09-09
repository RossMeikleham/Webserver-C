CFLAGS = -W -Wall
SRCS = web_server.c 
OBJECTS = $(SRCS:.c=.o)
LIBS = -lpthread
EXEC = web_server

all: $(EXEC) clean

clean:
	rm *.o

$(EXEC): $(OBJECTS)
	gcc $(CFLAGS) -o $(EXEC) $(OBJECTS) $(LIBS)

$(OBJECTS): $(SRCS) 
	gcc -c $(CFLAGS) $(SRCS) 

