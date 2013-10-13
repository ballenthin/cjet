#include <endian.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "peer.h"
#include "compiler.h"
#include "cJSON.h"

#define READ_MSG_LENGTH 0
#define READ_MSG 1
#define WRITE_OP 2

static struct peer *alloc_peer(int fd)
{
	struct peer *p;
	p = malloc(sizeof(*p));
	if (unlikely(p == NULL)) {
		return NULL;
	}
	p->fd = fd;
	p->op = READ_MSG_LENGTH;
	p->read_ptr = p->buffer;
	p->write_ptr = p->buffer;
	return p;
}

static void free_peer(struct peer *p)
{
	free(p);
}

static void close_peer_connection(struct peer *p, int epoll_fd, int fd)
{
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
	free_peer(p);
}

static int unread_space(struct peer *p)
{
	return &(p->buffer[MAX_MESSAGE_SIZE]) - p->read_ptr;
}

static int free_space(struct peer *p)
{
	return &(p->buffer[MAX_MESSAGE_SIZE]) - p->write_ptr;
}

static void reorganize_buffer(struct peer *p)
{
	unsigned int unread = p->write_ptr - p->read_ptr;
	if (unread != 0) {
		memmove(p->buffer, p->read_ptr, unread);
		p->write_ptr = p->buffer + unread;
	} else {
		p->write_ptr = p->buffer;
	}
	p->read_ptr = p->buffer;
}

static char *get_read_ptr(struct peer *p, int count)
{
	if (unlikely(count > unread_space(p))) {
		fprintf(stderr, "peer asked for too much data: %d!\n", count);
		return NULL;
	}
	while (1) {
		int read_length;
		if (p->write_ptr - p->read_ptr >= count) {
			char *read_ptr = p->read_ptr;
			p->read_ptr += count;
			return read_ptr;
		}

		read_length = read(p->fd, p->write_ptr, free_space(p));
		if (unlikely(read_length == 0)) {
			fprintf(stdout, "peer closed connection!\n");
			return NULL;
		}
		if (read_length == -1) {
			if ((errno != EAGAIN) &&
			    (errno != EWOULDBLOCK)) {
				fprintf(stderr, "unexpected read error: %d!\n", errno);
				return NULL;
			}
			return (char*)-1;
		}
		p->write_ptr += read_length;
	}
}

static int handle_message(char *msg, uint32_t length)
{
	cJSON *root;
	const char *end_parse;

	root = cJSON_ParseWithOpts(msg, &end_parse, 0);
	if (unlikely(root == NULL)) {
		fprintf(stderr, "Could not parse JSON!\n");
		return -1;
	} else {
		uint32_t parsed_length = end_parse - msg;
		if (unlikely(parsed_length != length)) {
			fprintf(stderr, "length of parsed JSON does not math to message length!\n");
			return -1;
		}
		cJSON_Delete(root);
		return 0;
	}
}

static int process_all_messages(struct peer *p, int epoll_fd)
{
	uint32_t message_length;
	char *message_ptr;

	while (1) {
		char *message_length_ptr;
		int ret;

		switch (p->op) {
		case READ_MSG_LENGTH:
			message_length_ptr = get_read_ptr(p, sizeof(message_length));
			if (unlikely(message_length_ptr == NULL)) {
				close_peer_connection(p, epoll_fd, p->fd);
				return -1;
			} else if (message_length_ptr == (char *)-1) {
				return 0;
			}
			memcpy(&message_length, message_length_ptr, sizeof(message_length));
			message_length = be32toh(message_length);
			p->op = READ_MSG;
			p->msg_length = message_length;
			/*
			 *  CAUTION! This fall through is by design! Typically, the
			 *  length of a messages and the message itself will go into
			 *  a single TCP packet. This fall through eliminates an
			 *  additional loop iteration
			 */

		case READ_MSG:
			message_length = p->msg_length;
			message_ptr = get_read_ptr(p, message_length);
			if (unlikely(message_ptr == NULL)) {
				close_peer_connection(p, epoll_fd, p->fd);
				return -1;
			} else if (message_ptr == (char *)-1) {
				return 0;
			}
			ret = handle_message(message_ptr, message_length);
			if (unlikely(ret == -1)) {
				close_peer_connection(p, epoll_fd, p->fd);
			}
			p->op = READ_MSG_LENGTH;
			reorganize_buffer(p);
			break;

		default:
			fprintf(stderr, "Unknown client operation!\n");
			return -1;
			break;
		}
	}
}

void handle_all_peer_operations(struct peer *p, int epoll_fd)
{
	process_all_messages(p, epoll_fd);
	return;
}

static int add_epoll(int epoll_fd, int epoll_op, int fd, void *cookie)
{
	struct epoll_event ev;

	memset(&ev, 0, sizeof(ev));
	ev.data.ptr = cookie;
	ev.events = EPOLLIN | EPOLLET;
	if (unlikely(epoll_ctl(epoll_fd, epoll_op, fd, &ev) < 0)) {
		fprintf(stderr, "epoll_ctl failed!\n");
		return -1;
	}
	return 0;
}

void *peer_create_wait(int fd, int epoll_fd)
{
	struct peer *peer;
	peer = alloc_peer(fd);
	if (unlikely(peer == NULL)) {
		fprintf(stderr, "Could not allocate peer!\n");
		goto alloc_peer_failed;
	}

	if (unlikely(add_epoll(epoll_fd, EPOLL_CTL_ADD, fd, peer) < 0)) {
		goto epollctl_failed;
	}
	return peer;

epollctl_failed:
	free_peer(peer);
alloc_peer_failed:
	return NULL;
}

void peer_unwait_delete(struct peer *p, int epoll_fd)
{
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, p->fd, NULL);
	close(p->fd);
	free_peer(p);
}
