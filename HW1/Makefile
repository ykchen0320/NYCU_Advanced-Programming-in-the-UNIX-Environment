CC = gcc
all:logger.so logger
	
logger:logger.c
	$(CC) -o $@ -Wall -g $<

logger.so:loggerso.c
	$(CC) -o $@ -shared -fPIC $^ -ldl

clean:
	rm -rf logger.so logger