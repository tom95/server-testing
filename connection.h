#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <arpa/inet.h>

#define MAX_REQUEST_LEN 4096

typedef enum {
	CONNECTING,
	CONNECTED,
	CLOSING,
	CLOSED
} State;

enum Method {
	GET,
	POST,
	PUT,
	DELETE
};

struct Request {
	enum Method method;
	char path[1024];
};

typedef struct {
	int fd;
	char ip[INET_ADDRSTRLEN];

	char *request;
	size_t request_len;

	struct Request parsed_request;

	State state;
} Connection;

int
connection_add_request_data (Connection *self, char *data, int len);

Connection *
connection_new (int fd, struct sockaddr_in *addr, socklen_t addrlen);

int
connection_parse_request (Connection *self);

void
connection_send_response (Connection *self);

#endif // __CONNECTION_H__
