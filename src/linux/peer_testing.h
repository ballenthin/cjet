/*
 *The MIT License (MIT)
 *
 * Copyright (c) <2014> <Stephan Gatzka>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef CJET_PEER_TESTING_H
#define CJET_PEER_TESTING_H

#ifdef TESTING

#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

int fake_read(int fd, void *buf, size_t count);
int fake_send(int fd, void *buf, size_t count, int flags);
int fake_writev(int fd, const struct iovec *iov, int iovcnt);
int fake_close(int fd);

#ifdef __cplusplus
}
#endif

#define READ fake_read
#define SEND fake_send
#define WRITEV fake_writev
#define CLOSE fake_close
#else

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#define READ read
#define SEND send
#define WRITEV writev
#define CLOSE close

#endif

#endif
