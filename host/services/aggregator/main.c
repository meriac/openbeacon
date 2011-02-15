// (c)2006 Andy Green <andy@warmcat.com>

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
  Binary log is consisting of

   - 32-bit timestamp
   - 32-bit reader source IP (used a reader UID)
   - decrypted struct TBeaconTracker received from reader

*/

#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <time.h>
#include <stdarg.h>
#include <signal.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <signal.h>
#include <sched.h>

#include "openbeacon.h"

#define USER_TO_RUN_AS 1200
#define GROUP_TO_RUN_AS 1200
#define UDP_LISTEN_PORT	2342
#define TCP_LISTEN_PORT 8111
#define LOG_DIRPATH "."
#define CONFIG_FILEPATH "openbeacon.conf"
//#define MAX_LOG_FILE_SIZE 10000000
#define MAX_READERS 200
#define MAX_TAGS 1000
#define SIGHTING_FIFO_SIZE 64
#define HASH_TABLE_SIZE 256
#define MS_READER_HITS_LIVE 30
#define MAX_STRENGTH 255
#define MAX_UDP_CLIENTS 100
#define SIZE_ENCRYPTED_PACKET 16
#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#define MX (z>>5^y<<2)+(y>>3^z<<4)^(sum^y)+(tea_key[p&3^e]^z)
#define DELTA 0x9E3779B9L

/* CCC 25C3 Key
 *
 * see also http://wiki.openbeacon.org/wiki/EncryptionKeys
 *
*/
const long tea_key[4]={0xbf0c3a08,0x1d4228fc,0x4244b2b0,0x0b4492e9};

static int flagDecrypt=1;
static int flagLog=1;


#define MAXBUF	1024
char buffer[MAXBUF];
static FILE* pfileLog=NULL;
static unsigned long ulongLogSize=0;
static char vChildStack[100 * 1024];
static struct sockaddr_in sockaddr_ina[MAX_UDP_CLIENTS];
static int nCountClientSockets=0;
static char szConfigFilepath[MAX_PATH];
static char szLogDirpath[MAX_PATH];
static int nHourLogCurrent=-1;

#define PERROR(msg)	{ perror(msg); abort(); }

typedef long coordinate;
typedef unsigned long address;
typedef long long timestamp;
typedef unsigned int taguid;
typedef unsigned char thash;

typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int u_int32_t;



typedef struct structstructReader {

	struct structstructReader * m_pNextWithSameHash;
	address m_address;
	int m_nReaderIndex;

} structReader;


typedef struct {

	structReader * m_pReader;
	unsigned char m_ucAmplitude;
	timestamp m_timestamp;

} structSighting;


typedef struct structstructTag {

	struct structstructTag * m_pNextWithSameHash;
	taguid m_tagUID;
	structSighting m_ringbufferSighting[SIGHTING_FIFO_SIZE];
	int m_nValidFifo;
	int m_nTagIndex;

} structTag;


static structReader readera[MAX_READERS];
static structTag taga[MAX_TAGS];
static timestamp timestampRepititionPeriod[MAX_STRENGTH+1];
static int nNextUnusedReader=0, nNextUnusedTag=0;

static structReader * pFirstReaderWithThisHash[HASH_TABLE_SIZE];
static structTag * pFirstTagWithThisHash[HASH_TABLE_SIZE];

thash ReaderIPToHash(address * pIP) { char *pc=((char *)pIP); return pc[0]^pc[1]^pc[2]^pc[3]; }
thash TagUidToHash(taguid * pTagUID) { char *pc=((char *)pTagUID); return pc[0]^pc[1]^pc[2]^pc[3]; }

void AddReaderToHashTable(structReader * preader) {
	thash hash=ReaderIPToHash(&preader->m_address);
//	printf("hash=%d\n", hash);
	preader->m_pNextWithSameHash=NULL;
	if(pFirstReaderWithThisHash[hash]==0) {
//		printf("AddReaderToHashTable start has was 0\n");
		pFirstReaderWithThisHash[hash]=preader;
	} else {
		structReader * pr=pFirstReaderWithThisHash[hash];
//		printf("AddReaderToHashTable start hash nonzero\n");
		while(pr->m_pNextWithSameHash) { pr=pr->m_pNextWithSameHash; }
		pr->m_pNextWithSameHash=preader;
	}
}

