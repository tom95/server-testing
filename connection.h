#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <arpa/inet.h>

#define MAX_REQUEST_LEN 4096

typedef enum {
	CONNECTED,
	HEADER_SENDING,
	HEADER_SENT,
	BODY_SENDING,
	BODY_SENT,
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

	char response_buffer[2048];
	int response_buffer_length;
	long response_data_sent;
	int response_file_fd;
	long response_file_size;
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
