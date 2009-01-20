/* generate_splash -- Converts an image in PGM format into a C file containing compressed chunks
 * (Based on testmini.c from the minilzo distribution)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pgm.h>


#include "lzo/minilzo.h"
#include "splash.h"

#define IN_LEN MAX_PART_SIZE
#define OUT_LEN     (IN_LEN + IN_LEN / 16 + 64 + 3)

static unsigned char __LZO_MMODEL in  [ IN_LEN ];
static unsigned char __LZO_MMODEL out [ OUT_LEN ];


/* Work-memory needed for compression. Allocate memory in units
 * of `lzo_align_t' (instead of `char') to make sure it is properly aligned.
 */

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem,LZO1X_1_MEM_COMPRESS);


/*************************************************************************
//
**************************************************************************/

unsigned char scratch_space[MAX_PART_SIZE];
int main(int argc, char *argv[])
{
    int r;
    lzo_uint in_len;
    lzo_uint out_len;
    lzo_uint new_len;
    char *filename = NULL;
    
    /* We will cache all output parts
     * part_pointers is an array with pointers to the data of the parts that were actually 
     * output
     * part_lengths is an array with the lengths of the compressed data in part_pointers
     * part_pointer_len is the length of both arrays */
    unsigned char **part_pointers = NULL;
    size_t *part_lengths = NULL;
    size_t *part_in_lengths = NULL;
    size_t part_pointers_len=0;
    /* part_indices (containing part_indices_len entries) is an array of indices into
     * part_pointers containing the parts that should have been output, in order */
    int *part_indices;
    size_t part_indices_len=0;

    if (argc < 2) {
    	fprintf(stderr, "Need at least 1 argument: The name of the PGM input file\n");
    	return 1;
    }
    filename = argv[1];
    fprintf(stderr, "Working on %s\n", filename);
    
    FILE *fp = fopen(filename, "rb");
    if(fp == NULL) {
    	fprintf(stderr, "Couldn't open %s\n", filename);
    	return 1;
    }
    
    char *destination_basename = "splash_";
    if(argc > 2)
    	destination_basename = argv[2];
    
    pm_init(argv[0], 0);
    int cols, rows, maxval;
    gray **data = pgm_readpgm(fp, &rows, &cols, &maxval);
    int fullsize = rows*cols;
    part_indices_len = (fullsize+IN_LEN-1)/IN_LEN;
    part_indices = calloc(part_indices_len, sizeof(part_indices[0]));
    if(part_indices == NULL) {
    	fprintf(stderr, "Can't allocate index table, abort\n");
    	return 1;
    }
    
    fprintf(stderr, "%i rows by %i columns (%i bytes)\n", rows, cols, fullsize);

    fprintf(stderr, "LZO real-time data compression library (v%s, %s).\n",
           lzo_version_string(), lzo_version_date());
    fprintf(stderr, "Copyright (C) 1996-2008 Markus Franz Xaver Johannes Oberhumer\nAll Rights Reserved.\n\n");

    if (lzo_init() != LZO_E_OK)
    {
        fprintf(stderr, "internal error - lzo_init() failed !!!\n");
        fprintf(stderr, "(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable `-DLZO_DEBUG' for diagnostics)\n");
        return 3;
    }

    int pos=0, part=0, compressed_sum = 0;
    while(pos < fullsize) {
        int inpos=0;
        in_len = IN_LEN;
        if(pos + in_len > fullsize)
        	in_len = fullsize-pos;
        while(inpos < in_len) {
        	int x=pos/rows, y=pos%rows;
        	    in[inpos] = data[x][y];
        	    inpos++; pos++;
        }

        r = lzo1x_1_compress(in,in_len,out,&out_len,wrkmem);
        if (r == LZO_E_OK)
            fprintf(stderr, "compressed %lu bytes into %lu bytes\n",
                (unsigned long) in_len, (unsigned long) out_len);
        else
        {
            /* this should NEVER happen */
            fprintf(stderr, "internal error - compression failed: %d\n", r);
            return 2;
        }
        /* check for an incompressible block */
        if (out_len >= in_len)
        {
            fprintf(stderr, "This block contains incompressible data.\n");
            //return 0;
        }
        
        /* First: Try to find the compressed data in our already existing list of compressed
         * parts */
        int i, found = 0;
        for(i=0; i<part_pointers_len; i++) {
        	if(out_len == part_lengths[i] && (memcmp(out, part_pointers[i], out_len) == 0)) {
        		/* Found one, enter its index into the list of part indices and abort search */
        		found = 1;
        		part_indices[part] = i;
        		break;
        	}
        }
        if(!found) {
        	/* No match was found in the preceding loop, instead add the compressed part to our list */
        	part_pointers = realloc(part_pointers, (part_pointers_len+1)*sizeof(part_pointers[0]));
        	part_lengths = realloc(part_lengths, (part_pointers_len+1)*sizeof(part_lengths[0]));
        	part_in_lengths = realloc(part_in_lengths, (part_pointers_len+1)*sizeof(part_in_lengths[0]));
        	if(part_pointers == NULL || part_lengths == NULL || part_in_lengths == NULL) {
        		fprintf(stderr, "Unable to increase part cache, abort\n");
        		return 1;
        	}
        	
        	part_lengths[part_pointers_len] = out_len;
        	part_in_lengths[part_pointers_len] = in_len;
        	part_pointers[part_pointers_len] = malloc(out_len);
        	if(part_pointers[part_pointers_len] == NULL) {
        		fprintf(stderr, "Unable to allocate part cache entry, abort\n");
        		return 1;
        	}
        	memcpy(part_pointers[part_pointers_len], out, out_len);
        	
        	part_pointers_len++;
        	
        	/* Finally add this part to the list of part indices */
        	part_indices[part] = part_pointers_len-1;
        }
        
        part++;
    }

    printf("#include \"splash.h\"\n");
    
    int i;
    /* First print all the cached parts */
    for(part=0; part<part_pointers_len; part++) {
    	printf("static const struct splash_part part%i = {\n\t%i,\n\t%i,\n\"", part, 
    			part_lengths[part], part_in_lengths[part]);
    	for(i=0; i<part_lengths[part]; i++) {
    		printf("\\x%02X", part_pointers[part][i]);
    		if(i%20 == 19) printf("\"\n\"");
    	}
    	printf("\"};\n");
    	compressed_sum += part_lengths[part];
    }
	
    /* Then print all the references to the cache */
    printf("\n\nstatic const struct splash_part * const %s_parts[] = {\n", destination_basename);
    for(i=0; i<part_indices_len; i++) {
    	printf("\t&part%i,\n", part_indices[i]);
    }
    printf("\t0\n");
    printf("};\n");
    /* Then print header information */
    printf("\nconst struct splash_image %s_image = {\n", destination_basename);
    printf("\t.width=%i,\n", rows);
    printf("\t.height=%i,\n", cols);
    printf("\t.splash_parts=%s_parts,\n", destination_basename);
    printf("};\n");
    fprintf(stderr, "compressed sum: %i\n", compressed_sum);
    return 0;
}

/*
vi:ts=4:et
*/

