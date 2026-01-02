CC = gcc
CFLAGS = -g -Wall `sdl2-config --cflags` -I.
LDLIBS = `sdl2-config --libs` -lSDL2_ttf -lSDL2_gfx -lconfig -lm -lpthread -lzmq -lprotobuf-c

PROTO_SRC = messages.proto
PROTO_GEN = messages.pb-c.c messages.pb-c.h

SERVER_SRCS = new-universe-server.c display.c universe-data.c physics-rules.c communication.c messages.pb-c.c
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
SERVER_TARGET = new-universe-server

CLIENT_SRCS = new-trashship-client.c communication.c messages.pb-c.c display.c universe-data.c
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)
CLIENT_TARGET = new-trashship-client

TARGETS = $(SERVER_TARGET) $(CLIENT_TARGET)

all: $(TARGETS)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS) $(LDLIBS)

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

messages.pb-c.c messages.pb-c.h: $(PROTO_SRC)
	protoc-c --c_out=. $(PROTO_SRC)

communication.o: messages.pb-c.h
new-universe-server.o: messages.pb-c.h
new-trashship-client.o: messages.pb-c.h

clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(TARGETS) $(PROTO_GEN)