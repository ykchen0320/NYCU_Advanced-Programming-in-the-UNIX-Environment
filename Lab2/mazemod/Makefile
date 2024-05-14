
obj-m     += maze.o
ccflags-y += -DEXPORT_SYMTAB

all: 
	make -C ../dist/modulebuild M=$(PWD) modules

clean:
	rm -f maze
	make -C ../dist/modulebuild M=$(PWD) clean

install: all
	mkdir -p ../rootfs/modules
	cp maze.h md5.txt mazetest maze.ko ../rootfs/modules
