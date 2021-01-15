#include <stdio.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

/**
 * Fetches the specified file block from the specified inode.
 * Returns the number of valid bytes in the block, -1 on error.
 */
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
	struct inode in;
	int actual_blockNum, ret, valid_block, size;

	/* Step 1. Get the inode of inumber. */
	if(inode_iget(fs, inumber, &in) < 0){
		fprintf(stderr, "Error: failed to read the inode.\n");
		return -1;
	}

	/* Step 2. Get the actual block number of blockNum in the inode of inumber. */
	if((actual_blockNum = inode_indexlookup(fs, &in, blockNum)) < 0){
		fprintf(stderr, "Error: faied to read the block number.\n");
		return -1;
	}
	
	/* Step 3. Read the data of the block to buf. */
	if((ret  = diskimg_readsector(fs->dfd, actual_blockNum, buf)) != DISKIMG_SECTOR_SIZE){
		fprintf(stderr, "Error: failed to read the block.\n");
		return -1;
	}

	size = inode_getsize(&in);
  	valid_block = (size - 1) / DISKIMG_SECTOR_SIZE + 1;

  	/* If the read block is the last block of the file and not all data is valid. */
  	if((blockNum == valid_block - 1) && (size % DISKIMG_SECTOR_SIZE != 0)){
  		return size % DISKIMG_SECTOR_SIZE;
  	}
	
	return ret;
}
