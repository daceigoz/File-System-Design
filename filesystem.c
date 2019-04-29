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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define NUM_INODES 40

static struct inode inodes[NUM_INODES];

//static int fd; //File Descriptor for disk.dat

static int mounted=0;//Boolean to indicate if the disk is mounted (0 is closed 1 is open)

static char bitmap[5]; //These will represent the 40 blocks that can be used for files.

static int num_items; //Will count the amount of generated directories and files to avoid exceeding the maximum amount.

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */

int mkFS(long deviceSize)
{
	bzero(inodes, NUM_INODES*sizeof(struct inode));
	bzero(bitmap, 5*sizeof(char));

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
	if(mounted){
		printf("The disk is already mounted\n");
		return -1;
	}

	int cur_inode=0;
	for(int i=0; i<8; i++){//For the blocks of inodes

		char inode_block[5][sizeof(struct inode)];
		for(int j=0; j<5; j++){
			memcpy(inode_block[j], &inodes[cur_inode],sizeof(struct inode));
			cur_inode++;
		}

		bwrite(DEVICE_IMAGE, i, (char *) inode_block); //write all the inodes to the current block
	}

	mounted=1;
	return 0;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	mounted=0;
	return mounted;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *path)
{
	if(num_items>40) {
		printf("There are too many elements in the File System\n");
		return -2;
	}

	if(!mounted){
		printf("disk not mounted yet\n");
		return -1;
	}
	int ret_value=0;
	//First we obtain the path of the directory containing this file with the path given
	char * aux_path=path;
	char aux_char=*path; //This character will be used to read char by char until an "/" is found to measure the length.
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

	int f=0, counter=0;
	for(int k=0; k<NUM_INODES; k++){
		if(!strcmp(obtained_dir, inodes[k].dir_path)) f=1;
		counter++;
	}
	counter--;
	if(!f){
		printf("The directory where the file wants to be created does not exist\n");
		return -2;
	}

	printf("Previous obtained path: %s\n", obtained_dir);

	//Creating the inode for the file:
	struct inode new_file;
	bzero(&new_file, sizeof(struct inode));
	strcpy(new_file.file_path, path);
	new_file.type='F';
	new_file.opened='N';
	int n;
	for(n=0;n<NUM_INODES;n++){
		if(!bitmap_getbit(bitmap,n)){
			bitmap_setbit(bitmap,n,1);
			break;
		}
	}
	new_file.block=n+8;//We add up 8 as the first 8 blocks are for inodes
	printf("Block for data: %d\n", new_file.block);

	for(int i=0;i<NUM_INODES;++i){//traverse all inodes array to check if the file exists already
		if(!strcmp(inodes[i].file_path, path) ){
			return -1;
		}
	}
	int i;
	for(i=0;i<NUM_INODES;++i){//traverse all the inodes array and asign the first free space to this inode
		if(!strcmp(inodes[i].dir_path, "") && !strcmp(inodes[i].file_path, "")){
			inodes[i]=new_file;
			inodes[i].id=i;
			break;
		}
	}

	//Adding a reference to the directory where the file is stored:
	int coinciding_path=0, adv=0;
	while(!coinciding_path && adv<NUM_INODES){
		if(!strcmp(obtained_dir, inodes[adv].dir_path)){
			coinciding_path=1;
			inodes[i].parent = &inodes[adv];
		}
		else{
			adv++;
		}
	}

	printf("Value of adv: %d\n", adv);


	for(int j=0;j<NUM_INODES;++j){//traverse all the inodes array and asign the first free space to this inode
		if(!inodes[adv].contents[j]){
			inodes[adv].contents[j]=&inodes[i];
			break;
		}
	}


	/*for(int x=0;x<10;x++){
		printf("Item in root: %s\n", inodes[1].contents[x]->file_path);
	}*/


	//lastly we have to update the disk if there is any modification
	if (mounted){
		int cur_inode=0;
		for(int x=0; x<8; x++){//For the blocks of inodes
			char inode_block[5][sizeof(struct inode)];

			for(int y=0; y<5; y++){
				memcpy(inode_block[y], &inodes[cur_inode],sizeof(struct inode));
				cur_inode++;
			}

			bwrite(DEVICE_IMAGE, x, (char *) inode_block); //write all the inodes to the current block
		}
	}

	num_items++;
	return ret_value;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *path)
{
	if(!mounted){
		printf("disk not mounted yet\n");
		return -1;
	}
	/*For removing a file we will have to remove the inode of the file itself,
	clean the block where the file was stored and romove the reference to the inode
	from its prent directory*/

	//First we will check if the file's inode exists and remove it:

	for(int i=0;i<NUM_INODES;++i){//traverse all inodes array to check if the file exists
		if(!strcmp(inodes[i].file_path, path)){
			//REMOVE HERE THE DATA STORED IN THE FILE BLOCK.
			if(inodes[i].opened=='Y'){
				printf("The file is opened so it cannot be deleted.\n");
				return -2;
			}
			bitmap_setbit(bitmap,(inodes[i].block-8),0);

			for(int j=0;j<10;j++){
				if(inodes[i].parent->contents[j]==&inodes[i]){
					inodes[i].parent->contents[j]=NULL;
				}
			}
			//Removing the reference from the parent directory of the file:
			memset(&inodes[i], 0, sizeof(struct inode));

			if (mounted){
				int cur_inode=0;
				for(int x=0; x<8; x++){//For the blocks of inodes
					char inode_block[5][sizeof(struct inode)];
					memset(inode_block, '0', 5*sizeof(struct inode));
					for(int y=0; y<5; y++){
						memcpy(inode_block[y], &inodes[cur_inode],sizeof(struct inode));
						cur_inode++;
					}

					bwrite(DEVICE_IMAGE, x, (char *) inode_block); //write all the inodes to the current block
				}
			}

			num_items--;
			return 0;
		}
	}

	return -1;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *path)
{
	if(!mounted){
		printf("disk not mounted yet\n");
		return -1;
	}
	int found_file=0;
	int i;
	for(i=0;i<NUM_INODES;++i){//traverse all inodes array to check if the file exists already
		if(!strcmp(inodes[i].file_path, path) ){
			found_file=1;
			break;
		}
	}

	if(!found_file) return -1;

	inodes[i].opened='Y';

	inodes[i].seek_ptr=0;
	return inodes[i].id;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	if(!mounted){
		printf("disk not mounted yet\n");
		return -1;
	}
	inodes[fileDescriptor].opened='N';
	return 0;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
	if(!mounted){
		printf("disk not mounted yet\n");
		return -1;
	}

	if(bread(DEVICE_IMAGE, inodes[fileDescriptor].block, (char*)buffer)==-1){

	return -1;
	}
	return numBytes;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	if(!mounted){
		printf("disk not mounted yet\n");
		return -1;
	}
	int i;
	for(i=0;i<40;i++){
		if(inodes[i].id==fileDescriptor){
			break;
		}
	}
	if(i==40){
		printf("The file descriptor does not correspond to any existing file\n");
		return -1;
	}
	char auxbuffer[2048];
	bzero(auxbuffer, sizeof(auxbuffer));
	strcpy(auxbuffer, (char *)buffer);
	printf("buffer: %s\n", (char *)buffer);
	bwrite(DEVICE_IMAGE, inodes[i].block, (char *)auxbuffer);
	char rdbuffer[2048];
	bzero(rdbuffer, sizeof(rdbuffer));
	bread(DEVICE_IMAGE, inodes[i].block, rdbuffer);
	printf("Auxbuf: %s\n", rdbuffer);
	return numBytes;
}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	if(!mounted){
		printf("disk not mounted yet\n");
		return -1;
	}
