/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	metadata.h
 * @brief 	Definition of the structures and data types of the file system.
 * @date	01/03/2017
 */


#define bitmap_getbit(bitmap_, i_) (bitmap_[i_ >> 3] & (1 << (i_ & 0x07)))
static inline void bitmap_setbit(char *bitmap_, int i_, int val_) {
  if (val_)
    bitmap_[(i_ >> 3)] |= (1 << (i_ & 0x07));
  else
    bitmap_[(i_ >> 3)] &= ~(1 << (i_ & 0x07));
}


#ifndef STRUCT_INODE
#define STRUCT_INODE

typedef struct inode{

  int id;
  char dir_path[99];
  char file_path[132];
  char type; //This will be either "F" for file or "D" for directory.
  struct inode * parent; //Pointer to the directory where the inode is contained.

  //Variables for directories:
  struct inode * contents[10]; //This will only be used in the case that it is the inode for a directory.

  //Variables for files:
  char opened; //This will be either "Y" or "N".
  int seek_ptr; //Seek pointer for the file.
  int block; //Will define the block where the file content is stored.

} inode;

#endif
