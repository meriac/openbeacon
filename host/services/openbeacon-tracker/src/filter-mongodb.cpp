/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol position tracker filter
 *
 * accepts stdout from position tracker on stdin and constantly
 * updates two JSON files reflecting the current state of the
 * tracker. One of the files ig gzip compressed.
 *
 * The basic idea is to point this filter to a RAM backed tmpfs
 * mount which is served by an Apache webserver et al.
 *
 * Copyright 2009-2012 Milosch Meriac <meriac@bitmanufaktur.de>
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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <mongo.h>
#include <json/json.h>

#define LOG "FILTER_MONGODB "

#define MEMORY_INCREMENT 4096

mongo g_mongo;
const int g_mongo_port = 27017;
const char g_mongo_host[] = "127.0.0.1";

void
process_json_object (const char *json_key, struct json_object *json_obj,
					 int level)
{
	char *key;
	struct lh_entry *entry;
	struct json_object *val;
	enum json_type type = json_object_get_type (json_obj);

	level++;

	printf ("\tkey=%s val=%s type=%i level=%i\n", json_key,
			json_object_get_string (json_obj), type, level);

	switch (type)
	{
		case json_type_boolean:
			break;
		case json_type_double:
			break;
		case json_type_int:
			break;
		case json_type_object:
			{
				printf ("###OBJECT:\n");
				for (entry = json_object_get_object (json_obj)->head;
					 (entry ? (key = (char *) entry->k, val =
						 (struct json_object *) entry->v, entry) : 0);
					 entry = entry->next)
				{
					process_json_object (key, val, level);
				}
				printf ("OBJECT###\n");
				break;
			}
		case json_type_array:
//          json_object_get_array(json_obj);
			break;
		case json_type_string:
			break;
		default:
			break;
	}

}

void
process_json (struct json_object *json_obj)
{
	char *key;
	struct lh_entry *entry;
	struct json_object *val;
	enum json_type type = json_object_get_type (json_obj);

	switch (type)
	{
		case json_type_boolean:
			break;
		case json_type_double:
			break;
		case json_type_int:
			break;
		case json_type_object:
			{
				printf ("###OBJECT:\n");
				for (entry = json_object_get_object (json_obj)->head;
					 (entry
					  ? (key = (char *) entry->k, val =
						 (struct json_object *) entry->v, entry) : 0);
					 entry = entry->next)
				{
					process_json_object (key, val, 0);
				}
				printf ("OBJECT###\n");
				break;
			}
		case json_type_array:
//          json_object_get_array(json_obj);
			break;
		case json_type_string:
			break;
		default:
			break;
	}
}

int
main (int argc, char *argv[])
{
	int res, data, size, pos;
	uint8_t c[3];
	char *buffer, *p;
	struct json_tokener *tok;
	struct json_object *json;

	if (argc != 2)
	{
		fprintf (stderr, LOG " usage: %s mongodb_table_name\n", argv[0]);
		return -1;
	}

	if ((tok = json_tokener_new ()) == NULL)
	{
		fprintf (stderr, LOG " failed to allocate tokener\n");
		return -2;
	}

	mongo_init (&g_mongo);
	mongo_set_op_timeout (&g_mongo, 1000);
	if ((res =
		 mongo_connect (&g_mongo, g_mongo_host, g_mongo_port)) != MONGO_OK)
	{
		fprintf (stderr, LOG " can't open mongo_db (host %s:%i)\n",
				 g_mongo_host, g_mongo_port);
		json_tokener_free (tok);
		return -3;
	}


	res = size = pos = 0;
	buffer = p = NULL;
	c[1] = c[2] = 0;
	while ((data = getchar ()) != EOF)
	{
		/* convert to 8 bits */
		c[0] = (uint8_t) data;

		/* echo everything to maintain pipe chain */
//      putchar (c[0]);

		/* find start of new object */
		if (pos && c[2] == '\n' && c[1] == '}' && c[0] == ',')
		{
			/* terminate string */
			*p = 0;

			json = json_tokener_parse_ex (tok, buffer, pos);
			if (tok->err != json_tokener_success)
				fprintf (stderr, LOG "JSON conversion error\n");
			else
				process_json (json);
			json_tokener_reset (tok);

			/* reset object buffer */
			p = buffer;
			pos = 0;
		}
		else
		{
			/* increase memory pool for larger JSON object */
			if (pos >= (size - 1))
			{
				size += MEMORY_INCREMENT;
				if ((buffer = (char *) realloc (buffer, size)) == NULL)
				{
					fprintf (stderr, LOG "out of memory\n");
					res = -4;
					break;
				}
				/* adjust current object-end pointer */
				p = buffer + pos;
			}

			*p++ = c[0];
			pos++;
		}

		/* remember last two characters */
		c[2] = c[1];
		c[1] = c[0];
	}

	mongo_destroy (&g_mongo);
	json_tokener_free (tok);

	return res;
}
