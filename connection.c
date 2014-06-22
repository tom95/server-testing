#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <linux/tcp.h>

#include "connection.h"

#define EOR "\n\r\n"

Connection *
connection_new (int fd, struct sockaddr_in *addr, socklen_t addrlen)
{
	Connection *self;

	self = malloc (sizeof (Connection));
	self->fd = fd;
	self->request_len = 0;

	inet_ntop (AF_INET, &addr->sin_addr.s_addr, self->ip, addrlen);

	int one = 1;

	fcntl (fd, F_SETFL, O_NONBLOCK, 1);
	setsockopt (fd, IPPROTO_TCP, TCP_CORK, &one, sizeof (int));

	return self;
}

/**
 * Add part of a request
 *
 * @return wheter to continue waiting or end the connection
 */
int
connection_add_request_data (Connection *self, char *data, int len)
{
	size_t new_size;

	new_size = self->request_len + len;
	if (new_size > MAX_REQUEST_LEN) {
		fprintf (stderr, "Max request size exceeded");
		return 0;
	}

	self->request = realloc (self->request, len);

	memcpy (self->request + self->request_len, data, len);

	self->request_len = new_size;

	if (memcmp (self->request + (self->request_len - 3), EOR, 3) == 0)
		return 0;

	return 1;
}

int
connection_parse_request (Connection *self)
{
	enum ParserState {
		REQUEST_LINE,
		HEADERS,
		DONE
	};

	printf ("DATA: %s\n", self->request);

	enum ParserState mode = REQUEST_LINE;

	char *line;
	int start = 0;
	int end;

	enum Method method;
	char method_string[6];
	char path[1024];

	char field[16];
	char value[1024];

	while (mode != DONE) {
		line = strstr (self->request + start, "\n");
		end = line - self->request;
		if (line == NULL)
			return 0;

		self->request[end] = '\0';

		if (end - start < 3) {
			mode = DONE;
			break;
		}

		switch (mode) {
			case REQUEST_LINE:
				if (sscanf (self->request + start, "%s %[^ ] HTTP/1.1", method_string, path) != 2)
					return 0;

				mode = HEADERS;

				break;
			case HEADERS:
				sscanf (self->request + start, "%s: %[^\n]", field, value);
				printf ("FIELD: %s WITH VALUE %s\n", field, value);
				break;
			case DONE:
				break;
		}

		start = end + 1;
	}

	if (memcmp (method_string, "GET", 3) == 0)
		method = GET;
	else if (memcmp (method_string, "POST", 4) == 0)
		method = POST;
	else if (memcmp (method_string, "PUT", 3) == 0)
		method = PUT;
	else if (memcmp (method_string, "DELETE", 6) == 0)
		method = DELETE;
	else
		return 0;

	printf ("REQUESTEE DPATH: %s\n", path);
	strcpy (self->parsed_request.path, path);
	self->parsed_request.method = method;

	return 1;
}

#define BASE_PATH "/home/tom/Code/js/game/"

void
connection_send_response (Connection *self)
{
	char path[1024];

	printf ("REQUESTEE DPATH: %s\n", self->parsed_request.path);

	if (self->parsed_request.path[strlen (self->parsed_request.path) - 1] == '/')
		strcat (self->parsed_request.path, "index.html");

	strcpy (path, BASE_PATH);
	strcat (path, self->parsed_request.path);

	printf ("FINAL PATH: %s\n", path);
}