void AddTagToHashTable(structTag * ptag) {
	thash hash=TagUidToHash(&ptag->m_tagUID);
	ptag->m_pNextWithSameHash=NULL;
	if(pFirstTagWithThisHash[hash]==0) {
		pFirstTagWithThisHash[hash]=ptag;
	} else {
		structTag * pt=pFirstTagWithThisHash[hash];
		while(pt->m_pNextWithSameHash) { pt=pt->m_pNextWithSameHash; }
		pt->m_pNextWithSameHash=ptag;
	}
}

structReader * ReaderFromAddress(address ads) {
	thash hash=ReaderIPToHash(&ads);
	if(pFirstReaderWithThisHash[hash]==0) {
//		printf("ReaderFromAddress [%d] is 0\n", hash);
		return NULL;
	} else {
		structReader * pr=pFirstReaderWithThisHash[hash];
//		printf("ReaderFromAddress [hash] is occupied\n");
		do {
			if(pr->m_address==ads) return pr;
			pr=pr->m_pNextWithSameHash;
		} while(pr!=NULL);
		
		return NULL;
	}
}

structTag * TagFromUID(taguid tagUID) {
	thash hash=TagUidToHash(&tagUID);
	if(pFirstTagWithThisHash[hash]==0) {
		return NULL;
	} else {
		structTag * pt=pFirstTagWithThisHash[hash];
		do {
			if(pt->m_tagUID==tagUID) return pt;
			pt=pt->m_pNextWithSameHash;
		} while(pt!=NULL);
		
		return NULL;
	}
}

unsigned short CRC16(unsigned char * buffer, int size) {
    unsigned short crc=0xFFFF;
    if(buffer)
    {
        while(size--)
        {
            crc  =(crc>>8)|(crc<<8);
            crc ^=*buffer++;
            crc ^=((unsigned char)crc)>>4;
            crc ^=crc<<12;
            crc ^=(crc &0xFF) << 5;
        }
    }
    return crc;
}


void ParseConfigFile()
{

	int n;
	nCountClientSockets=0;

	char sz[256];
	FILE *pfile=fopen(szConfigFilepath, "r");
	if(pfile!=NULL) {
		while(fgets(sz, sizeof(sz), pfile)) {

			if( (sz[strlen(sz)-1]=='\r') || (sz[strlen(sz)-1]=='\n')) sz[strlen(sz)-1]='\0';
			if( (sz[strlen(sz)-1]=='\r') || (sz[strlen(sz)-1]=='\n')) sz[strlen(sz)-1]='\0';

			if(strncmp(sz, "client=", 7)==0) {
				if(nCountClientSockets >=(sizeof(sockaddr_ina)/sizeof(struct sockaddr_in)) ) {
					fprintf(stderr, "Reached client limit of %d: not serving %s\n", MAX_UDP_CLIENTS, &sz[7]);
				} else {
					sockaddr_ina[nCountClientSockets].sin_family = AF_INET;
					if (inet_aton(&sz[7], &(sockaddr_ina[nCountClientSockets].sin_addr)) == 0) {
						fprintf(stderr, "inet_aton(%s) failed\n", &sz[7]);
					}
					sockaddr_ina[nCountClientSockets].sin_port = htons(UDP_LISTEN_PORT);
					nCountClientSockets++;
				}
			}
			
			if( (strncmp(sz, "log=0", 5)==0) || (strncmp(sz, "log=n", 5)==0) || (strncmp(sz, "log=N", 5)==0) ) {
				flagLog=0;
			}

			if( (strncmp(sz, "decrypt=0", 9)==0) || (strncmp(sz, "decrypt=n", 9)==0) || (strncmp(sz, "decrypt=N", 9)==0) ) {
				flagDecrypt=0;
			}

			if( strncmp(sz, "logdir=", 7)==0) {
				strcpy(szLogDirpath, &sz[7]);
			}

		}
		fclose(pfile);
	} else {
		fprintf(stderr, "Unable to open config file '%s'\n", szConfigFilepath);
		exit(1);
	}
}

