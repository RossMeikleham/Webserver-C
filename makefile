all: client_d client_t t_dt_server clean

clean:
	rm *.o

client_d: client_d.o
	gcc -o client_d client_d.o

client_t: client_t.o
	gcc -o client_t client_t.o
   	
t_dt_server: t_dt_server.o
	gcc -o t_dt_server t_dt_server.o -lpthread

client_d.o: client_d.c
	gcc -c -W -Wall client_d.c

t_dt_server.o: t_dt_server.c
	gcc -c -W -Wall t_dt_server.c

client_t.o: client_t.c
	gcc -c -W -Wall client_t.c
  
