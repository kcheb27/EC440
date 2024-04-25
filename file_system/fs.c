
#include "disk.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#define GET 0x01
#define SET 0x02
#define CLEAR 0x04
#define MAX_FILE_DESCRIPTOR 32
#define MAX_FILE_NUM 64
#define MAX_FILE_SIZE (1 << 24)
#define MAX_NAME_LENGTH 15
#define INODES_NUM 256
#include <stdio.h>

struct super_block {
    uint16_t used_block_bitmap_count;
    uint16_t used_block_bitmap_offset;
    uint16_t inode_metadata_blocks;
    uint16_t inode_metadata_offset;
    uint16_t directory_offset;
    uint16_t directory_blocks;
    uint16_t directory_size;

};
struct Inode {
    bool used;
    int offset [INODES_NUM];
    uint16_t file_size;
};
struct dir_entry{
    bool used;
    int inode_number;
    char file_name[MAX_NAME_LENGTH];
};

struct file_descriptor{
    bool used;
    int inode_num;
    int offset; 
};

// globals
uint8_t block_bitmap[DISK_BLOCKS/8];
struct super_block Sblock;
struct Inode inode_table[MAX_FILE_NUM];
bool mounted = false;
struct dir_entry directory[MAX_FILE_NUM];
struct file_descriptor descriptors[MAX_FILE_DESCRIPTOR];

//

//bitmap operator
/*
uint8_t bitmap_op(int flags){
}
*/
///
bool checkBit(uint8_t num, int bitPosition) {
    // Left shift 1 by bitPosition to create a mask with a single bit set at the given position
    uint8_t mask = 1 << bitPosition;
    
    // Perform bitwise AND operation between the number and the mask
    // If the result is non-zero, it means the bit is set
    if ((num & mask) != 0) {
        return true; // Bit is set
    } else {
        return false; // Bit is not set
    }
}
void setBit(uint8_t *num, int bitPosition) {
    // Left shift 1 by bitPosition to create a mask with a single bit set at the given position
    uint8_t mask = 1 << bitPosition;
    
    // Perform bitwise OR operation between the number and the mask to set the bit
    *num |= mask;
}

// Function to clear a specific bit in an 8-bit integer
void clearBit(uint8_t *num, int bitPosition) {
    // Left shift 1 by bitPosition to create a mask with a single bit set at the given position
    uint8_t mask = 1 << bitPosition;
    
    // Perform bitwise AND operation between the number and the complement of the mask to clear the bit
    *num &= ~mask;
}