FILE * CreateNewLog()
{
	char szLogFilepath[512];
	struct tm * ptmCurrent;
	time_t timeCurrent=time(NULL);

	ptmCurrent=localtime(&timeCurrent);
	if(ptmCurrent!=NULL) {
		strftime(szLogFilepath, sizeof(szLogFilepath), LOG_DIRPATH"/log-%Y-%m-%d-%H", ptmCurrent);
		nHourLogCurrent=ptmCurrent->tm_hour;
	} else {
		perror("localtime");
		sprintf(szLogFilepath, LOG_DIRPATH"/log-%d", timeCurrent);
	}

	if(pfileLog!=NULL) fclose(pfileLog);

	pfileLog=fopen(szLogFilepath, "a");
	if(pfileLog==NULL) {
		perror(szLogFilepath);
		return NULL;
	}
	ulongLogSize=0;
	return pfileLog;
}


void Log(const char *szFmt, ...)
{
	char szBuf[512];
	char *sz=&szBuf[0];
	va_list ap;
	int len;
	
	va_start(ap, szFmt);
	len=vsprintf(szBuf, szFmt, ap);
	ulongLogSize+=len;

	va_end(ap);
/*
	if((ulongLogSize > MAX_LOG_FILE_SIZE) || (pfileLog==NULL)) {
		if(CreateNewLog()==NULL) {
			printf("Out of log space!!!\n");
		}
	}
	*/
	fwrite(szBuf, 1, len, pfileLog);
//	fflush(pfileLog);
}

void LogBinary(const unsigned char * uca, int nLength)
{
	fwrite(uca, 1, nLength, pfileLog);
}

