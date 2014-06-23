#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdint.h>

#include "server.h"

#define MAX_CONNECTIONS 1024

Server *
server_new (int port)
{
	Server *self;
	struct epoll_event ev;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons (port);
	addr.sin_addr.s_addr = INADDR_ANY;

	self = malloc (sizeof (Server));

	if ((self->server_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror ("Can't create TCP socket");
		return NULL;
	}

	if (bind (self->server_socket, (struct sockaddr *) &addr, sizeof (addr)) < 0) {
		perror ("binding to address failed");
		return NULL;
	}

	int one = 1;
	if (setsockopt (self->server_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof (int)) < 0) {
		perror ("setsockopt failed");
		return NULL;
	}

	if (listen (self->server_socket, MAX_CONNECTIONS) < 0) {
		perror ("listen failed");
		return NULL;
	}

	if (fcntl (self->server_socket, F_SETFL, O_NONBLOCK, 1) < 0) {
		perror ("changing to non-blocking");
		return NULL;
	}

	if (fcntl (self->server_socket, F_SETFL, O_ASYNC, 1) < 0) {
		perror ("changing to async");
		return NULL;
	}

	self->connections = NULL;

	self->epfd = epoll_create1 (0);

	ev.events = EPOLLIN;
	ev.data.fd = self->server_socket;
	epoll_ctl (self->epfd, EPOLL_CTL_ADD, self->server_socket, &ev);

	return self;
}

static void
_server_close_client (Server *self, int fd)
{
	struct epoll_event ev;

	printf ("CLIENT CLOSED. %i\n", fd);

	ev.events = 0;
	ev.data.fd = fd;

	epoll_ctl (self->epfd, EPOLL_CTL_DEL, fd, &ev);
	shutdown (fd, SHUT_RDWR);
}

static void
_server_handle_client_connect (Server *self)
{
	Connection *connection;
	int client_fd;

	struct epoll_event ev;
	struct sockaddr addr;
	socklen_t addrlen;

	client_fd = accept (self->server_socket, &addr, &addrlen);

	connection = connection_new (client_fd, (struct sockaddr_in*) &addr, addrlen);
	printf ("Connected client from %s\n", connection->ip);

	self->connections = hash_table_set (self->connections,
	                                    INT_TO_POINTER (client_fd), connection);

	ev.events = EPOLLIN;
	ev.data.fd = client_fd;
	epoll_ctl (self->epfd, EPOLL_CTL_ADD, client_fd, &ev);
}

static void
_server_handle_client_read (Server *self, int fd)
{
	Connection *connection;
	struct epoll_event ev;
	int len;
	char buffer[1024];

	connection = hash_table_get (self->connections, INT_TO_POINTER (fd));

	if (connection == NULL) {
		fprintf (stderr, "attempt to write to an unknown socket\n");
		return;
	}

	len = recv (fd, buffer, 1024, 0);
	if (len < -1) {
		perror ("receiving data failed");
		return;
	}

	if (len == 0 ||
	    !connection_add_request_data (connection, buffer, len)) {

		if (!connection_parse_request (connection))
			_server_close_client (self, fd);
		else {
			printf ("SETTING TO SEND MODE\n");
			ev.events = EPOLLOUT;
			ev.data.fd = fd;
			epoll_ctl (self->epfd, EPOLL_CTL_MOD, fd, &ev);
		}

	}
}

static void
_server_handle_client_write (Server *self, int fd)
{
	Connection *connection;

	connection = hash_table_get (self->connections, INT_TO_POINTER (fd));

	connection_send_response (connection);

	if (connection->state == BODY_SENT)
		_server_close_client (self, fd);
}

int
server_run (Server *self)
{
#define MAX_EVENTS 16

	int nfds, n;
	struct epoll_event events[MAX_EVENTS];

	for (;;) {
		nfds = epoll_wait (self->epfd, events, MAX_EVENTS, -1);

		if (nfds < 0) {
			perror ("Getting epoll events failed");
			return 1;
		}

		for (n = 0; n < nfds; n++) {
			if (events[n].data.fd == self->server_socket)
				_server_handle_client_connect (self);

			else if (events[n].events & EPOLLIN)
				_server_handle_client_read (self, events[n].data.fd);

			else if (events[n].events & EPOLLOUT)
				_server_handle_client_write (self, events[n].data.fd);

			else if (events[n].events & EPOLLHUP ||
			         events[n].events & EPOLLERR ||
			         events[n].events % EPOLLRDHUP)
				_server_close_client (self, events[n].data.fd);
		}
	}

	return 0;
}