int make_fs(const char *disk_name){
    if(make_disk(disk_name) == -1){
        return -1;
    }
    if(open_disk(disk_name) == -1){
        printf("failed open");
        return -1;
    }
    //initialize file descriptor
    for ( int i = 0; i<MAX_FILE_DESCRIPTOR;i++){
        descriptors[i].inode_num = -1;
        descriptors[i].used = false;
        descriptors[i].offset = 0;
    }


    struct super_block Super;
    struct Inode empty_inode[MAX_FILE_NUM];
    uint8_t empty_bitmap[DISK_BLOCKS/8];
    struct dir_entry empty_dir[MAX_FILE_NUM];

    for (int i = 0; i < (DISK_BLOCKS/8); i++){
        //empty bitmap
        empty_bitmap[i] = 0;
    }
    for(int j = 0; j <MAX_FILE_NUM; j++){
        for (int i =0; i < 256; i++){
            //empty inodes
            empty_inode[j].offset[i] = -1;
            empty_inode[j].used = false;
            empty_inode[j].file_size = 0;
        }
        //empty directory
        empty_dir[j].inode_number = -1;
        empty_dir[j].used = false;
        char filename[] = ""; // Create a character array for the filename
        int max_length = sizeof(empty_dir[j].file_name) - 1; // Maximum length of file_name
        strncpy(empty_dir[j].file_name, filename, max_length); // Copy at most max_length characters
        empty_dir[j].file_name[max_length] = '\0'; // Ensure null termination
    }
    //Set Super block data
    Super.directory_blocks = ceil(sizeof(empty_dir)/BLOCK_SIZE);
    Super.inode_metadata_blocks = ceil(sizeof(empty_inode)/BLOCK_SIZE);
    Super.used_block_bitmap_count = ceil(sizeof(empty_bitmap)/BLOCK_SIZE);
    Super.directory_offset = 1;
    Super.used_block_bitmap_offset = Super.directory_offset + Super.directory_blocks;
    Super.directory_size = sizeof(empty_dir);
    Super.inode_metadata_offset = Super.used_block_bitmap_offset + Super.used_block_bitmap_count;
    char buffer[BLOCK_SIZE];
    //write inode

    long bits_left = sizeof(empty_inode);
    int current_block_offset = 0;
    int current_bit_offset = 0;
    while (bits_left > 0 ){
        if(bits_left > BLOCK_SIZE){
            //copy next chunk of Block sized data
            memcpy(buffer,(char *)empty_inode + current_bit_offset,BLOCK_SIZE);
            bits_left = bits_left - BLOCK_SIZE;
            //update offset to next chunk of data 
            current_bit_offset += BLOCK_SIZE;
        }else{
            //copy last chunk of data
            memcpy(buffer,(char *)empty_inode + current_bit_offset,bits_left);
            bits_left = bits_left - bits_left;
            
        }
        //write to the block
        block_write(Super.inode_metadata_offset+ current_block_offset,buffer);
        current_block_offset += 1;
        
    }

    //write bitmap
    for(int i = 0; i<Super.used_block_bitmap_count;i++){
        memcpy(buffer,(char *)block_bitmap + (i * BLOCK_SIZE),BLOCK_SIZE);
        if(block_write(Super.used_block_bitmap_offset + i, buffer)){
            return -1;
        }
    }

    //write empty dir
    memcpy(buffer,(char*)empty_dir,sizeof(empty_dir));
    if(block_write(Super.directory_offset,buffer)){
        return -1;
    }
    //write SUPER
    memcpy(buffer,(char *)&Super,sizeof(Super));
    if(block_write(0,&Super) == -1){
        printf("failed Super write");
        return -1;
    }

    //close
    if(close_disk(disk_name) == -1){
        return -1;
    }
    //free buffer
    //free(buffer);
    return 0;
}
int mount_fs(const char *disk_name){
    
    char buffer[BLOCK_SIZE];

    if(open_disk(disk_name) == -1){
        return -1;
    }

    if(mounted == true){
        close_disk(disk_name);
        return -1;
    }

    //load Superblock
    if(block_read(0,buffer) == -1){
        return -1;
    }
    memcpy(&Sblock,buffer,sizeof(struct super_block));
    //load Directory
    if(block_read(Sblock.directory_offset,buffer) == -1){
        return -1;
    }
    memcpy(directory,buffer,sizeof(directory));
    
    //load bitmap
    long bits_left = sizeof(block_bitmap);
    long current_block_offset = 0;
    long current_bit_offset = 0;


    while(bits_left > 0){
        if(block_read(Sblock.used_block_bitmap_offset + current_block_offset,buffer)==-1){
            return -1;
        }

        if(bits_left > BLOCK_SIZE){
            memcpy(block_bitmap + current_bit_offset,buffer,BLOCK_SIZE);
            bits_left = bits_left - BLOCK_SIZE;
            current_bit_offset += BLOCK_SIZE;
        }else{
            memcpy(block_bitmap + current_bit_offset,buffer,bits_left);
            bits_left = bits_left - bits_left;
        }
        current_block_offset += 1;
        
    }
    //load Inode
    current_block_offset =0;
    current_bit_offset = 0;
    bits_left = sizeof(inode_table);
    while(bits_left > 0){
        if(block_read(Sblock.inode_metadata_offset + current_block_offset,buffer)==-1){
            return -1;
        }

        if(bits_left > BLOCK_SIZE){
            memcpy((char*)inode_table + current_bit_offset,buffer,BLOCK_SIZE);
            bits_left = bits_left - BLOCK_SIZE;
            current_bit_offset += BLOCK_SIZE;
        }else{
            memcpy((char*)inode_table + current_bit_offset,buffer,bits_left);
            bits_left = bits_left - bits_left;
        }
        current_block_offset += 1;
        
     }  

    //cleanup stuff
    mounted = true;
    
    return 0;
}
int umount_fs(const char *disk_name){

    if(mounted == false){
        return -1;
    }
    //clear all the variables inode
    for (int i = 0; i< MAX_FILE_NUM; i++){
        for (int j = 0; j<INODES_NUM; j++){
            inode_table[i].offset[j] = -1;
        }
    }
    //clear Super block
    Sblock.directory_blocks = 0;
    Sblock.directory_offset = 0;
    Sblock.inode_metadata_blocks = 0;
    Sblock.inode_metadata_offset = 0;
    Sblock.directory_size = 0;
    Sblock.used_block_bitmap_count = 0;
    Sblock.used_block_bitmap_offset = 0;

    // //clear bitmap
    // for (int i; i<(DISK_BLOCKS/8);i++){
    //     block_bitmap[i] = 0;
    // }

    mounted = false;
    close_disk(disk_name);
    return 0;
}
int fs_open(const char *name){
    if(!mounted){
        return -1;
    }
    
    int descriptor_num = -1;
    for (int i = 0; i<MAX_FILE_DESCRIPTOR; i++){
        if(descriptors[i].used == false){
            descriptor_num=i;
            break;
        }
    }
    if (descriptor_num == -1){
        return -1;
    }
    int file_num = -1;
    for( int i = 0; i<MAX_FILE_NUM; i++){
        if(strcmp(name,directory[i].file_name) == 0){
            file_num = i;
            break;
        }
    }
    if(file_num == -1){
        return -1;
    }
    descriptors[descriptor_num].inode_num = directory[file_num].inode_number;
    descriptors[descriptor_num].used = true;
    descriptors[descriptor_num].offset = 0;


    return descriptor_num;
}
int fs_close(int fildes){

    if(fildes>=MAX_FILE_DESCRIPTOR){
        printf("too big");
        return -1;
    }

    if(descriptors[fildes].used == false){
        printf("no used");
        return -1;
    }
    descriptors[fildes].inode_num = -1;
    descriptors[fildes].used = false;
    descriptors[fildes].offset = -1;
    return 0;
}
int fs_create(const char *name){
    //check
    if(mounted == false){
        return -1;
    }
    if(strlen(name)>15){
        return -1;
    }
    //check for name redudancy
    for(int i = 0; i<MAX_FILE_NUM; i++){
        if(strcmp(name,directory[i].file_name) == 0){
            return -1;
        }
    }
    int directory_index = -1;
    //find place in directory
    for(int i = 0; i<MAX_FILE_NUM; i++){
        if(directory[i].used == false){
            directory[i].used = true;
            strcpy(directory[i].file_name,name);
            directory_index = i;
            break;
        }
    }
    if(directory_index == -1){
        return -1;
    }
    for(int i = 0; i<MAX_FILE_NUM; i++){
        if(inode_table[i].used == false){
            directory[directory_index].inode_number = i;
        }
    }
    //
    return 0;
}
int fs_delete(const char *name){
    
    if(mounted == false){
        return -1;
    }
    if(strlen(name)>15){
        return -1;
    }
    int dir_index = -1;
    for(int i = 0; i<MAX_FILE_NUM; i++){
        if(strcmp(directory[i].file_name,name) == 0){
            
            dir_index = i;
            //clear inode and bitmap stuff
            inode_table[directory[i].inode_number].used = false;

            for (int j = 0; j<INODES_NUM; j++){
                //clear bitmap
                if(inode_table[directory[i].inode_number].offset[j]!= -1){
                    //clear bitmap index
                    int currentoffset = inode_table[directory[i].inode_number].offset[j];
                    int index =((double)currentoffset/8.0);
                    int bit = currentoffset % 8;
                    
                    clearBit(block_bitmap,currentoffset);
                   // printf("bit is: %d",checkBit(block_bitmap,currentoffset));
                }
                inode_table[directory[i].inode_number].offset[j]= -1;
                
            }
            break;
        }
    }

    if (dir_index == -1){
        return -1;
    }
    for(int i = 0; i<MAX_FILE_DESCRIPTOR; i++){
        if(directory[dir_index].inode_number == descriptors[i].inode_num){
            return -1;
        }
    }
    directory[dir_index].used = false;
    memset(directory[dir_index].file_name, 0,sizeof(directory[dir_index].file_name));
    directory[dir_index].inode_number = -1;

    return 0;
}
int fs_read(int fildes, void *buf, size_t nbyte){
    if(!mounted){
        return -1;
    }
    if(fildes>MAX_FILE_DESCRIPTOR){
        return -1;
    }
    if(descriptors[fildes].used == false){
        return -1;
    }
    size_t return_val = nbyte;
    int bits_left = nbyte;
    int inode_num = descriptors[fildes].inode_num;
    struct Inode current_inode = inode_table[inode_num];
    if(nbyte > current_inode.file_size){
        bits_left = current_inode.file_size;
        return_val = current_inode.file_size;
    }

    //num of bits left
    
    int current_block = descriptors[fildes].offset / BLOCK_SIZE;
    char buffer[BLOCK_SIZE];
    if(bits_left > BLOCK_SIZE){
        while(bits_left > 0){
            if(current_inode.offset[current_block]== -1){
                break;
            }

            block_read(current_inode.offset[current_block],buffer);
            if(bits_left>BLOCK_SIZE){
                memcpy(buf + (current_block * BLOCK_SIZE),buffer,BLOCK_SIZE);
                bits_left = bits_left - BLOCK_SIZE;
            }else{
                memcpy(buf + (current_block)*BLOCK_SIZE,buffer,bits_left);
                bits_left = bits_left - bits_left;
            }
            current_block += 1;
            
        }
    }else{
        //just read one block
        block_read(current_inode.offset[current_block],buffer);
        memcpy(buf, buffer, bits_left);

    }

    return return_val;
}