int ThreadTCPListen(void * pvoid)
{
	struct sigaction sa;
	int nBindGood=0;

	printf("ThreadTCPListen started\n");

/*	
	sigemptyset(&(sa.sa_mask));
	sa.sa_handler=DoClidInterpretation;
	sa.sa_flags=0;

	sigaction(SIGUSR1,&sa,NULL);
*/
	struct sockaddr_in addr;
	int sd, addrlen = sizeof(addr);

	if ( (sd = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
		PERROR("TCP Socket");
	addr.sin_family = AF_INET;
	addr.sin_port = htons(TCP_LISTEN_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	while(!nBindGood) {
		if ( bind(sd, (struct sockaddr*)&addr, addrlen) < 0 ) { perror("TCP Bind"); sleep(1); } else { nBindGood=1; }
	}
	if ( listen(sd, 20) < 0 ) { PERROR("TCP Listen"); }
	while (1)
	{	int len;
		int client = accept(sd, (struct sockaddr*)&addr, &addrlen);
		printf("TCP Connected: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

		if ( (len = recv(client, buffer, MAXBUF, 0)) > 0 )
		{
			FILE* ClientFP = fdopen(client, "w");
			if ( ClientFP == NULL )
				perror("TCP fpopen");
			else
			{	char Req[512];
				unsigned long ulTime=time(NULL);
				unsigned long ulSpecificTag=-1;
				address addressa[MAX_READERS];
				int strengtha[MAX_READERS];
				int nCountHits=0;

				sscanf(buffer, "GET %s HTTP", Req);
				printf("Request: \"%s\"\n", Req);
				if(strlen(Req)>1) ulSpecificTag=atoi(&Req[1]);
				
				sprintf(Req, "HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n");
				fwrite(Req, 1, strlen(Req), ClientFP);

				{
					int n,m, nLimit;
					int ucaReaderLowest[MAX_READERS];
					timestamp timeReaderLowest[MAX_READERS];

					if(ulSpecificTag!=-1) {
						structTag * pTag=TagFromUID(ulSpecificTag);
						if(pTag==NULL) {
							n=0; nLimit=0;
						} else {
							n=pTag->m_nTagIndex;
							nLimit=n+1;
						}
					} else {
						n=0;
						nLimit=nNextUnusedTag;
					}

					for(;n<nLimit;n++) {


						sprintf(Req, "+%u:", taga[n].m_tagUID); fwrite(Req, 1, strlen(Req), ClientFP);
/*
						for(m=0;m<SIGHTING_FIFO_SIZE;m++) {
							printf("fifo entry %d\n", m);
							structSighting * ps=&taga[n].m_ringbufferSighting[(taga[n].m_nValidFifo+m)&(SIGHTING_FIFO_SIZE-1)];
							printf("ps %p\n", ps);
							printf("time %ld\n", ps->m_timestamp);
							printf("reader %p\n", ps->m_pReader);
							if((ps->m_timestamp < ulTime) && ((ps->m_timestamp+MS_READER_HITS_LIVE) > ulTime) && (ps->m_pReader!=NULL)) {
								sprintf(Req, " %s,%d,%08X<br>", ps->m_pReader->szFriendlyName, ps->m_ucAmplitude, ulTime-ps->m_timestamp); fwrite(Req, 1, strlen(Req), ClientFP);
							}
						}
*/

						memset(ucaReaderLowest, 0x7f, nNextUnusedReader * sizeof(int));

						for(m=0;m<SIGHTING_FIFO_SIZE;m++) {
//							printf("fifo entry %d\n", m);
							structSighting * ps=&taga[n].m_ringbufferSighting[(taga[n].m_nValidFifo+m)&(SIGHTING_FIFO_SIZE-1)];
							if((ps->m_timestamp < ulTime) && ((ps->m_timestamp+MS_READER_HITS_LIVE) > ulTime) && (ps->m_pReader!=NULL)) {
								if(ps->m_ucAmplitude <= ucaReaderLowest[ps->m_pReader->m_nReaderIndex]) {
									ucaReaderLowest[ps->m_pReader->m_nReaderIndex]=ps->m_ucAmplitude;
									timeReaderLowest[ps->m_pReader->m_nReaderIndex]=ps->m_timestamp;
								}
							}
						}

						nCountHits=0;

						for(m=0;m<nNextUnusedReader;m++) {
							if(ucaReaderLowest[m]!=0x7f7f7f7f) {
								timestamp age=ulTime-timeReaderLowest[m];
								int nStrengthAged=ucaReaderLowest[m];
								if(age>timestampRepititionPeriod[ucaReaderLowest[m]]) { // it is stale
									nStrengthAged+=5*(age-timestampRepititionPeriod[ucaReaderLowest[m]] );
									if(nStrengthAged>255) nStrengthAged=255;
								}
								addressa[nCountHits]=readera[m].m_address;
								strengtha[nCountHits]=nStrengthAged;
								nCountHits++;
							}
						}


						for(m=0;m<nCountHits;m++) {
							int nBiggest=0, nMatch;
							int i;
							for(i=0;i<nCountHits;i++) { if(strengtha[i]>=nBiggest) { nMatch=i; nBiggest=strengtha[i]; }
							}

							sprintf(Req, "%d,%d;", addressa[nMatch], strengtha[nMatch]); fwrite(Req, 1, strlen(Req), ClientFP);
							strengtha[nMatch]=-1;
						}

//						sprintf(Req, "\n<br>", readera[m].m_address, ucaReaderLowest[m]); fwrite(Req, 1, strlen(Req), ClientFP);

					}
				}

				fclose(ClientFP);
				printf("TCP: closed\n");
			}
		}
		close(client);
	}
	return 0;

}

void tea_block_decode( long * v, long n )
{
    unsigned long z,y=v[0],e,p,sum;

    sum = (6+52/n)*DELTA;
    while (sum != 0)
    {
	e = sum>>2 & 3 ;
	for (p = n-1 ; p > 0 ; p-- )
	{
	    z = v[p-1];
	    y = v[p] -= MX;
	}
	z = v[n-1];
	y = v[0] -= MX;
	sum -= DELTA ;
    }
}

int main(int count, char *strings[])
{
	int sd,i, socketSend, nStatus;
	int port=UDP_LISTEN_PORT;
	struct sockaddr_in addr;
	unsigned char buffer[1024], szLogBinary[1024];
	unsigned long *pl;
	struct stat stat1;
	structReader * pr;
	time_t timeLastLocaltimeChecked=0;


/*	nStatus=setgid(GROUP_TO_RUN_AS); // vp group
	if(nStatus) {
		printf("Problem with setgid: %s\n", strerror(errno));
	}
	nStatus=setuid(USER_TO_RUN_AS); // vp user
	if(nStatus) {
		printf("Problem with setuid: %s\n", strerror(errno));
	}*/

	strcpy(szConfigFilepath, CONFIG_FILEPATH);
	if(count>1) {
		if(strncmp(strings[1], "-c=", 3)==0 ) {
			strncpy(szConfigFilepath, &strings[1][3], MAX_PATH);
		} else {
			printf("Usage: %s [-c=<config filepath>]\n");
			return 1;
		}
	}

	strcpy(szLogDirpath, LOG_DIRPATH);

	ParseConfigFile();

	memset(pFirstReaderWithThisHash, 0, sizeof(pFirstReaderWithThisHash));
	memset(pFirstTagWithThisHash, 0, sizeof(pFirstTagWithThisHash));
	memset(taga, 0, sizeof(taga));

	memset(timestampRepititionPeriod, 5, sizeof(timestampRepititionPeriod));

   printf("OpenBeacon Station Aggregator  (c)2006 andy@warmcat.com\n");

	if(stat(LOG_DIRPATH, &stat1)==-1) {
		perror("Ensure "LOG_DIRPATH" is created and has good permissions");
		return 1;
	}

	if(CreateNewLog()==NULL) return 2;

	{
		int pidChild=clone(ThreadTCPListen, &vChildStack[sizeof(vChildStack)-4], CLONE_THREAD | CLONE_VM | CLONE_SIGHAND, NULL);
//		perror("clone");
//		printf("clone() returns %d\n", pidChild);
		usleep(100000);
	}


	if ((socketSend = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) { PERROR("sending socket"); }

	sd = socket(PF_INET, SOCK_DGRAM, 0);
	memset((char *)&addr, 0, (size_t)sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if ( bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 ) {
		perror("bind");
	}

	while (1) {	
		int bytes, addr_len=sizeof(addr);
		unsigned char *pucPayload;
		time_t timeCurrent;

		bytes = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &addr_len);

		timeCurrent=time(NULL);
		if(timeLastLocaltimeChecked != timeCurrent) { // limit to once per second
			struct tm * ptm=localtime(&timeCurrent);
			if(nHourLogCurrent!=ptm->tm_hour) { // we have changed hour: move to new Log filename
				if(CreateNewLog()==NULL) {
					fprintf(stderr, "Uh oh, we could not open a new log\n");
				}
			}
		}

		if( ((bytes==SIZE_ENCRYPTED_PACKET) && flagDecrypt) || ((bytes==(SIZE_ENCRYPTED_PACKET+4))&& !flagDecrypt) ) {

			address addressEffective;

			if(flagDecrypt) {

				addressEffective=ntohl(addr.sin_addr.s_addr);
				pucPayload=&buffer[0];

				// unshuffle tea blocks from network byte order to host byte order
				pl=(unsigned long *)&buffer;
				for(i=0;i<TEA_ENCRYPTION_BLOCK_COUNT;i++)
					*pl++=ntohl(*pl);
				// decode data
				tea_block_decode((unsigned long *)&buffer,TEA_ENCRYPTION_BLOCK_COUNT);
				// unshuffle data content from network byte order to host byte order
				pl=(unsigned long *)&buffer;
				for(i=0;i<TEA_ENCRYPTION_BLOCK_COUNT;i++)
					*pl++=ntohl(*pl);

			} else {

				addressEffective=ntohl((address)(*((unsigned long *)buffer)));
				pucPayload=&buffer[4];

			}
			
			if(CRC16(pucPayload, SIZE_ENCRYPTED_PACKET-2)==ntohs(*((unsigned short *)&pucPayload[SIZE_ENCRYPTED_PACKET-2]))) {

		//		printf("msg from %s:%d (%d bytes)\n", inet_ntoa(addr.sin_addr),
		//						ntohs(addr.sin_port), bytes);
		
//				printf("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X ",
//					(unsigned char)pucPayload[0], (unsigned char)pucPayload[1], (unsigned char)pucPayload[2], (unsigned char)pucPayload[3], (unsigned char)pucPayload[4], (unsigned char)pucPayload[5], (unsigned char)pucPayload[6],
//					(unsigned char)pucPayload[7], (unsigned char)pucPayload[8], (unsigned char)pucPayload[9], (unsigned char)pucPayload[10], (unsigned char)pucPayload[11], (unsigned char)pucPayload[12], (unsigned char)pucPayload[13]
//				);
		
				TBeaconTracker * tbeacon=(TBeaconTracker *)pucPayload;
				structReader * pr=ReaderFromAddress(addressEffective);
				structTag * pt=TagFromUID(ntohl(tbeacon->oid));

				if(pr==NULL) { // add reader

					if(nNextUnusedReader<(sizeof(readera)/sizeof(readera[0])) ) {
						printf("Adding reader %d\n", addressEffective);
						pr=&readera[nNextUnusedReader++];
						pr->m_address=addressEffective;
						pr->m_nReaderIndex=nNextUnusedReader-1;
						AddReaderToHashTable(pr);
					} else {
						printf("More readers than I can handle... ignoring %d\n", addressEffective);
					}
				}


				if(pt==NULL) {
					if(nNextUnusedTag==(sizeof(taga)/sizeof(taga[0]))) {
						printf("More individual tags than I can handle\n");
					} else {
						printf("added tag\n");
						pt=&taga[nNextUnusedTag++];
						pt->m_tagUID=(taguid)ntohl(tbeacon->oid);
						pt->m_nValidFifo=0;
						pt->m_nTagIndex=nNextUnusedTag-1;
						AddTagToHashTable(pt);
					}
				}
	
				if(pr!=NULL) {
					int n;
					printf("RX:%s %d 0x%08X 0x%02X %d\n", inet_ntoa(htonl(pr->m_address)), ntohl(tbeacon->oid), ntohl(tbeacon->seq), tbeacon->flags, tbeacon->strength);
				
					structSighting * ps=&pt->m_ringbufferSighting[pt->m_nValidFifo & (SIGHTING_FIFO_SIZE-1)];
					ps->m_timestamp=timeCurrent;
	//			ps->m_x=pr->m_x; ps->m_y=pr->m_y; ps->m_z=pr->m_z;
					ps->m_pReader=pr;
					ps->m_ucAmplitude=tbeacon->strength;
					pt->m_nValidFifo=(pt->m_nValidFifo+1)&(SIGHTING_FIFO_SIZE-1);

						// adjust to add genuine source IP to start of packet
						// ... assuming it isn't there already

					if(flagDecrypt) {
						int n;
						for(n=(SIZE_ENCRYPTED_PACKET)+4-1;n>(4)-1;n--) {
							buffer[n]=buffer[n-4];
						}
						*((unsigned long *)&buffer[0])=addr.sin_addr.s_addr;
					}

						// pass it on to any clients

					for(n=0;n<nCountClientSockets;n++) {
						sendto(socketSend, buffer, SIZE_ENCRYPTED_PACKET+4, 0, (struct sockaddr*)&sockaddr_ina[n], sizeof(addr));
					}

					{
						int n;
						for(n=0;n<SIZE_ENCRYPTED_PACKET;n++) {
							szLogBinary[n+8]=buffer[n+4];
						}
						*((unsigned long *)&szLogBinary[0])=htonl(timeCurrent);
						*((unsigned long *)&szLogBinary[4])=addr.sin_addr.s_addr;

						LogBinary(szLogBinary, SIZE_ENCRYPTED_PACKET+8);
					}

				} else {
					printf("null pr\n");
				}
			} else {
				printf("(Rejecting on CRC=%x %04X)\n", CRC16(pucPayload, SIZE_ENCRYPTED_PACKET-2), ntohs(*((unsigned short *)&pucPayload[SIZE_ENCRYPTED_PACKET-2])));
			}

		} else {
			printf("(Rejecting on size %d)\n", bytes);
		}
	}
	close(sd);
}

