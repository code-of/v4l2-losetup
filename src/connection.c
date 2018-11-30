#include "common.h"
#include <arpa/inet.h>
#include <sys/socket.h>

socket_t connect_cam(char *ip, int port)
{
    struct sockaddr_in sin;
    socket_t sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == INVALID_SOCKET) {
        return sock;
    } else {
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = inet_addr(ip);
        sin.sin_port = htons(port);

        if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            errprint("connect failed %d '%s'\n", errno, strerror(errno));
            close(sock);
            return INVALID_SOCKET;
        }
        return sock;
    }
}

int iostream(int doSend, char *buffer, int bytes, socket_t s)
{
    int retCode;
    char *ptr = buffer;

    while (bytes > 0) {
        retCode = (doSend) ? send(s, ptr, bytes, 0) : recv(s, ptr, bytes, 0);
        if (retCode <= 0)
            return retCode;
        ptr += retCode;
        bytes -= retCode;
    }
    return EXIT_FAILURE;
}

void disconnect(socket_t s)
{
    close(s);
}
