/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol position tracker filter
 *
 * Accept stdout from position tracker on stdin and constantly
 * update a MongoDB database by dumping the latest JSON object
 * into it. By using a good index an excellent read performance
 * is achieved (see database index examples below).
 *
 * Please note that stdin is forwarded to stdout unmodified -
 * that allows to combine multiple filters or forwarding the 
 * network stream over network.
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

/*
 Suggested MongoDB database indexe examples:
 *
 db.log.ensureIndex ({"id":1, "time": 1}, {unique: true});
 db.log.ensureIndex ({"id": 1});
 db.log.ensureIndex ({"time": 1});
 db.log.ensureIndex ({"tag.id": 1});
 db.log.ensureIndex ({"tag.id": 1, "time": 1});
 db.log.ensureIndex ({"edge.tag": 1});
 db.log.ensureIndex ({"edge.tag": 1, "time": 1});
*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <mongo.h>
#include <json/json.h>

#define LOG "FILTER_MONGODB "

#define MAX_NUM_SIZE 16
#define MEMORY_INCREMENT 4096

mongo g_mongo;
const int g_mongo_port = 27017;
const char g_mongo_host[] = "127.0.0.1";

void
process_json2bson (struct json_object *json_obj, const char *json_key,
				   bson * b)
{
	int n, i;
	char *key, index[MAX_NUM_SIZE];
	struct lh_entry *entry;
	struct json_object *val;
	enum json_type type = json_object_get_type (json_obj);

	switch (type)
	{
		case json_type_boolean:
			bson_append_bool (b, json_key,
							  json_object_get_boolean (json_obj));
			break;

		case json_type_double:
			bson_append_double (b, json_key,
								json_object_get_double (json_obj));
			break;

		case json_type_int:
			bson_append_int (b, json_key, json_object_get_int (json_obj));
			break;

		case json_type_object:
			{
				if (json_key)
					bson_append_start_object (b, json_key);

				for (entry = json_object_get_object (json_obj)->head;
					 (entry ? (key = (char *) entry->k, val =
							   (struct json_object *) entry->v, entry) : 0);
					 entry = entry->next)
				{
					process_json2bson (val, key, b);
				}

				if (json_key)
					bson_append_finish_object (b);
				break;
			}

		case json_type_array:
			{
				bson_append_start_array (b, json_key);

				n = json_object_array_length (json_obj);
				for (i = 0; i < n; i++)
				{
					snprintf(index, sizeof(index), "%i", i);

					if ((val = json_object_array_get_idx (json_obj, i)) == NULL)
						bson_append_null (b, index);
					else
						process_json2bson (val, index, b);
				}

				bson_append_finish_array (b);
				break;
			}

		case json_type_string:
			bson_append_string (b, json_key, json_object_get_string (json_obj));
			break;

		default:
			bson_append_null (b, json_key);
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
	bson b;
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
		putchar (c[0]);
		/* find start of new object */
		if (pos && c[2] == '\n' && c[1] == '}' && c[0] == ',')
		{
			/* terminate string */
			*p = 0;
			json = json_tokener_parse_ex (tok, buffer, pos);
			if (tok->err != json_tokener_success)
				fprintf (stderr, LOG "JSON conversion error\n");
			else
			{
				bson_init (&b);
				process_json2bson (json, NULL, &b);
				bson_finish (&b);

				/* insert object into database */
				mongo_insert (&g_mongo, argv[1], &b, NULL);

				bson_destroy (&b);
			}
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
	free (buffer);
	return res;
}
