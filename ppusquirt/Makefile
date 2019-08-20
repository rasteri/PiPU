CC=gcc
CFLAGS=-O3 -Wall

pipu: main.o frameprocess.o ppusquirt.o palmus.o
	$(CC) -o ppusquirt main.o ppusquirt.o frameprocess.o -lrt -lusb-1.0 -O3 -lpthread
	$(CC) -o palmus palmus.o -lrt -O3 -lpthread
clean: 
	rm *.o ppusquirt palmus
