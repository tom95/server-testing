#include <stdio.h>

#include "server.h"

#define DEFAULT_PORT 8000

int main (int argc, char **argv)
{
	Server *server;

	int port;

	if (argc > 1)
		port = strtol (argv[1], NULL, 10);
	else
		port = DEFAULT_PORT;

	/*void *data;
	HashTable *h = NULL;

	char abc [] = "test\0";
	printf ("%s\n", abc);
	h = hash_table_set (h, INT_TO_POINTER (5), INT_TO_POINTER (20));

	data = hash_table_get (h, INT_TO_POINTER (5));
	printf ("DATA: %p\n", data);

	return 0;*/
	server = server_new (port);

	if (!server)
		return 1;

	printf ("Starting server on port %i\n", port);
	return server_run (server);
}