int fs_write(int fildes, void *buf, size_t nbyte){
    
    if(descriptors->used == false){
        return -1;
    }
    if(fildes > MAX_FILE_DESCRIPTOR){
        return -1;
    }
    //lseek offset handling
    int num_blocks_off = (descriptors[fildes].offset/BLOCK_SIZE);
    int num_bits = descriptors[fildes].offset % BLOCK_SIZE;
    if(descriptors[fildes].offset + nbyte > MAX_FILE_SIZE){
        return -1;
    }
    
    

    //
    struct Inode curr_inode = inode_table[descriptors[fildes].inode_num];
    char buffer[BLOCK_SIZE];
    int bits_left = nbyte;
    int offset_index = 0;
    block_read(buffer,curr_inode.offset[(descriptors[fildes].offset/BLOCK_SIZE)-1]);
    memcpy(buffer,buf,descriptors[fildes].offset%BLOCK_SIZE );

    while(bits_left > 0){

        //find empty bit
        int empty_bit = -1;
        for (int i = 0; i < DISK_BLOCKS/8; i++){
            for(int j = 0; j < 8; j++){
                if(checkBit(block_bitmap,(i * 8) + j)==false){
                    empty_bit = (i * 8) + j; 
                    setBit(block_bitmap,empty_bit);
                    break;
                }
            }
        }
        if(empty_bit == -1){
            break;
        }
        //copy into buffer
        if(bits_left>BLOCK_SIZE){
            memcpy(buffer,buf + offset_index *BLOCK_SIZE,BLOCK_SIZE);
            bits_left = bits_left - BLOCK_SIZE;
        }else{
            memcpy(buffer,buf + offset_index *BLOCK_SIZE, bits_left);
            bits_left = bits_left - bits_left;
        }

        block_write(empty_bit,buffer);
        curr_inode.offset[offset_index + (descriptors[fildes].offset/BLOCK_SIZE)] = empty_bit;
        offset_index += 1;
        
    }
    curr_inode.file_size = nbyte - bits_left;
    inode_table[descriptors[fildes].inode_num] = curr_inode;
    //printf("bits left %d", bits_left);
    return nbyte - bits_left;
}
int fs_get_filesize(int fildes){
    if(fildes >MAX_FILE_DESCRIPTOR){
        return -1;
    }
    return inode_table[descriptors[fildes].inode_num].file_size;

}

