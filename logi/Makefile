#Compiler to use
CC=g++
#Compiler Flags
#Linker Flags
CFLAGS = -c
LDFLAGS= -ljpeg -lrt -llogi


all: image_demo

image_demo: image_demo.o jpeg_func.o
	$(CC) -o image_demo image_demo.o jpeg_func.o $(LDFLAGS) 

image_demo.o: image_demo.cpp
	$(CC) $(CFLAGS) image_demo.cpp

jpeg_func.o: jpeg_func.cpp
	$(CC) $(CFLAGS) jpeg_func.cpp

clean:
	rm -rf *.o image_demo
