/*
 * OPERATING SYSTEMS DESING - 16/17
 */

#ifndef _BLOCKS_H_
#define _BLOCKS_H_

/* This file contains the interface of the functions that must be implemented
 * to allow the user access to the file system and the files. IT CANNOT BE
 * MODIFIED.
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLOCK_SIZE 2048


/****************/
/* Disk access. */
/****************/

/*
 * Reads a block from the device and stores it in a buffer.
 * Returns 0 if correct or -1 in case of error, including short
 * read.
 */
int bread(char *deviceName, int blockNumber, char *buffer);

/*
 * Writes a block from a buffer to the device.
 * Returns 0 if correct or -1 in case of error.
 */
int bwrite(char *deviceName, int blockNumber, char*buffer);
#endif
