CC = gcc
CFLAGS = -g -Wall `sdl2-config --cflags` -I.
LDLIBS = `sdl2-config --libs` -lSDL2_ttf -lSDL2_gfx -lconfig -lm

SRCS = universe-simulator.c display.c universe-data.c physics-rules.c
OBJS = $(SRCS:.c=.o)
TARGET = universe-simulator

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)