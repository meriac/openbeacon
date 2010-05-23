/***************************************************************
 *
 * OpenPCD.org - main entry for LPC flashing tool
 *
 * Copyright 2010 Milosch Meriac <meriac@bitmanufaktur.de>
 *
 ***************************************************************

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 3.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */

/* Library includes. */
#include <stdio.h>

int main(int argc, char **argv)
{
	int data;
	unsigned int value, sum, i, neg;
	FILE *input, *output;

	if (argc != 3) {
		fprintf(stderr,
				"\nusage: %s inputfirmware.bin \"/media/CRP DISABLD/firmware.bin\"\n\n",
				argv[0]);
		return 1;
	}
	// open input file
	if ((input = fopen(argv[1], "rb")) == NULL) {
		fprintf(stderr, "\ncan't open input file \"%s\"\n\n", argv[1]);
		return 2;
	}
	// open output file
	if ((output = fopen(argv[2], "r+b")) == NULL) {
		fprintf(stderr, "\ncan't open output file \"%s\"\n\n", argv[2]);
		fclose(input);
		return 2;
	}

	value = sum = neg = i = 0;
	while ((data = fgetc(input)) != EOF) {
		// output checksum
		if (i >= 0x1C && i < (0x1C + 4)) {
			fputc(neg & 0xFF, output);
			neg >>= 8;
		} else {
			// output unmodified data
			fputc(data, output);

			// calculate checksum for first 0x1C bytes
			if (i < 0x1C) {
				value = (value >> 8) | (((unsigned int) data) << 24);
				if ((i & 3) == 3) {
					sum += value;
					neg = ~sum + 1;
				}
			}
		}

		i++;
	}

	// close & flush files
	fclose(input);
	fflush(output);
	fclose(output);

	return 0;
}
