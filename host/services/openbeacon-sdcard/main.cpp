#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "openbeacon.h"

#define MX  ( (((z>>5)^(y<<2))+((y>>3)^(z<<4)))^((sum^y)+(tea_key[(p&3)^e]^z)) )
#define DELTA 0x9e3779b9UL

/* proximity tag TEA encryption key */
static const long tea_key[XXTEA_BLOCK_COUNT] = {0x00112233, 0x44556677, 0x8899aabb, 0xccddeeff};

static u_int16_t
crc16 (const unsigned char *buffer, int size)
{
	u_int16_t crc = 0xFFFF;

	if (buffer && size)
		while (size--)
		{
			crc = (crc >> 8) | (crc << 8);
			crc ^= *buffer++;
			crc ^= ((unsigned char) crc) >> 4;
			crc ^= crc << 12;
			crc ^= (crc & 0xFF) << 5;
		}

	return crc;
}

static inline u_int16_t
icrc16 (const unsigned char *buffer, int size)
{
	return crc16 (buffer, size) ^ 0xFFFF;
}

static inline void
xxtea_decode (u_int32_t * v, u_int32_t length, const long *tea_key)
{
	u_int32_t z, y = v[0], sum = 0, e, p, q;

	if (!v || !tea_key)
		return;

	q = 6 + 52 / length;
	sum = q * DELTA;
	while (sum)
	{
		e = (sum >> 2) & 3;
		for (p = length - 1; p > 0; p--)
			z = v[p - 1], y = v[p] -= MX;

		z = v[length - 1];
		y = v[0] -= MX;
		sum -= DELTA;
	}
}

static inline void
shuffle_tx_byteorder (u_int32_t * v, u_int32_t len)
{
	while (len--)
	{
		*v = htonl (*v);
		v++;
	}
}

#ifdef  DEBUG
static void
hex_dump (const void *data, unsigned int addr, unsigned int len)
{
	unsigned int start, i, j;
	const unsigned char *buf;
	char c;

	buf = (const unsigned char *) data;
	start = addr & ~0xf;

	for (j = 0; j < len; j += 16)
	{
		fprintf (stderr, "%08x:", start + j);

		for (i = 0; i < 16; i++)
		{
			if (start + i + j >= addr && start + i + j < addr + len)
				fprintf (stderr, " %02x", buf[start + i + j]);
			else
				fprintf (stderr, "   ");
		}
		fprintf (stderr, "  |");
		for (i = 0; i < 16; i++)
		{
			if (start + i + j >= addr && start + i + j < addr + len)
			{
				c = buf[start + i + j];
				if (c >= ' ' && c < 127)
					fprintf (stderr, "%c", c);
				else
					fprintf (stderr, ".");
			}
			else
				fprintf (stderr, " ");
		}
		fprintf (stderr, "|\n\r");
	}
}
#endif/*DEBUG*/

int
main (int argc, char **argv)
{
	TBeaconLogSighting pkt;
	TBeaconEnvelopeLog out;
	uint16_t crc;
	int seq, reader_id, time, logtime, error;
	FILE *f,*fout;

	if(argc!=3)
	{
		printf("usage: %s logfile.bin outfile.bin\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	if((fout = fopen(argv[2],"wb"))==NULL)
	{
		printf("ERROR: cant't open '%s' for writing\n",argv[1]);
	}

	if((f = fopen(argv[1],"rb"))==NULL)
	{
		printf("ERROR: cant't open '%s' for reading\n",argv[1]);
	}

	error = 0;
	while(fread(&pkt,sizeof(pkt),1,f) == 1)
	{
		/* check log protocol */
		if(pkt.hdr.protocol != BEACONLOG_SIGHTING)
		{
			printf("ERROR: unknown protocol 0x%02X\n",pkt.hdr.protocol);
			break;
		}

		/* check log header size */
		if(ntohs(pkt.hdr.size) != sizeof(pkt))
		{
			printf("ERROR: unknown protocol size %i!=%i\n",(int)ntohs(pkt.hdr.size),(int)sizeof(pkt));
			break;
		}

		/* post packet to log file queue with CRC */
		crc = icrc16 ((u_int8_t *) & pkt.hdr.protocol,
			sizeof (pkt) - sizeof (pkt.hdr.icrc16));

		/* verify logged CRC */
		if(ntohs(pkt.hdr.icrc16)!=crc)
		{
			printf("ERROR: incorrct CRC 0x%04X!=0x%04X\n",ntohs(pkt.hdr.icrc16),crc);
			break;
		}

		/* log entry */
		reader_id = ntohs(pkt.hdr.reader_id);
		time = ntohl(pkt.timestamp);

		/* decode packet */
		shuffle_tx_byteorder (pkt.log.block, XXTEA_BLOCK_COUNT);
		xxtea_decode (pkt.log.block, XXTEA_BLOCK_COUNT, tea_key);
		shuffle_tx_byteorder (pkt.log.block, XXTEA_BLOCK_COUNT);

		/* verify CRC */
		if (crc16 (pkt.log.byte, sizeof (pkt.log.pkt) - sizeof (pkt.log.pkt.crc)) != ntohs (pkt.log.pkt.crc))
			error++;
		else
		{
			out.timestamp = htonl(time);
			out.ip = htonl(reader_id);
			memcpy(&out.env,&pkt.log,sizeof(out.env));
			fwrite(&out,sizeof(out),1,fout);
#ifdef  DEBUG
			hex_dump(&pkt.log,0,sizeof(pkt.log));
#endif/*DEBUG*/
		}
	}
	fclose(f);
	fclose(fout);
	return EXIT_SUCCESS;
}

