#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <linux/limits.h>

#define errprint(...)      fprintf(stderr, __VA_ARGS__)
#define voidprint(...)     /* */
#if !defined(DEBUG)
#define dbgprint           voidprint
#else
#define dbgprint           errprint
#endif

#if !defined(FALSE)
#define FALSE          (0 > 1)
#endif

#if !defined(TRUE)
#define TRUE           (1 > 0)
#endif

#define VIDEO_REQ "CMD /v2/video?%dx%d"
#define INVALID_SOCKET -1

#define make_int(num, b1, b2)    \
    num = 0; num |= (b1 & 0xFF); \
    num <<= 8; num |= (b2 & 0xFF);

#define make_int4(num, b0, b1, b2, b3) \
    num = 0;                           \
    num |= (b3 & 0xFF); num <<= 8;     \
    num |= (b2 & 0xFF); num <<= 8;     \
    num |= (b1 & 0xFF); num <<= 8;     \
    num |= (b0 & 0xFF)

typedef int socket_t;

extern void showerror();
socket_t connect_cam(char *ip, int port);
void connection_cleanup();
void disconnect(socket_t s);
int iostream(int doSend, char *buffer, int bytes, socket_t s);
