
#include <iostream>
#include <string.h>
#include <list>
#include <algorithm>
#include "sobel.h"
#include "gauss.h"
#include "dilate.h"
#include "erode.h"
#include "hyst.h"
#include "output.h"

#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include <linux/ioctl.h>

#include "logi/jpeg_func.h"
#include "logilib.h"

using namespace std;

#define REG_ADDR 0x0800
#define FIFO_CMD_ADDR 0x0200

#define IMAGE_WIDTH 320
#define IMAGE_HEIGHT 240

#define LINE_BURST 1

#define FILTER_SIZE 6

int *filtering_chain_builder(list<Filter*> input_filter);
void print_array(int *array, int size);

int main(int argc, char* argv[]){

	if(argc <= 2){
		cout<<"Missing parameters. Please use the following input notation: "<<endl;
		cout<<"<appname> <image.jpg> <sobel, gauss, erode, dilate, hyst> <sobel, ...>"<<endl;
		return 0;
	}

	// get the input filter from the user
	list<Filter*> input_filter;

	for(int i = 0; i < argc - 2; i++){
		if(strcmp(argv[i + 2], "sobel") == 0)
			input_filter.push_back(new Sobel());
		else if (strcmp(argv[i + 2], "gauss") == 0)
			input_filter.push_back(new Gauss());
		else if (strcmp(argv[i + 2], "erode") == 0)
			input_filter.push_back(new Erode());
		else if (strcmp(argv[i + 2], "dilate") == 0)
			input_filter.push_back(new Dilate());
		else if (strcmp(argv[i + 2], "hyst") == 0)
			input_filter.push_back(new Hyst());
	}

	// create the filtering chain
	int *buffer = filtering_chain_builder(input_filter);
	print_array(buffer, FILTER_SIZE);

	return 0;
}

int logi_call(int argc, char ** argv){

unsigned char * inputImage ;
	long start_time, end_time ;
	double diff_time ;
	struct timespec cpu_time ;
	FILE * jpeg_fd ;
	FILE * raw_file ;
	int i,j, res ;
	unsigned int pos = 0 ;
	
	unsigned char cmd_buffer[4];
	unsigned char reg_buffer[6];
	
	unsigned char image_buffer[(320*240)] ; //monochrome frame buffer
	unsigned short fifo_state, fifo_data ;
	if(logi_open() < 0){
		printf("Error openinglogi \n");
		return -1 ;	
	}
	jpeg_fd  = fopen("./grabbed_frame.jpg", "w");
	if(jpeg_fd == NULL){
		perror("Error opening output file");
		exit(EXIT_FAILURE);
	}
	printf("output file openened \n");
	printf("loading input file : %s \n", argv[1]);
	res = read_jpeg_file( argv[1], &inputImage);
	if(res < 0){
		perror("Error opening input file");
		exit(EXIT_FAILURE);
	}
	
	//configuring for gauss->sobel->hysteresis->dilate->erode->output
	/*
	reg_buffer[0] = GAUSS_SOURCE_FIFO ;
	reg_buffer[1] = SOBEL_SOURCE_GAUSS ;
	reg_buffer[2] = ERODE_SOURCE_DILATE ;
	reg_buffer[3] = DILATE_SOURCE_HYST ;
	reg_buffer[4] = HYST_SOURCE_SOBEL ;
	reg_buffer[5] = OUTPUT_SOURCE_ERODE ;
	*/

	logi_write(reg_buffer, 12, REG_ADDR);
	printf("issuing reset to fifo \n");
	cmd_buffer[1] = 0; 
	cmd_buffer[2] = 0 ;
	logi_write(cmd_buffer, 6, FIFO_CMD_ADDR);
	logi_read(cmd_buffer, 6, FIFO_CMD_ADDR);
	printf("fifo size : %d, free: %d, available : %d \n", cmd_buffer[0],cmd_buffer[1], cmd_buffer[2]);
	clock_gettime(CLOCK_REALTIME, &cpu_time);
	start_time = cpu_time.tv_nsec ;
	 for(i = 0 ; i < IMAGE_HEIGHT ; i +=LINE_BURST){
		logi_write(&inputImage[(i*IMAGE_WIDTH)], IMAGE_WIDTH*LINE_BURST, 0x0000);
		do{
			logi_read(cmd_buffer, 6, FIFO_CMD_ADDR);
			//printf("fifo size : %d, free: %d, available : %d \n", cmd_buffer[0],cmd_buffer[1], cmd_buffer[2]);
		}while((cmd_buffer[2]*2) < IMAGE_WIDTH*LINE_BURST);
		logi_read(&image_buffer[(i*IMAGE_WIDTH)], IMAGE_WIDTH*LINE_BURST, 0x0000);
        }
	clock_gettime(CLOCK_REALTIME, &cpu_time);
	end_time = cpu_time.tv_nsec ;
	diff_time = end_time - start_time ;
	diff_time = diff_time/1000000000 ;
	printf("transffered %d bytes in %f s : %f B/s \n", IMAGE_WIDTH * IMAGE_HEIGHT, diff_time, (IMAGE_WIDTH * IMAGE_HEIGHT)/diff_time);
	write_jpegfile(image_buffer, 320, 240, jpeg_fd, 100);
	logi_close();
	fclose(jpeg_fd);
	return 0 ;

}

void place(list<Filter*> filtering_chain, Filter *new_filter, const char *source){
	list<Filter*>::iterator current = filtering_chain.begin();

	for(current; current != filtering_chain.end(); current++){
		if(strcmp((*current)->getName(), new_filter->getName()) == 0){
			(*current)->set_source_id(source);
		}
	}
}

int *filtering_chain_builder(list<Filter*> input_filter){

	static int buffer[FILTER_SIZE] = {0};
	list<Filter*> filtering_chain;

	// build the filtering chain
	filtering_chain.push_back(new Gauss());
	filtering_chain.push_back(new Sobel());
	filtering_chain.push_back(new Erode());
	filtering_chain.push_back(new Dilate());
	filtering_chain.push_back(new Hyst());
	filtering_chain.push_back(new Output());

	list<Filter*>::iterator current = input_filter.begin();
	list<Filter*>::iterator previous = current++;

	// set initial to fifo
	place(filtering_chain, *current, "fifo");
	for(current; current != input_filter.end(); current++, previous++){
		place(filtering_chain, *current, (*previous)->getName());
	}
	//set final to output
	place(filtering_chain, new Output(), (*previous)->getName());

	// print result
	current = filtering_chain.begin();
	int j = 0;
	for(current; current != filtering_chain.end(); current++){
		buffer[j++] = (*current)->get_source();
	}
	
	// build array
	return buffer;
}

void print_array(int *array, int size){
	for(int i = 0; i < size; i++){
		cout<<i<<". Element: "<<array[i]<<endl;
	}
}