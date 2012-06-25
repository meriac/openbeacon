/***************************************************************
 *
 * OpenPCD.org - Sniffer Decoder for ISO15693 / iCLASS
 *
 * assumes a sampling rate of 15.625MHz @ 16bit (PicoScope)
 *
 * Copyright 2012 Milosch Meriac <meriac@bitmanufaktur.de>
 *
 ***************************************************************/

/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published
 by the Free Software Foundation; version 3.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

FILE *fin, *fout;

#define CARRIER_FREQUENCY (13.56*1000*1000)
#define FILTER_FREQUENCY (CARRIER_FREQUENCY/32)
#define SAMPLING_FREQUENCY (15625000)
#define COMB_DEFAULT ((int)(SAMPLING_FREQUENCY/FILTER_FREQUENCY))
#define COMB_SIZE ((int)((SAMPLING_FREQUENCY/COMB_DEFAULT)+1))
#define COMB_HIGHPASS 32

int16_t m_buffer_in[SAMPLING_FREQUENCY];
int16_t m_buffer_out[COMB_SIZE*2];
int16_t m_comb[COMB_DEFAULT];
int16_t m_min[COMB_HIGHPASS];

int
main (int argc, char *argv[])
{
	int count, res, seconds, i, written, pos_comb, lowpass, lowpass_prev,
		pos_min;
	double delta;
	uint8_t bit, bit_prev;
	int16_t *data_in, *data_out, data, min;

	if (argc != 3)
	{
		printf ("Usage: %s infile.bin outfile.bin\n", argv[0]);
		return -1;
	}

	if ((fin = fopen (argv[1], "rb")) == NULL)
	{
		fprintf (stderr, "ERROR: Can't open infile:'%s'\n", argv[1]);
		res = -2;
	}
	else
	{
		if ((fout = fopen (argv[2], "wb")) == NULL)
		{
			fprintf (stderr, "ERROR: Can't open outfile:'%s'\n", argv[2]);
			res = -3;
		}
		else
		{
			i = pos_comb = pos_min = seconds = res = lowpass = lowpass_prev =
				0;
			delta = 0;
			bit_prev = 0;
			memset (&m_comb, 0, sizeof (m_comb));
			memset (&m_min, 0, sizeof (m_min));

			/* output header */
			printf ("DeltaTime[ns],SignalEnvelope\n");

			while ((count =
					fread (&m_buffer_in, sizeof (m_buffer_in[0]),
						   SAMPLING_FREQUENCY, fin)) > 0)
			{
				data_in = m_buffer_in;
				data_out = m_buffer_out;
				written = 0;

				while (count--)
				{
					data = *data_in++;
					lowpass -= m_comb[pos_comb];
					m_comb[pos_comb] = data;
					lowpass += data;

					pos_comb++;
					if (pos_comb == COMB_DEFAULT)
					{
						pos_comb = 0;

						/* skip COMB_DEFAULT values in output */
						data = lowpass / COMB_DEFAULT;

						/* add latest value to highpass filter array */
						m_min[pos_min++] = data;
						if (pos_min == COMB_HIGHPASS)
							pos_min = 0;

						/* find lowest value in array */
						min = 0x7FFF;
						for (i = 0; i < COMB_HIGHPASS; i++)
							if (m_min[i] < min)
								min = m_min[i];

						/* remove DC offset */
						data -= min;

						/* differential compression on STDOUT */
						bit = data > (0.2 * 0x7FFF) ? 1 : 0;
						if (bit != bit_prev)
						{
							printf ("%i,%i\n", (int) delta, bit_prev);
							delta = 0;
							bit_prev = bit;
						}
						delta += ((1 / FILTER_FREQUENCY) * 1000000000);

						/* output filtered file in stereo */
						*data_out++ = data;
						*data_out++ = bit * 0x6FFF;
						written++;
					}
				}

				fprintf (stderr, "\nprocessed %03i seconds (written %i)",
						 ++seconds, written);
				/* write remaining data */
				fwrite (&m_buffer_out, sizeof (m_buffer_out[0]), written,
						fout);
			}
			fprintf (stderr, ", filtered @ %i Hz [DONE]\n",
					 (int) (SAMPLING_FREQUENCY / COMB_DEFAULT));
			fclose (fout);
		}
		fclose (fin);
	}
	return res;
}
