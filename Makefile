#######################
# Author: Ivan        #
#######################

#Compiler to use
CC=g++

#Compiler Flags
CFLAGS = -c

#Linker 
LDFLAGS= -ljpeg -lrt -llogi

all: cfpga

cfpga: main.o sobel.o gauss.o hyst.o erode.o dilate.o output.o jpeg_func.o
	$(CC) -o cfpga main.o sobel.o gauss.o hyst.o erode.o dilate.o output.o jpeg_func.o $(LDFLAGS)

#libs

main.o: main.cpp
	$(CC) $(CFLAGS) main.cpp

sobel.o: sobel.cpp
	$(CC) $(CFLAGS) sobel.cpp

gauss.o: gauss.cpp
	$(CC) $(CFLAGS) gauss.cpp

hyst.o: hyst.cpp
	$(CC) $(CFLAGS) hyst.cpp

erode.o: erode.cpp
	$(CC) $(CFLAGS) erode.cpp

dilate.o: dilate.cpp
	$(CC) $(CFLAGS) dilate.cpp

output.o: output.cpp
	$(CC) $(CFLAGS) output.cpp

#logi related

jpeg_func.o: jpeg_func.cpp
	$(CC) $(CFLAGS) jpeg_func.cpp

#clean

clean:
	rm *.o cfpga