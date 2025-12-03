all: Universe-client Universe-server

Universe-client: Universe-client.o Communication.o
	gcc -g Universe-client.o Communication.o -o Universe-client -lSDL2 -lzmq

Universe-server: Universe-server.o Communication.o Universe-data.o Display.o
	gcc -g Universe-server.o Communication.o Universe-data.o Display.o -o Universe-server -lSDL2 -lzmq -lconfig -lSDL2_gfx

Universe-client.o: Universe-client.c Communication.h
	gcc -g -c Universe-client.c -o Universe-client.o

Universe-server.o: Universe-server.c Communication.h Universe-data.h Display.h
	gcc -g -c Universe-server.c -o Universe-server.o

Communication.o: Communication.c Communication.h
	gcc -g -c Communication.c -o Communication.o

Display.o: Display.c Display.h Universe-data.h
	gcc -g -c Display.c -o Display.o

Universe-data.o: Universe-data.c Universe-data.h
	gcc -g -c Universe-data.c -o Universe-data.o

clean:
	rm -f Universe-client Universe-server *.o
