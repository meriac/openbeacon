/*
 *  sorting taken from linux kernel 2.6.1 - linux/fs/ext3/namei.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/namei.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Big-endian to little-endian byte-swapping/bitmaps by
 *        David S. Miller (davem@caip.rutgers.edu), 1995
 *  Directory entry file type support and forward compatibility hooks
 *  	for B-tree directories by Theodore Ts'o (tytso@mit.edu), 1998
 *  Hash Tree Directory indexing (c)
 *  	Daniel Phillips, 2001
 *  Hash Tree Directory indexing porting
 *  	Christopher Li, 2002
 *  Hash Tree Directory indexing cleanup
 * 	Theodore Ts'o, 2002
 */

#include <beacontypes.h>
#include <board.h>
#include "sort.h"

void sort (unsigned int *array, unsigned int count)
{
    unsigned int *p, *q, *top = array + count - 1,tmp;
    int more;
    
    /* Combsort until bubble sort doesn't suck */
    while (count > 2)
    {
	count = count*10/13;
        if (count - 9 < 2) /* 9, 10 -> 11 */
    	    count = 11;
        for (p = top, q = p - count; q >= array; p--, q--)
    	    if (*p < *q)
	    {
		tmp=*p;
		*p=*q;
		*q=tmp;
	    }
    }

    /* Garden variety bubble sort */
    do {
        more = 0;
        q = top;
        while (q-- > array)
	{
	    tmp=q[1];
	    
            if (tmp >= q[0])	    
	        continue;

	    q[1]=q[0];
	    q[0]=tmp;
            more = 1;
	}
    } while(more);
}
