
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

/**
 * Returns the inode number associated with the specified pathname.  This need only
 * handle absolute paths.  Returns a negative number (-1 is fine) if an error is 
 * encountered.
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
	/* If not begin with '/', means is not absolute path. */
	if(pathname[0] != '/'){
		fprintf(stderr, "Error: not absolute path.\n");
		return -1;
	}

	char *path = strdup(pathname);
	char *token;
	struct direntv6 dirEnt;
	int inumber = ROOT_INUMBER;

	/* Separate the path with '/' and find the file according the path. */
	for(token = strtok(path, "/"); token != NULL; token = strtok(NULL, "/")){
		if(directory_findname(fs, token, inumber, &dirEnt) < 0){
			fprintf(stderr, "Error: failed to find file %s.\n", token);
			return -1;
		}
		inumber = dirEnt.d_inumber;
	}

	/* Free the strdup(). */
	free(path);
	path = NULL;

	/* If token is NULL, means the path = "/", then return ROOT_INUMBER. */
	return inumber;
}
