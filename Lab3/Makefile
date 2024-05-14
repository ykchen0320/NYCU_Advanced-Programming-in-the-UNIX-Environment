CC = gcc
all: libmaze.so maze libsolver.so

libmaze.so: libmaze_dummy.c
	$(CC) -o $@ -shared -fPIC $^ -ldl

maze: maze.c libmaze.so
	$(CC) -o $@ -Wall -g $< -L./ -Wl,-rpath=./ -lmaze

libsolver.so: libsolver.c
	$(CC) -o $@ -shared -fPIC $^ -ldl
	
clean:
	rm -rf libmaze.so maze libsolver.so