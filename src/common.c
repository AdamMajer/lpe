/*
 * Copyright (c) 2004 Adam Majer
 *
 * This file may be distributed only udner the terms of the GPL license
 * version 2. See COPYING for further details.
 */

#include "common.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int copy( const char *from_file, const char *to_file )
{
    int in, out;
    char buffer[8192];
    struct stat buf;
    ssize_t size_read, total = 0;
    
    if(!( in = open( from_file, O_RDONLY )))
        return -1;
    if(!( out = open( to_file, O_WRONLY | O_TRUNC ))){
        close( in );
        return -1;
    }
    if(!fstat( in, &buf )){
        close( in );
        unlink( to_file );
        close( out );
        return -1;
    }
    
    while((size_read = read( in, buffer, 8192 )) > -1 ){
        total += size_read;
        if( write( out, buffer, size_read ) != size_read ){
            unlink( to_file );
            break; /* error, cannot write */
        }
        
        if( total == buf.st_size ) /* copied the entire file */
            break;
        if( size_read < 1 ){
            unlink( to_file );
            break;
        }
    }

    close( in );

    /* attempt to set same mode and priv. as the source */
    fchmod( out, buf.st_mode );
    fchown( out, buf.st_uid, buf.st_gid );
    close( out );

    /* check if we have copied the file */
    if( close( open( to_file, O_RDONLY )))
        return 0;
    
    return -1; /* An error occured during copy */
}
