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

static struct inode inodes[NUM_INODES];//
static struct sBlock superBlock;


/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */

int mkFS(long deviceSize)
{
	if(deviceSize<50000 || deviceSize>10000000){
		printf("The device size must be between 50Kb and 10Mb\n");
		return -1;
	}
	if(deviceSize%2048!=0){
		printf("The device size must be a multiple of the block size: %d\n", BLOCK_SIZE);
		return -1;
	}
	superBlock.partitionBlocks=(int)deviceSize/2048;
	bzero(inodes, NUM_INODES*sizeof(struct inode));
	bzero(superBlock.bitmap, 5*sizeof(char));
	//Intializing the root directory inode:
	struct inode root;
	bzero(&root, sizeof(struct inode));
	strcpy(root.dir_path, "/");
	root.type='D';
	superBlock.num_items=1;
	superBlock.mounted=0;
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
	if(superBlock.mounted){
		printf("The disk is already mounted\n");
		return -1;
	}

	int cur_inode=0;
	for(int i=1; i<9; i++){//For the blocks of inodes

		char inode_block[5][sizeof(struct inode)];
		int j;
		for(j=0; j<5; j++){
			memcpy(inode_block[j], &inodes[cur_inode],sizeof(struct inode));
			cur_inode++;
		}

		bwrite(DEVICE_IMAGE, i, (char *) inode_block); //write all the inodes to the current block

	}
	superBlock.mounted=1;

	char supblock[2048];
	bzero(supblock, sizeof(supblock));
	memcpy(supblock,&superBlock, sizeof(struct sBlock));

	bwrite(DEVICE_IMAGE,0,supblock);
	return 0;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	superBlock.mounted=0;

	char resetblock[2048];
	bzero(resetblock, sizeof(resetblock));
	bwrite(DEVICE_IMAGE,0,resetblock);
	return superBlock.mounted;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *path)
{
	if(superBlock.num_items>=40) {
		printf("There are too many elements in the File System\n");
		return -2;
	}

	if(!superBlock.mounted){
		printf("disk not mounted yet\n");
		return -2;
	}
	if(strlen(path)>132){
		printf("Name of the path too long, try shortening the names of the directories\n");
		return -2;
	}
	int ret_value=0;

	//We also need to check the depth is not greater than 3, for that we will count the '/' of the path
	char * aux_path=path;
	char aux_char=*path;
	int depth=0;
	while((aux_char=*aux_path)!='\0'){
		if(aux_char=='/'){
			depth++;
			aux_path++;

		}
		else{
			aux_path++;
		}
	}
	if(depth>5){
		printf("The maximum depth is 3, you cannot create a directory here\n");
		return -2;
	}
	//Then we obtain the path of the directory containing this file with the path given
	 aux_path=path;
	 aux_char=*path; //This character will be used to read char by char until an "/" is found to measure the length.
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

	char obtained_dir[strlen(path)];
	int size_path=strlen(path);
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


	//Creating the inode for the file:
	struct inode new_file;
	bzero(&new_file, sizeof(struct inode));
	strcpy(new_file.file_path, path);
	new_file.type='F';
	new_file.opened='N';
	int n;
	for(n=0;n<NUM_INODES;n++){
		if(!bitmap_getbit(superBlock.bitmap,n)){
			if(n>superBlock.partitionBlocks-9){//To avoid creating a block outside the partition
				printf("No space remaining in the disk for files\n");
				return -2;
			}
			char checkblock[2048];
			bzero(checkblock, sizeof(checkblock));
			if(bread(DEVICE_IMAGE, n+9, checkblock)==-1){
				printf("No space remaining in the disk for files\n");
				return -2;
			}
			bitmap_setbit(superBlock.bitmap,n,1);
			break;
		}
	}
	new_file.block=n+9;//We add up 10 as the first 8 blocks are for inodes plus 1 of the bitmap plus one for the superblock

	for(int i=0;i<NUM_INODES;++i){//traverse all inodes array to check if the file exists already
		if(!strcmp(inodes[i].file_path, path) ){
			printf("The file exist already\n");
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

	if(strlen(path)-strlen(obtained_dir)>32){
		printf("Name of the file too long, please insert a name under 32 characters\n");
		return -1;
	}


	int checkparent=0;
	for(int j=0;j<10;++j){//traverse all the inodes array and asign the first free space to this inode
		if(!inodes[adv].contents[j]){
			inodes[adv].contents[j]=&inodes[i];
			checkparent++;
			break;
		}
	}
	if(!checkparent){
		printf("Not enough space in the directory\n");
		return -1;
	}

	/*for(int x=0;x<10;x++){
		printf("Item in root: %s\n", inodes[1].contents[x]->file_path);
	}*/


	//lastly we have to update the disk if there is any modification
	if (superBlock.mounted){
		int cur_inode=0;
		for(int x=1; x<9; x++){//For the blocks of inodes
			char inode_block[5][sizeof(struct inode)];

			for(int y=0; y<5; y++){
				memcpy(inode_block[y], &inodes[cur_inode],sizeof(struct inode));
				cur_inode++;
			}

			bwrite(DEVICE_IMAGE, x, (char *) inode_block); //write all the inodes to the current block
		}
	}

	superBlock.num_items++;


	char supblock[2048];
bzero(supblock, sizeof(supblock));
memcpy(supblock,&superBlock, sizeof(struct sBlock));

bwrite(DEVICE_IMAGE,0,supblock);
	return ret_value;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *path)
{
	if(!superBlock.mounted){
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
			bitmap_setbit(superBlock.bitmap,(inodes[i].block-8),0);

			for(int j=0;j<10;j++){
				if(inodes[i].parent->contents[j]==&inodes[i]){
					inodes[i].parent->contents[j]=NULL;
				}
			}
			//Removing the reference from the parent directory of the file:
			memset(&inodes[i], 0, sizeof(struct inode));

			if (superBlock.mounted){
				int cur_inode=0;
				for(int x=1; x<9; x++){//For the blocks of inodes
					char inode_block[5][sizeof(struct inode)];
					memset(inode_block, '0', 5*sizeof(struct inode));
					for(int y=0; y<5; y++){
						memcpy(inode_block[y], &inodes[cur_inode],sizeof(struct inode));
						cur_inode++;
					}

					bwrite(DEVICE_IMAGE, x, (char *) inode_block); //write all the inodes to the current block
				}
			}

			superBlock.num_items--;


			char supblock[2048];
bzero(supblock, sizeof(supblock));
memcpy(supblock,&superBlock, sizeof(struct sBlock));

bwrite(DEVICE_IMAGE,0,supblock);
			return 0;
		}
	}
	printf("Error removing the file\n");
	return -1;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *path)
{
	if(!superBlock.mounted){
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

	if(!found_file){
		printf("The file that is being opened does not exist\n");
		return -1;
	}

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
	if(!superBlock.mounted){
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
	if(!superBlock.mounted){
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

	if(inodes[i].opened=='N'){
		printf("File is not opened\n");
		return -1;
	}
	if(numBytes+inodes[i].seek_ptr>2048){
		numBytes=2048-inodes[i].seek_ptr;
	}
	//Now we perform the read
	char rdbuffer[2048];
	bzero(rdbuffer, sizeof(rdbuffer));
	bread(DEVICE_IMAGE, inodes[i].block, rdbuffer);
	memcpy(buffer,(void *)rdbuffer+inodes[i].seek_ptr,numBytes);

	inodes[i].seek_ptr+=numBytes;

	return numBytes;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	if(!superBlock.mounted){
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
	if(numBytes>=strlen(buffer)){//To avoid copying the end of file character
		numBytes=strlen(buffer)-1;
	}
	if(inodes[i].opened=='N'){
		printf("File is not opened\n");
		return -1;
	}
	if(numBytes+inodes[i].seek_ptr>2048){
		numBytes=2048-inodes[i].seek_ptr;
	}

	//Now we just need to write on the file
	char rdbuffer[2048];
	bzero(rdbuffer, sizeof(rdbuffer));
	bread(DEVICE_IMAGE, inodes[i].block, rdbuffer);
	//strncpy(rdbuffer+inodes[i].seek_ptr,buffer,numBytes);
	memcpy(rdbuffer+inodes[i].seek_ptr,buffer,numBytes);

	bwrite(DEVICE_IMAGE, inodes[i].block, (char *)rdbuffer);
	inodes[i].seek_ptr+=numBytes;

	return numBytes;
}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	if(!superBlock.mounted){
		printf("disk not mounted yet\n");
		return -1;
	}
switch(whence){
	case 0:
		inodes[fileDescriptor].seek_ptr=inodes[fileDescriptor].seek_ptr+offset;
		if((inodes[fileDescriptor].seek_ptr>2048) || inodes[fileDescriptor].seek_ptr<0){
			printf("The pointer goes out of bounds\n");
			return -1;
		}
		return 0;

	case 1:
		inodes[fileDescriptor].seek_ptr=2048;
		if((inodes[fileDescriptor].seek_ptr>2048) || inodes[fileDescriptor].seek_ptr<0){
			printf("The pointer goes out of bounds\n");
			return -1;
		}
		return 0;

	case 2:
		inodes[fileDescriptor].seek_ptr=0;
		if((inodes[fileDescriptor].seek_ptr>2048) || inodes[fileDescriptor].seek_ptr<0){
			printf("The pointer goes out of bounds\n");
			return -1;
		}
		return 0;

	default:
		printf("Unexpected error while performing lseek\n");
		return -1;
	}
}

/*
 * @brief	Creates a new directory provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the directory already exists, -2 in case of error.
 */
int mkDir(char *path)
{
	if(superBlock.num_items>=40) {
		printf("There are too many elements in the File System\n");
		return -2;
	}

	if(!superBlock.mounted){
		printf("disk not mounted yet\n");
		return -2;
	}
	if(strlen(path)>99){
		printf("Name of the path too long, try shortening the names of the directories\n");
		return -2;
	}
	//Now we will check if the directory to be created already exists:
	for(int x=0;x<NUM_INODES;++x){//traverse all inodes array to check if the file exists
		if(!strcmp(inodes[x].dir_path, path)){
			//In this case the directory to be created already exists.
			printf("The directory already exists\n");
			return -1;
		}
	}
	//We also need to check the depth is not greater than 3, for that we will count the '/' of the path
	char * aux_path=path;
	char aux_char=*path;
	int depth=0;
	while((aux_char=*aux_path)!='\0'){
		if(aux_char=='/'){
			depth++;
			aux_path++;

		}
		else{
			aux_path++;
		}
	}
	if(depth>5){
		printf("The maximum depth is 3, you cannot create a directory here\n");
		return -2;
	}

	//Then we obtain the path of the directory containing this directory with the path given
	aux_path=path;
	aux_char=*path; //This character will be used to read char by char until an "/" is found to measure the length.
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

	char obtained_dir[strlen(path)];
	int size_path=strlen(path);
	bzero(obtained_dir, size_path);
	memcpy(obtained_dir, path, slash_pos); //The obtained dir will be useful to place the newly created file as a content of the corresponding directory inode.

	if(strlen(path)-strlen(obtained_dir)>32){
		printf("Name of the directory too long, it must have under 32 characters\n");
		return -2;
	}

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
			coinciding_path=1;
			inodes[i].parent = &inodes[adv];
		}
		else{
			adv++;
		}
	}

	if(!coinciding_path){
		printf("There is no such directory\n");
		return -2;
	}

	else{

		int checkparent=0;
		for(int j=0;j<10;++j){//traverse all the inodes array and asign the first free space to this inode
			if(!inodes[adv].contents[j]){
				inodes[adv].contents[j]=&inodes[i];
				checkparent++;
				break;
			}
		}
		if(!checkparent){
			printf("Not enough space in the directory\n");
			return -1;
		}
	}

	/*for(int y=0;y<=10;y++){
		printf("Item in root: %s\n", inodes[0].contents[y]->dir_path);
	}*/

	if (superBlock.mounted){
		int cur_inode=0;
		for(int x=1; x<9; x++){//For the blocks of inodes
			char inode_block[5][sizeof(struct inode)];

			for(int y=0; y<5; y++){
				memcpy(inode_block[y], &inodes[cur_inode],sizeof(struct inode));
				cur_inode++;
			}

			bwrite(DEVICE_IMAGE, x, (char *) inode_block); //write all the inodes to the current block
		}
	}

	superBlock.num_items++;


	char supblock[2048];
bzero(supblock, sizeof(supblock));
memcpy(supblock,&superBlock, sizeof(struct sBlock));

bwrite(DEVICE_IMAGE,0,supblock);
	return 0;
}

/*
 * @brief	Deletes a directory, provided it exists in the file system.
 * @return	0 if success, -1 if the directory does not exist, -2 in case of error..
 */
int rmDir(char *path)
{
	if(!superBlock.mounted){
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
					printf("The directory has contents inside\n");
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

			if (superBlock.mounted){
				int cur_inode=0;
				for(int x=1; x<9; x++){//For the blocks of inodes
					char inode_block[5][sizeof(struct inode)];

					for(int y=0; y<5; y++){
						memcpy(inode_block[y], &inodes[cur_inode],sizeof(struct inode));
						cur_inode++;
					}

					bwrite(DEVICE_IMAGE, x, (char *) inode_block); //write all the inodes to the current block
				}
			}

			superBlock.num_items--;


			char supblock[2048];
bzero(supblock, sizeof(supblock));
memcpy(supblock,&superBlock, sizeof(struct sBlock));

bwrite(DEVICE_IMAGE,0,supblock);
			return 0;
		}
	}
		//directory does not exist
		printf("The directory does not exist\n");
		return -1;
}

/*
 * @brief	Lists the content of a directory and stores the inodes and names in arrays.
 * @return	The number of items in the directory, -1 if the directory does not exist, -2 in case of error..
 */
int lsDir(char *path, int inodesDir[10], char namesDir[10][33])
{
	if(!superBlock.mounted){
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
						printf("%s\n", inodes[i].contents[k]->dir_path);
					}
					else if(inodes[i].contents[k]->type=='F'){
						strcpy(namesDir[k],inodes[i].contents[k]->file_path);
						printf("%s\n", inodes[i].contents[k]->file_path);
					}
					else {//Unknown file type.
						printf("Unknown element type\n");
						return -2;
					}
				}
			}


			return 0;
		}
		if(!strcmp(inodes[i].file_path, path)){
			printf("The path of the arguments is from a file. This path is required to be from a directory\n");
			return -2; //The path is from a file not from a directory
		}
	}
		//directory does not exist
		printf("The directory does not exist\n");
		return -1;
}
