#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define FILEINFO_PER_BLOCK (DISKIMG_SECTOR_SIZE/sizeof(struct direntv6)) 


/**
 * Looks up the specified name (name) in the specified directory (dirinumber).  
 * If found, return the directory entry in space addressed by dirEnt.  Returns 0 
 * on success and something negative on failure. 
 */
int directory_findname(struct unixfilesystem *fs, const char *name, int dirinumber,
					 struct direntv6 *dirEnt) {
	struct inode in;
	struct inode *inp = &in;
	unsigned int i, j, valid_block; 
	int valid_bytes;
	struct direntv6 dir[FILEINFO_PER_BLOCK];
	struct direntv6 cur_file;

	/* Get the inode of dirinumber. */
	if(inode_iget(fs, dirinumber, inp) < 0){
		fprintf(stderr, "Error: failed to read the inode. \n");
		return -1;
	}

	/* If is not a directory. */
	if((inp-> i_mode & IFDIR) == 0){
		fprintf(stderr, "Error: is not a dir. \n");
		return -1;
	}

	/* Get the number of blocks constituting the dircetory. */
	valid_block = (inode_getsize(inp)-1) / DISKIMG_SECTOR_SIZE + 1;
	
	/* Find the file of name in the blocks of the directory. */
	for(i = 0; i < valid_block; i++){

		/* Get the ith block of the directory. */
		if((valid_bytes = file_getblock(fs, dirinumber, i, dir)) < 0){
			fprintf(stderr, "Error: failed to read the file. \n");
			return -1;
		}

		/* Search the file of name in the ith block. */
		for(j = 0; j < valid_bytes / sizeof(struct direntv6); j++){
			cur_file = dir[j];
			if(strcmp(cur_file.d_name, name) == 0){
				*dirEnt = cur_file;
				return 0;
			}
		}
	}

	/* Not find the file of name under the directory. */
	fprintf(stderr, "Error: failed to find the file. \n");
	return -1;
}
