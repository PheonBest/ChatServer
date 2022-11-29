#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#define CRLF        "\r\n"
#define MAX_CLIENTS     100
#define MAX_ROOMS 100
#define BUF_SIZE    1024

#endif