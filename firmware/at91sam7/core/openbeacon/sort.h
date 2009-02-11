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

#ifndef __SORT_H__
#define __SORT_H__

extern void sort (unsigned int *array, unsigned int count);

#endif/*__SORT_H__*/
