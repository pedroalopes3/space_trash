all: Universe-client Universe-server

Universe-client: Universe-client.o Communication.o messages.pb-c.o
	gcc -g Universe-client.o Communication.o messages.pb-c.o -o Universe-client -lSDL2 -lzmq -lprotobuf-c

Universe-server: Universe-server.o Communication.o Display.o Universe-data.o messages.pb-c.o
	gcc -g Universe-server.o Communication.o Display.o Universe-data.o messages.pb-c.o -o Universe-server -lSDL2 -lzmq -lSDL2_gfx -lconfig -lm -lprotobuf-c

Universe-client.o: Universe-client.c Communication.h messages.pb-c.h
	gcc -g -c Universe-client.c -o Universe-client.o

Universe-server.o: Universe-server.c Communication.h messages.pb-c.h
	gcc -g -c Universe-server.c -o Universe-server.o

Communication.o: Communication.c Communication.h
	gcc -g -c Communication.c -o Communication.o

messages.pb-c.c messages.pb-c.h: messages.proto
	protoc-c messages.proto --c_out=.

messages.pb-c.o: messages.pb-c.c messages.pb-c.h
	gcc -g -c messages.pb-c.c -o messages.pb-c.o

clean:
	rm -f Universe-client Universe-server *.o messages.pb-c.c messages.pb-c.h