switch(whence){
	case 0:
		inodes[fileDescriptor].seek_ptr=inodes[fileDescriptor].seek_ptr+offset;
		if((inodes[fileDescriptor].seek_ptr>(2048*(inodes[fileDescriptor].block+1)-1)) || inodes[fileDescriptor].seek_ptr<(2048*(inodes[fileDescriptor].block))){
			return -1;
		}
		return 0;

	case 1:
		inodes[fileDescriptor].seek_ptr=2048*(inodes[fileDescriptor].block+1)-1;
		if((inodes[fileDescriptor].seek_ptr>(2048*(inodes[fileDescriptor].block+1)-1)) || inodes[fileDescriptor].seek_ptr<(2048*(inodes[fileDescriptor].block))){
			return -1;
		}
		return 0;

	case 2:
		inodes[fileDescriptor].seek_ptr=2048*inodes[fileDescriptor].block;
		if((inodes[fileDescriptor].seek_ptr>(2048*(inodes[fileDescriptor].block+1)-1)) || inodes[fileDescriptor].seek_ptr<(2048*(inodes[fileDescriptor].block))){
			return -1;
		}
		return 0;

	default:
		return -1;
	}
}

/*
 * @brief	Creates a new directory provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the directory already exists, -2 in case of error.
 */
