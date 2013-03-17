#include <stdlib.h>
#include <stdio.h>
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

int
main (int argc, char **argv)
{
	TBeaconLogSighting pkt;
	int delta, i;
	int seq, reader_id, time, logtime;
	FILE *f;

	if(argc!=3)
	{
		printf("usage: %s logfile.bin timestamp_offset\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	if((f = fopen(argv[1],"rb"))==NULL)
	{
		printf("ERROR: cant't open '%s' for writing\n",argv[1]);
	}

	delta = atoi(argv[2]);
	printf("Delta-time = %i\n",delta);

	i = 0;
	while(fread(&pkt,sizeof(pkt),1,f) == 1)
	{
		if(pkt.hdr.protocol != BEACONLOG_SIGHTING)
		{
			printf("ERROR: unknown protocol 0x%02X\n",pkt.hdr.protocol);
			break;
		}

		if(ntohs(pkt.hdr.size) != sizeof(pkt))
		{
			printf("ERROR: unknown protocol size %i!=%i\n",(int)ntohs(pkt.hdr.size),(int)sizeof(pkt));
		}

		/* decode packet */
		shuffle_tx_byteorder (pkt.log.block, XXTEA_BLOCK_COUNT);
		xxtea_decode (pkt.log.block, XXTEA_BLOCK_COUNT, tea_key);
		shuffle_tx_byteorder (pkt.log.block, XXTEA_BLOCK_COUNT);

		reader_id = ntohs(pkt.hdr.reader_id);
		time = ntohl(pkt.timestamp);
		printf("%04i,%i,%i\n", reader_id, time, pkt.log.pkt.proto);

	}
	fclose(f);
	return EXIT_SUCCESS;
}

