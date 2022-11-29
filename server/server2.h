#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#include <stdbool.h>

#include "../protocol/protocol.h"
#include "server-config.h"

#else

#error not defined for this platform

#endif

#include "client2.h"

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);

//static bool username_already_used(const char * client_name, Client *clients, int actual);
static void send_broadcast_message(Client *clients, Client * client, Message *m, int actual);
static void send_str_from_server_to_client(char * str, Client * dest);
static void send_message(Client * sender, Message * msg, Client * clients, int actual);
static void send_server_infos(Client * dest, Client * clients, int actual);

#endif /* guard */
