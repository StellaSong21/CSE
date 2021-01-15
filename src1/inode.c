#include <stdio.h>
#include <assert.h>

#include "inode.h"
#include "diskimg.h"
#include "ino.h"
#include "unixfilesystem.h"

/* the number of inodes in a disk sector (e.g. block). */
#define INODE_COUNT_PER_SECTOR (DISKIMG_SECTOR_SIZE/(sizeof(struct inode))) 
/* the number of address in a disk sectior (e.g. block) */
#define ADDR_PER_SECTOR (DISKIMG_SECTOR_SIZE/sizeof(uint16_t))
/* the max number of blocks that the file's block info only in inodes, without indirect block */
#define INDIRECT_BLOCK_START 8
/* the first block in the doubly block */
#define DOUBLY_BLOCK_START (ADDR_PER_SECTOR*7)


/**
 * Fetches the specified inode from the filesystem. 
 * Returns 0 on success, -1 on error.  
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
	int fd = fs->dfd;
	struct inode inodes[INODE_COUNT_PER_SECTOR];
	int offset, sectorNum;

	/* Determine the block where the inode is located. */
	sectorNum = (inumber-1) / INODE_COUNT_PER_SECTOR + INODE_START_SECTOR;

	/* Read the contents of the block from corresponding diskimg to inodes,
	   which is an array of inodes. */
	if(diskimg_readsector(fd, sectorNum, inodes) != DISKIMG_SECTOR_SIZE){
		fprintf(stderr, "Error: failed to read the block.\n");
        return -1;
	} else{
		*inp = inodes[(inumber - 1) % INODE_COUNT_PER_SECTOR];
	}
	return 0;
}


/**
 * Given an index of a file block, retrieves the file's actual block number
 * of from the given inode.
 *
 * Returns the disk block number on success, -1 on error.  
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) { 
	int fd = fs->dfd;

	/* Step 1. Get the number of blocks constituting file. */
	int valid_block = (inode_getsize(inp)-1) / DISKIMG_SECTOR_SIZE + 1; /* Rounding up and down rounding: <N/M> = [(N-1)/M]+1 */

	/* Step 2. Determine if blockNum is within the valid range. */
	if(blockNum >= valid_block || blockNum < 0){
		fprintf(stderr, "Error: blockNum out of range.\n");
		return -1;
	}

	/* Step 3. According to the location of the address of the file's actual block number, discuss. */

	/* Case 1. The actual block number is in the inode. */
	if((inp->i_mode & ILARG) == 0){
		return inp->i_addr[blockNum];
	}

	/* Case 2. The actual block number is in the indirect block. */
	uint16_t layer1_addr[ADDR_PER_SECTOR];
	if(blockNum < DOUBLY_BLOCK_START){
		if(diskimg_readsector(fd, inp->i_addr[blockNum/ADDR_PER_SECTOR], layer1_addr) != DISKIMG_SECTOR_SIZE){
			fprintf(stderr, "Error: failed to get layer1_addr.\n");
			return -1;
		}
		return layer1_addr[blockNum % ADDR_PER_SECTOR];
	}

	/* Case 3. The actual block number is in the doubly indirect block. */
	uint16_t layer2_addr[ADDR_PER_SECTOR];
	int temp_addr = blockNum - DOUBLY_BLOCK_START;
	if(diskimg_readsector(fd, inp->i_addr[INDIRECT_BLOCK_START - 1], layer1_addr) != DISKIMG_SECTOR_SIZE){
		fprintf(stderr, "Error: failed to get layer1_addr.\n");
		return -1;
	}
	if(diskimg_readsector(fd, layer1_addr[temp_addr/ADDR_PER_SECTOR], layer2_addr) != DISKIMG_SECTOR_SIZE){
		fprintf(stderr, "Error: failed to get layer2_addr.\n");
		return -1;
	}
	return layer2_addr[temp_addr % ADDR_PER_SECTOR];
}

/**
 * Computes the size in bytes of the file identified by the given inode
 */
int inode_getsize(struct inode *inp) {
	return ((inp->i_size0 << 16) | inp->i_size1);
}