int mkDir(char *path)
{
	if(num_items>40) {
		printf("There are too many elements in the File System\n");
		return -2;
	}

	if(!mounted){
		printf("disk not mounted yet\n");
		return -1;
	}
	//First we will check if the directory to be created already exists:
	for(int x=0;x<NUM_INODES;++x){//traverse all inodes array to check if the file exists
		if(!strcmp(inodes[x].dir_path, path)){
			//In this case the directory to be created already exists.
			return -1;
			}
		}

	//Then we obtain the path of the directory containing this directory with the path given
	char * aux_path=path;
	char aux_char=*path; //This character will be used to read char by char until an "/" is found to measure the length.
	int slash_pos=strlen(path), found=0, last=strlen(path);

	aux_path+=last-1;
	while(found!=2){
		aux_char=*aux_path;
		if(aux_char=='/'){
			found++;
			if(found==1){
				slash_pos--;
				aux_path--;
			}
		}
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

	//Creating the inode for the new directory:
	struct inode new_dir;
	bzero(&new_dir, sizeof(struct inode));
	strcpy(new_dir.dir_path, path);
	new_dir.type='D';

	int i;
	for(i=0;i<NUM_INODES;++i){//traverse all the inodes array and asign the first free space to this inode
		if(!strcmp(inodes[i].dir_path, "") && !strcmp(inodes[i].file_path, "")){
			inodes[i]=new_dir;
			inodes[i].id=i;
			break;
		}
	}

	//Adding a reference to the directory where the file is stored:
	int coinciding_path=0, adv=0;
	while(!coinciding_path && adv<NUM_INODES){
		if(!strcmp(obtained_dir, inodes[adv].dir_path)){
			printf("a\n");
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
		for(int j=0;j<NUM_INODES;++j){//traverse all the inodes array and asign the first free space to this inode
			if(!inodes[adv].contents[j]){
				inodes[adv].contents[j]=&inodes[i];
				break;
			}
		}
	}

	/*for(int y=0;y<=10;y++){
		printf("Item in root: %s\n", inodes[0].contents[y]->dir_path);
	}*/

	if (mounted){
		int cur_inode=0;
		for(int x=0; x<8; x++){//For the blocks of inodes
			char inode_block[5][sizeof(struct inode)];

			for(int y=0; y<5; y++){
				memcpy(inode_block[y], &inodes[cur_inode],sizeof(struct inode));
				cur_inode++;
			}

			bwrite(DEVICE_IMAGE, x, (char *) inode_block); //write all the inodes to the current block
		}
	}

	num_items++;
	return 0;
}

/*
 * @brief	Deletes a directory, provided it exists in the file system.
 * @return	0 if success, -1 if the directory does not exist, -2 in case of error..
 */
int rmDir(char *path)
{
	if(!mounted){
		printf("disk not mounted yet\n");
		return -1;
	}

	//First we will check if the directory's inode exists and remove it:

	for(int i=0;i<NUM_INODES;++i){//traverse all inodes array to check if the directory exists
		if(!strcmp(inodes[i].dir_path, path)){
			//REMOVE HERE THE DATA STORED IN THE FILE BLOCK.
			for(int k=0;k<10;++k){
				//printf("A: %d, B: %d\n", i, k);
				//printf("%c\n", inodes[i].contents[k]->type);

				if(inodes[i].contents[k]!=NULL){
					//The directory has contents inside
					return -2;
				}
			}

			for(int j=0;j<10;j++){
				if(inodes[i].parent->contents[j]==&inodes[i]){
					inodes[i].parent->contents[j]=NULL;
				}
			}
			//Removing the inode:
			memset(&inodes[i], 0, sizeof(struct inode));

			/*for(int y=0;y<10;y++){
				printf("Item in root: %s\n", inodes[0].contents[y]->dir_path);
			}*/

			if (mounted){
				int cur_inode=0;
				for(int x=0; x<8; x++){//For the blocks of inodes
					char inode_block[5][sizeof(struct inode)];

					for(int y=0; y<5; y++){
						memcpy(inode_block[y], &inodes[cur_inode],sizeof(struct inode));
						cur_inode++;
					}

					bwrite(DEVICE_IMAGE, x, (char *) inode_block); //write all the inodes to the current block
				}
			}

			num_items--;
			return 0;
		}
	}
		//directory does not exist
		return -1;
}

/*
 * @brief	Lists the content of a directory and stores the inodes and names in arrays.
 * @return	The number of items in the directory, -1 if the directory does not exist, -2 in case of error..
 */
int lsDir(char *path, int inodesDir[10], char namesDir[10][33])
{
	if(!mounted){
		printf("disk not mounted yet\n");
		return -1;
	}
	//First we will check if the directory's inode exists and remove it:

	for(int i=0;i<NUM_INODES;++i){//traverse all inodes array to check if the directory exists
		if(!strcmp(inodes[i].dir_path, path)){

			for(int k=0;k<10;++k){
				if(inodes[i].contents[k]!=NULL){
					inodesDir[k]=inodes[i].contents[k]->id;
					if(inodes[i].contents[k]->type=='D'){
						strcpy(namesDir[k],inodes[i].contents[k]->dir_path);
					}
					else if(inodes[i].contents[k]->type=='F'){
						strcpy(namesDir[k],inodes[i].contents[k]->file_path);
					}
					else return -2; //Unknown file type.
				}
			}

			return 0;
		}
		if(!strcmp(inodes[i].file_path, path)){
			return -2; //The path is from a file not from a directory
		}
	}
		//directory does not exist
		return -1;
}
