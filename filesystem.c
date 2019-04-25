/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	filesystem.c
 * @brief 	Implementation of the core file system funcionalities and auxiliary functions.
 * @date	01/03/2017
 */

#include "include/filesystem.h" // Headers for the core functionality
#include "include/auxiliary.h"  // Headers for auxiliary functions
#include "include/metadata.h"   // Type and structure declaration of the file system
#include <string.h>

static int num_elements;

static struct inode inodes[40];

/*for(int i=0;i<40;++i){
	if(inodes[i]==NULL){
		inodes[i]=((newinode))
	}
}*/

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{

	bzero(inodes, 40*sizeof(struct inode));

	//Intializing the root directory inode:
	struct inode root;
	bzero(&root, sizeof(struct inode));
	strcpy(root.dir_path, "/");
	root.type='D';
	//Everything else for the inode shall remain empty for the root in the initial state.

	inodes[0]=root;


	return 0;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void)
{
	return -1;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	return -1;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *path)
{
	int ret_value=0;
	//First we obtain the path of the directory containing this file with the path given
	char * aux_path=path;
	char aux_char=*path; //This character will be used to read char by char until a "/" is found to measure the lenght.
	int slash_pos=strlen(path), found=0, last=strlen(path);

	aux_path+=last-1;
	while(!found){
		aux_char=*aux_path;
		if(aux_char=='/')found=1;
		else{
			slash_pos--;
			aux_path--;
		}
	}

	char obtained_dir[sizeof(path)];
	int size_path=sizeof(path);
	bzero(obtained_dir, size_path);
	memcpy(obtained_dir, path, slash_pos); //The obtained dir will be useful to place the newly created file as a content of the corresponding directory inode.

	printf("Previous obtained path: %s\n", obtained_dir);

	//Creating the inode for the file:
	struct inode new_file;
	bzero(&new_file, sizeof(struct inode));
	strcpy(new_file.file_path, path);
	new_file.type='F';
	new_file.opened='N';

	for(int i=0;i<40;++i){//traverse all inodes array to check if the file exists already
		if(!strcmp(inodes[i].dir_path, path) || !strcmp(inodes[i].file_path, path) ){
			return -1;
		}
	}
	int i;
	for(i=0;i<40;++i){//traverse all the inodes array and asign the first free space to this inode
		if(!strcmp(inodes[i].dir_path, "") && !strcmp(inodes[i].file_path, "")){
			inodes[i]=new_file;
			break;
		}
	}

	//Adding a reference to the directory where the file is stored:
	int coinciding_path=0, adv=0;
	while(!coinciding_path && adv<40){
		if(!strcmp(obtained_dir, inodes[adv].dir_path)){
			coinciding_path=1;
			inodes[i].parent = &inodes[adv];
		}
		else{
			adv++;
		}
	}

	printf("Value of adv: %d\n", adv);

	if(!coinciding_path) return -2;

	else{
		for(int j=0;j<10;++j){//traverse all the inodes array and asign the first free space to this inode
			if(!inodes[adv].contents[j]){
				inodes[adv].contents[j]=&inodes[i];
				break;
			}
		}
	}

	for(int x=0;x<=num_elements;x++){
		printf("Item in root: %s\n", inodes[0].contents[x]->file_path);
	}

	num_elements++;


	return ret_value;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *path)
{
	int ret_value=0;
	/*For removing a file we will have to remove the inode of the file itself,
	clean the block where the file was stored and romove the reference to the inode
	from its prent directory*/

	//First we will check if the file's inode exists and remove it:

	struct inode stored_in; //will contain a pointer to the inode of the directory containing the file.
	bzero(&stored_in, sizeof(struct inode));

	for(int i=0;i<40;++i){//traverse all inodes array to check if the file exists already
		if(!strcmp(inodes[i].file_path, path)){
			//REMOVE HERE THE DATA STORED IN THE FILE BLOCK.

			//Removing the reference from the parent directory of the file:
			for(int j=0;j<10;++j){
				if(!strcmp(inodes[i].parent->contents[j]->file_path, path)){
					bzero(&inodes[i].parent->contents[j]->file_path, sizeof(inodes[0].file_path));
					break;
				}
			}

			bzero(&inodes[i], sizeof(struct inode));
			break;
		}
	}

	/*Now, using the parent directory stored in the inode of the file to be removed we will
	remove the reference to the file*/

	return ret_value;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *path)
{
	return -2;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	return -1;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
	return -1;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	return -1;
}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	return -1;
}

/*
 * @brief	Creates a new directory provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the directory already exists, -2 in case of error.
 */
int mkDir(char *path)
{
	return -2;
}

/*
 * @brief	Deletes a directory, provided it exists in the file system.
 * @return	0 if success, -1 if the directory does not exist, -2 in case of error..
 */
int rmDir(char *path)
{
	return -2;
}

/*
 * @brief	Lists the content of a directory and stores the inodes and names in arrays.
 * @return	The number of items in the directory, -1 if the directory does not exist, -2 in case of error..
 */
int lsDir(char *path, int inodesDir[10], char namesDir[10][33])
{
	return -2;
}
