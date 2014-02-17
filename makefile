all: http_server clean

clean:
	rm *.o

http_server: http_server.o circular_int_queue.o
	gcc -g -o http_server http_server.o circular_int_bounded_buffer.o -lpthread

http_server.o: http_server.c
	gcc -c -g -W -Wall http_server.c

circular_int_queue.o: circular_int_bounded_buffer.c circular_int_queue.h
	gcc -c -g -W circular_int_bounded_buffer.c

