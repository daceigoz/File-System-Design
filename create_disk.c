
/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 *  create_disk.c
 *
 *  DrvSim. Version 1.2
 *
 *  INFODSO@ARCOS.INF.UC3M.ES
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "include/filesystem.h"

int main ( int argc, char *argv[] )
{

	char dummy_block[BLOCK_SIZE];

	if(argc != 2){
		printf("ERROR: Incorrect number of arguments:\n");
		printf("Syntax: ./create_disk <num_blocks>\n");
		return -1;
	}

	int num_blocks = atoi(argv[1]);

	int fd = open("disk.dat", O_CREAT | O_RDWR | O_TRUNC, 0666);

	if(fd < 0){
		fprintf(stderr, "ERROR: UNABLE TO OPEN DISK FILE disk.dat \n");
	}

	memset(dummy_block, '0', BLOCK_SIZE);

	int i;
	int total_write, write_result;
	for(i = 0; i < num_blocks; i++){
		total_write = 0;
		do{
			write_result = write(fd, dummy_block+total_write, BLOCK_SIZE-total_write);
			total_write = total_write + write_result;
		}while(total_write < BLOCK_SIZE && write_result >= 0);
	}

	return 0;
}
