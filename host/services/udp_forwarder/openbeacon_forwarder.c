/*************************************************************** 
 *
 * OpenBeacon.org - OnAir protocol forwarder
 *
 * See the following website for already decoded Sputnik data: 
 * http://people.openpcd.org/meri/openbeacon/sputnik/data/24c3/ 
 *
 * Copyright 2009 Milosch Meriac <meriac@bitmanufaktur.de> 
 *
 ***************************************************************/

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

#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 2048
#define PORT 2342
#define IP4_SIZE 4
#define OPENBEACON_SIZE 16
#define REFRESH_SERVER_IP_TIME (60*5)

static uint8_t buffer[BUFLEN];

int
main (void)
{
  struct sockaddr_in si_me, si_other, si_server;
  struct hostent *addr;
  int sock, len;
  uint32_t server_ip;
  socklen_t slen = sizeof (si_other);
  time_t lasttime, t;


  if ((sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    perror ("can't create sockjet");
  else
    {
      memset ((char *) &si_me, 0, sizeof (si_me));
      si_me.sin_family = AF_INET;
      si_me.sin_port = htons (PORT);
      si_me.sin_addr.s_addr = htonl (INADDR_ANY);

      if (bind (sock, (sockaddr *) & si_me, sizeof (si_me)) == -1)
	perror ("can't bind to listening socket");

      lasttime = 0;
      server_ip = 0;

      memset (&si_server, 0, sizeof (si_server));
      si_server.sin_family = AF_INET;
      si_server.sin_port = htons (PORT);

      while (1)
	{
	  if ((len =
	       recvfrom (sock, &buffer, BUFLEN - IP4_SIZE, 0,
			 (sockaddr *) & si_other, &slen)) == -1)
	    exit (-6);

	  t = time (NULL);
	  if ((t - lasttime) > REFRESH_SERVER_IP_TIME)
	    {
	      if ((addr = gethostbyname ("meriac.dyndns.org")) == NULL)
		printf ("can't resolve server name\n");
	      else
		if ((addr->h_addrtype != AF_INET)
		    || (addr->h_length != IP4_SIZE))
		printf ("wrong address type\n");
	      else
		{
		  memcpy (&si_server.sin_addr, addr->h_addr, addr->h_length);

		  if (server_ip != si_server.sin_addr.s_addr)
		    {
		      server_ip = si_server.sin_addr.s_addr;
		      printf ("refreshed server IP to [%s]\n",
			      inet_ntoa (si_server.sin_addr));
		    }

		  lasttime = t;
		}
	    }

	  if (len == OPENBEACON_SIZE)
	    {
	      memcpy (&buffer[len], &si_other.sin_addr.s_addr, IP4_SIZE);
	      sendto (sock, &buffer, len + IP4_SIZE, 0,
		      (struct sockaddr *) &si_server, sizeof (si_server));
	    }
	}
    }
}
