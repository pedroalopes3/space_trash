all: config_test

config_test: config_test.c
	gcc config_test.c -o config_test \
	-lconfig++ \
	-I/opt/homebrew/Cellar/libconfig/1.8.1/include \
	-L/opt/homebrew/Cellar/libconfig/1.8.1/lib

clean:
	rm config_test
