CFLAGS = -g -W -Wall
SRCS = http_server.c http_request.c circular_int_bounded_buffer.c
INCLUDES = request.h circular_int_queue.h
OBJECTS = $(SRCS:.c=.o)
LIBS = -lpthread

all: http_server clean

clean:
	rm *.o

http_server: $(OBJECTS)
	gcc $(CFLAGS) -o http_server $(OBJECTS) $(LIBS)

$(OBJECTS): $(SRCS) $(INCLUDES)
	gcc -c $(CFLAGS) $(SRCS) $(INCLUDES)

