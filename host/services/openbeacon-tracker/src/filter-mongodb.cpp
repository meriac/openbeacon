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
 Suggested MongoDB database index examples:
 *

 * ensure double documents from multiple replays of the same data
 db.log.ensureIndex ({"id":1, "time": 1}, {unique: true});

 * add geospatial index to tag position collection 
 db.tag.ensureIndex ( { "loc" : "2d" , "time":1 , "button":1 } , { min:0,max:1024 } );

 * add index for edge detection collection
 db.edge.ensureIndex ( { "tag":1, "time":1, "power":1 } );

*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <stdio.h>
#include <mongo.h>
#include <json/json.h>

#define LOG "FILTER_MONGODB "

#define LOG_ERR LOG "ERROR: "

#define MAX_NUM_SIZE 16
#define MEMORY_INCREMENT 4096

mongo g_mongo;

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
mongodb_store (const char *db, const char *collection, struct json_object *json, uint32_t time)
{
	bson b;
	bson *pb, *batch;
	bson **ppb, **batch_list;

	int count, written, i, res;
	struct json_object *js;
	char mongo_ns[128];

	if(!json)
		return MONGO_OK;

	if(snprintf(mongo_ns, sizeof(mongo_ns), "%s.%s", db, collection)>=(int)sizeof(mongo_ns))
	{
		fprintf (stderr, LOG "ERROR: MongoDB target namespace truncated.\n");
		return MONGO_ERROR;
	}

	/* batch-store arrays */
	if(json_object_get_type (json) != json_type_array)
	{
		bson_init (&b);
		process_json2bson (json, NULL, &b);
		bson_finish (&b);

		/* insert object into database */
		res = mongo_insert (&g_mongo, mongo_ns, &b, NULL);

		bson_destroy (&b);

		return res;
	}

	if((count=json_object_array_length(json))<=0)
		return MONGO_OK;

	batch = pb = (bson*)malloc(sizeof(bson)*count);
	batch_list = ppb = (bson**)malloc(sizeof(ppb)*count);
	memset(ppb,0,sizeof(ppb)*count);

	/* add all array objects to BSON list as single batch */
	written = 0;
	for(i=0;i<count;i++)
		if((js = json_object_array_get_idx (json,i))!=NULL)
		{
			if(time && (json_object_get_type (js) == json_type_object))
				json_object_object_add(js, "time", json_object_new_int (time));

			/* convert to BSON */
			bson_init (pb);
			process_json2bson (js, NULL, pb);
			bson_finish (pb);

			/* advance to next BSON object */
			*ppb++=pb++;
			written++;
		}

	if(written)
	{
		/* insert object into database */
		res = mongo_insert_batch (&g_mongo, mongo_ns, (const bson**)batch_list, written, NULL, 0);

		/* delete bson objects */
		pb=batch;
		for(i=0;i<written;i++)
			bson_destroy(pb++);
	}
	else
		res = MONGO_OK;

	free(batch);
	free(batch_list);

	return res;
}

int
main (int argc, char *argv[])
{
	int cmd, res, data, size, pos, i, mongo_port, mongo_log;
	uint8_t c[3];
	char *buffer, *p;
	const char *mongo_host, *mongo_db;
	struct json_tokener *tok;
	struct json_object *json, *js;

	/* set default parameters */
	mongo_host = "127.0.0.1";
	mongo_port = 27017;
	mongo_db = "openbeacon";
	mongo_log = 0;

	/* scan for command line arguments */
	opterr = 0;
	while ((cmd = getopt (argc, argv, "h:p:d:l")) != -1)
		switch (cmd)
		{
			case 'd':
				mongo_db = optarg;
				break;
			case 'h':
				mongo_host = optarg;
				break;
			case 'p':
				if((i = atoi (optarg))>0)
					mongo_port = i;
				break;
			case 'l':
				mongo_log = 1;
				break;
			default:
				fprintf (stderr, "\nUsage: %s [-l] [-h db_host] [-p db_port] [-d db_name]\n\n", argv[0]);
				return -1;
		}

	if ((tok = json_tokener_new ()) == NULL)
	{
		fprintf (stderr, LOG_ERR " failed to allocate tokener\n");
		return -2;
	}

	mongo_init (&g_mongo);
	mongo_set_op_timeout (&g_mongo, 1000);
	if ((res =
		 mongo_connect (&g_mongo, mongo_host, mongo_port)) != MONGO_OK)
	{
		fprintf (stderr, LOG_ERR " can't open mongo_db (host %s:%i)\n",
				 mongo_host, mongo_port);
		json_tokener_free (tok);
		return -3;
	}

	/* show connection parameters */
	fprintf (stderr, LOG " connected to mongo://%s:%i/%s\n", mongo_host, mongo_port, mongo_db);

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
				fprintf (stderr, LOG_ERR "JSON conversion error\n");
			else
			{
				if(mongo_log)
					mongodb_store (mongo_db,"log",json, 0);

				if(json_object_get_type (json)==json_type_object)
				{
					js = json_object_object_get (json, "time");
					if(js && ((i=json_object_get_int(js))>0))
					{
						mongodb_store(mongo_db,"tag",json_object_object_get(json, "tag"),i);
						mongodb_store(mongo_db,"edge",json_object_object_get(json, "edge"),i);
					}
					else
						fprintf (stderr, LOG_ERR "time stamp not found - ignoring dataset!\n");
				}
				else
					fprintf (stderr, LOG_ERR "JSON object needed - ignoring dataset!\n");
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
					fprintf (stderr, LOG_ERR "out of memory\n");
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
