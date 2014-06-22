#ifndef __SERVER_H__
#define __SERVER_H__

#include "hash-table.h"
#include "connection.h"

#define INT_TO_POINTER(i) ((void *) (intptr_t) i)

typedef struct {
	HashTable *connections;
	int server_socket;
	int epfd;
} Server;

Server *
server_new (int port);

int
server_run (Server *self);

#endif // __SERVER_H__
