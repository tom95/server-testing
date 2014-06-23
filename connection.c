#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <linux/tcp.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include "connection.h"

#define EOR "\n\r\n"

Connection *
connection_new (int fd, struct sockaddr_in *addr, socklen_t addrlen)
{
	Connection *self;

	self = malloc (sizeof (Connection));
	self->fd = fd;
	self->request_len = 0;
	self->state = CONNECTED;

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

enum ParserState {
	S_START,
	S_METHOD,
	S_PATH,
	S_HTTP_VERSION,
	S_HEADER_KEY_START,
	S_HEADER_KEY,
	S_HEADER_VALUE,
	S_END
};

enum HeaderField {
	H_CONNECTION
};

int
connection_parse_request (Connection *self)
{
	enum ParserState parser_state = S_START;
	char *ch, c;
	size_t index = 0, tmp_index = 0;
	enum HeaderField current_field;

	enum Method method;
	char path[1024];
	int keep_alive = 0;

	printf ("DATA: %s\n", self->request);
	ch = self->request;

	while (ch && index < self->request_len) {
		c = *ch;

		if (c == '\r') {
			ch++;
			index++;
			continue;
		}

		switch (parser_state) {
			case S_START:

				switch (c) {
					case 'G': method = GET; break;
					case 'P': method = POST; break;
					case 'D': method = DELETE; break;
				}

				parser_state = S_METHOD;

				break;
			case S_METHOD:
				if (c == ' ') {
					// TODO check length of method with index
					parser_state = S_PATH;
					break;
				}

				if (index == 1 && method == POST && c == 'U')
					method = PUT;

				break;
			case S_PATH:
				if (c == ' ') {
					parser_state = S_HTTP_VERSION;
					path[tmp_index++] = '\0';
					break;
				}

				path[tmp_index++] = c;

				break;
			case S_HTTP_VERSION:
				if (c == '\n') {
					parser_state = S_HEADER_KEY;
					break;
				}

				// TODO check

				break;
			case S_HEADER_KEY:
			case S_HEADER_KEY_START:

				if (parser_state == S_HEADER_KEY_START) {

					if (memcmp (ch, "Connection:", 11) == 0) {
						parser_state = S_HEADER_VALUE;
						current_field = H_CONNECTION;

						tmp_index = 0;
						ch += 11;
						break;
					}

					parser_state = S_HEADER_KEY;
					break;
				}

				if (c == '\n') {
					parser_state = S_END;
					break;
				}

				if (c == ':') {
					parser_state = S_HEADER_VALUE;
					break;
				}
				break;
			case S_HEADER_VALUE:

				if (c == '\n') {
					parser_state = S_HEADER_KEY;
				}

				switch (current_field) {
					case H_CONNECTION:
						if (memcmp (ch, "keep-alive", 10) == 0)
							keep_alive = 1;

						break;
				}

				break;

			case S_END:
				ch = NULL;
				break;
		}

		index++;
		ch++;
	}

	strcpy (self->parsed_request.path, path);
	self->parsed_request.method = method;

	return 1;
}

#define BASE_PATH "/home/tom/Code/js/game"
#define DATE_FORMAT "%a, %d %b %Y %H:%M:%S %Z"
#define RESPONSE_TEMPLATE "HTTP/1.1 %i %s\n\r" \
                          "Date: %s\n\r" \
						  "Server: Awesome Tom Server! (elementary OS)\n\r" \
						  "Last-Modified: %s\n\r" \
						  "Content-Type: text/html; charset=UTF-8\n\r" \
						  "Content-Length: %li\n\r" \
						  "Connection: close\n\r" \
                          "\n\r%s"

static void
_connection_send_response_header (Connection *self)
{
	int fd, remaining, len;

	if (self->state == CONNECTED) {
		char path[1024];

		struct stat stat_buf;

		time_t date;
		struct tm *dateinfo;
		char date_buffer[64];
		char last_modified_buffer[64];

		self->state = HEADER_SENDING;
		self->response_data_sent = 0;

		if (self->parsed_request.path[strlen (self->parsed_request.path) - 1] == '/')
			strcat (self->parsed_request.path, "test.html");
		strcpy (path, BASE_PATH);
		strcat (path, self->parsed_request.path);

		date = time (NULL);
		dateinfo = localtime (&date);
		strftime (date_buffer, 64, DATE_FORMAT, dateinfo);

		fd = open (path, O_RDONLY);
		if (fd < 0 || fstat (fd, &stat_buf) < 0) {

			sprintf (self->response_buffer, RESPONSE_TEMPLATE, 404, "Not Found",
			         date_buffer, date_buffer, 16l, "404. Not found.");

		} else {

			dateinfo = localtime (&stat_buf.st_mtime);
			strftime (last_modified_buffer, 64, DATE_FORMAT, dateinfo);

			sprintf (self->response_buffer, RESPONSE_TEMPLATE, 200, "OK", date_buffer,
					 last_modified_buffer, stat_buf.st_size, "");

			self->response_file_fd = fd;
			self->response_file_size = stat_buf.st_size;
		}

		self->response_buffer_length = strlen (self->response_buffer);
		printf ("MY RESPONSE: %s\n", self->response_buffer);
	}

	remaining = self->response_buffer_length - self->response_data_sent;
	len = send (self->fd, self->response_buffer + self->response_data_sent, remaining, 0);

	if (len < 0) {
		perror ("sending header failed");
		self->state = BODY_SENT;
		return;
	}

	self->response_data_sent += len;

	if (self->response_data_sent >= self->response_buffer_length) {
		self->state = HEADER_SENT;
	}
}

static void
_connection_send_response_body (Connection *self)
{
	int len;

	if (self->state == HEADER_SENT) {
		self->state = BODY_SENDING;
		self->response_data_sent = 0;
	}

	len = sendfile (self->fd, self->response_file_fd,
	                &self->response_data_sent,
	                self->response_file_size - self->response_data_sent);

	if (len < 0) {
		perror ("sendfile failed");
		self->state = BODY_SENT;
		return;
	}

	self->response_data_sent += len;

	if (self->response_data_sent >= self->response_file_size) {
		close (self->response_data_sent);

		self->state = BODY_SENT;
	}
}


void
connection_send_response (Connection *self)
{
	if (self->state == CONNECTED || self->state == HEADER_SENDING) {
		_connection_send_response_header (self);
	} else if (!self->response_file_fd) {
		self->state = BODY_SENT;
	} else {
		_connection_send_response_body (self);
	}
}
