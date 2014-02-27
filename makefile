CFLAGS = -g -W -Wall
SRCS = http_server.c 
OBJECTS = $(SRCS:.c=.o)
LIBS = -lpthread

all: http_server clean

clean:
	rm *.o

http_server: $(OBJECTS)
	gcc $(CFLAGS) -o http_server $(OBJECTS) $(LIBS)

$(OBJECTS): $(SRCS) 
	gcc -c $(CFLAGS) $(SRCS) 