int fs_listfiles(char ***files){
    if(!mounted){
        return -1;
    }
    int num_of_files = 0;
    for(int i =0; i< MAX_FILE_NUM; i++){
        if(directory[i].used){
            num_of_files++;
        }
    }
    *files = (char **) calloc(num_of_files + 1, sizeof(char *));
    if ( *files == NULL){
        return -1;
    }
    for (int i = 0; i < num_of_files; i++){
        (*files)[i] = (char *)calloc(16, sizeof(char));
        if ((*files)[i] == NULL){
            for (int j = 0; j < i; j++){
                free((*files)[j]);

            }
            free(*files);
            return -1;
        }
        strncpy((*files)[i], directory[i].file_name, 15);
        
        
    }
    (*files)[num_of_files]=NULL;

    return 0;
}
int fs_lseek(int fildes, off_t offset){
    if(offset > MAX_FILE_SIZE || offset <0){
        return -1;
    }
    if(fildes>MAX_FILE_DESCRIPTOR){
        return -1;
    }
    if(offset>inode_table[descriptors[fildes].inode_num].file_size){
        return -1;
    }
    descriptors[fildes].offset = offset;

    return 0;
}
int fs_truncate(int fildes, off_t length){
    if(!mounted){
        return -1;
    }
    if(fildes > MAX_FILE_SIZE || fildes <0){
        return -1;
    }
    if(descriptors[fildes].used == false){
        return -1;
    }
    if(length <0){
        return -1;

    }
    struct Inode curr_inode = inode_table[descriptors[fildes].inode_num];
    if(length > curr_inode.file_size){
        return -1;
    }
    int num_blocks = length/ BLOCK_SIZE;
    int num_bits = length%BLOCK_SIZE;
    
    clearBit(&block_bitmap[curr_inode.offset[num_blocks + 1]/8],curr_inode.offset[num_blocks + 1]%8);

    curr_inode.file_size =length;
    inode_table[descriptors[fildes].inode_num] = curr_inode;
    return 0;
}